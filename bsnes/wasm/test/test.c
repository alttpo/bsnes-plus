#include <stdint.h>
#include "wasm.h"
#include "../../../external/printf/printf.c"

void _putchar(char character) {

}

#undef printf
int printf(const char* format, ...) {
  char buf[1024];
  va_list va;
  va_start(va, format);
  const int ret = vsnprintf_(buf, 1024, format, va);
  va_end(va);
  puts(buf);
  return ret;
}

uint16_t bus_read_u16(uint32_t i_address) {
  uint16_t d;
  snes_bus_read(i_address, (uint8_t *)&d, sizeof(uint16_t));
  return d;
}

uint8_t bus_read_u8(uint32_t i_address) {
  uint8_t d;
  snes_bus_read(i_address, (uint8_t *)&d, sizeof(uint8_t));
  return d;
}

int copied = 0;

struct loc {
  uint8_t  enabled;
  uint16_t chr;
  uint16_t x;
  uint16_t y;
  uint8_t  hflip;
  uint8_t  vflip;
  uint16_t offs_top;
  uint16_t offs_bot;
  uint8_t  palette;
  uint8_t  priority;
  uint8_t  bpp;
  uint8_t  width;
  uint8_t  height;
} locs[80][128];
uint8_t loc_tail = 0;

uint16_t xoffs;
uint16_t yoffs;

uint8_t last_sword = 0xFF;
uint8_t last_shield = 0xFF;

uint8_t oam[0x2A0];
uint8_t oam_used[128];

uint16_t last_msg_size = 0;
uint32_t msgs = 0, last_msgs = 0;
uint8_t  msg[65536];

uint8_t sprites[0x2000];
uint8_t pak5e[0x600*2];

const uint16_t sp_addr[32] =
  { 0x0ACC, 0x0ACC, 0x0AD0, 0x0AD0, 0x0AD4, 0x0AC0, 0x0AC0, 0x0AC4, 0x0AC4, 0x0AC8, 0x0AC8, 0x0AE0, 0x0AD8, 0x0AD8, 0x0AF6, 0x0AF6,
    0x0ACE, 0x0ACE, 0x0AD2, 0x0AD2, 0x0AD6, 0x0AC2, 0x0AC2, 0x0AC6, 0x0AC6, 0x0ACA, 0x0ACA, 0x0AE2, 0x0ADA, 0x0ADA, 0x0AF8, 0x0AF8 };
const uint8_t  sp_bpp[32]  =
  {      4,      4,      4,      4,      4,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3,
         4,      4,      4,      4,      4,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3 };
const uint8_t  sp_offs[32] =
  {      0,     32,      0,     32,      0,      0,     32,      0,     32,      0,     32,      0,      0,     32,      0,     32,
         0,     32,      0,     32,      0,      0,     32,      0,     32,      0,     32,      0,      0,     32,      0,     32 };

__attribute__((export_name("on_msg_recv")))
void on_msg_recv() {
  uint16_t size;

  if (msg_size(&last_msg_size) != 0) {
    msgs = 0xFF;
    return;
  }
  if (msg_recv(msg, sizeof(msg)) != 0) {
    msgs = 0xFE;
    return;
  }

  msgs++;
}

void draw_message() {
  uint32_t spr_index = 1024 - 32;
  struct ppux_sprite spr;

  spr.enabled = 1;
  spr.vram_space = 0;
  spr.cgram_space = 0;
  spr.palette = 0x0C;
  spr.layer = 1;
  spr.priority = 7;
  spr.color_exemption = 0;
  spr.bpp = 2;
  spr.width = 8;
  spr.height = 8;
  spr.hflip = 0;
  spr.vflip = 0;

  spr.x = 0xF0;
  spr.y = 0xD8;
  {
    // draw number of messages received in lower-right corner:
    uint32_t n = msgs;
    do {
      spr.vram_addr = 0xE900 + ((n % 10) << 4);
      ppux_sprite_write(spr_index++, &spr);

      spr.x -= 0x08;
      n /= 10;
    } while (n > 0);
  }

  // draw message text:
  spr.x = 0;
  uint16_t i;
  for (i = 0; i < last_msg_size; i++) {
    uint8_t c = msg[i];
    if (c >= 'A' && c <= 'Z') {
      spr.vram_addr = 0xF500 + ((c - 'A') << 4);
    } else if (c >= 'a' && c <= 'z') {
      spr.vram_addr = 0xF500 + ((c - 'a') << 4);
    } else if (c >= '0' && c <= '9') {
      spr.vram_addr = 0xE900 + ((c - '0') << 4);
    } else if (c == ' ' || c == '\n') {
      spr.x += 4;
      continue;
    } else {
      spr.vram_addr = 0xF500 + (26 << 4);
    }

    if (spr_index >= 1024) {
      break;
    }
    ppux_sprite_write(spr_index++, &spr);
    spr.x += 8;
  }
  for (; spr_index < 1024; spr_index++) {
    spr.enabled = 0;
    ppux_sprite_write(spr_index, &spr);
  }
}

uint32_t get_graphics_addr(uint8_t index) {
  uint32_t addr = 0;

  uint8_t t = 0;

  //LDA $CFF3, Y : STA $CA
  t = bus_read_u8(0x00CFF3 + index);
  addr |= t;
  addr <<= 8;

  //LDA $D0D2, Y : STA $C9
  t = bus_read_u8(0x00D0D2 + index);
  addr |= t;
  addr <<= 8;

  //LDA $D1B1, Y : STA $C8
  t = bus_read_u8(0x00D1B1 + index);
  addr |= t;

  return addr;
}

uint32_t decomp3bpp(uint8_t *buf, uint8_t *dest) {
  const int D_CMD_COPY = 0;
  const int D_CMD_BYTE_REPEAT = 1;
  const int D_CMD_WORD_REPEAT = 2;
  const int D_CMD_BYTE_INC = 3;
  const int D_CMD_COPY_EXISTING = 4;

  uint8_t *d = dest;

  // decompress the data to dest:
  uint32_t i = 0;
  uint8_t header = buf[i];
  while (header != 0xFF) {
    uint8_t cmd = header >> 5;
    uint32_t len = header & 0x1F;

    if (cmd == 7) {
      cmd = (header >> 2) & 7;
      len = ((uint32_t)((header & 3) << 8)) + buf[i + 1];
      i++;
    }

    len++;

    switch (cmd) {
      case D_CMD_COPY:
        memcpy(d, &buf[i+1], len);
        d += len;
        i += len + 1;
        break;
      case D_CMD_BYTE_REPEAT:
        memset(d, buf[i+1], len);
        d += len;
        i += 2;
        break;
      case D_CMD_WORD_REPEAT: {
        uint8_t a = buf[i+1];
        uint8_t b = buf[i+2];
        for (int j = 0; j < len; j += 2) {
          d[j] = a;
          if ((j + 1) < len) {
            d[j+1] = b;
          }
        }
        d += len;
        i += 3;
      } break;
      case D_CMD_BYTE_INC:
        for (int j = 0; j < len; j++) {
          d[j] = (uint8_t)(buf[i + 1] + j);
        }
        d += len;
        i += 2;
        break;
      case D_CMD_COPY_EXISTING: {
        uint16_t offs;
        offs = ((uint16_t)buf[i+1]) | ((uint16_t)buf[i+2] << 8);
        memcpy(d, dest+offs, len);
        d += len;
        i += 3;
      } break;
      default:
        puts("bad command\n");
        break;
    }

    header = buf[i];
  }

  return d - dest;
}

uint16_t expand3to4bpp(uint8_t *src, uint8_t *dest, uint16_t count, uint16_t x, uint16_t s00) {
  for (; count > 0; count--) {
    uint16_t s10 = s00 + 0x10;  // = $03
    for (int i = 7; i >= 0; i--) {
      uint16_t c = *(uint16_t *)(&src[s00]);
      s00 += 2;
      *(uint16_t *)(&dest[x]) = c;

      // XBA : ORA [$00] : AND.w #$00FF : STA $08 // = l
      uint16_t l = ((c >> 8) | c) & 0x00FF;

      // LDA [$03] : AND.w #$00FF : STA $BD // = t
      uint16_t t = src[s10++];
      // ORA $08 : XBA : ORA $BD
      c = ((t | l) << 8) | t;
      *(uint16_t *)(&dest[x + 0x10]) = c;

      x += 2;
    }

    x += 0x10;

    if ((s10 & 0x0078) == 0) {
      s10 += 0x180;
    }

    s00 = s10;
  }

  return x;
}

void decompress_sprites() {
  struct ppux_sprite spr;

  memset(sprites, 0, 0x2000);

  // copy link 4bpp body sprites:
  snes_bus_read(0x108000, sprites, 0x2000);
  ppux_ram_write(VRAM, 1, 0x0000, sprites, 0x2000);
  snes_bus_read(0x10A000, sprites, 0x2000);
  ppux_ram_write(VRAM, 1, 0x2000, sprites, 0x2000);
  snes_bus_read(0x10C000, sprites, 0x2000);
  ppux_ram_write(VRAM, 1, 0x4000, sprites, 0x2000);
  snes_bus_read(0x10E000, sprites, 0x2000);
  ppux_ram_write(VRAM, 1, 0x6000, sprites, 0x2000);

  // decompress sword+shield 3bpp data:
  uint8_t  buf[0x1000];
  uint8_t *pak5f = &pak5e[0x600];

  uint32_t addr = get_graphics_addr(0x5F);
  snes_bus_read(addr, buf, 0x1000);
  decomp3bpp(buf, pak5f);

  addr = get_graphics_addr(0x5E);
  snes_bus_read(addr, buf, 0x1000);
  decomp3bpp(buf, pak5e);

  //hexdump(pak5e, 0xC00);

  // load uncompressed 3bpp graphics pack #00:
  addr = get_graphics_addr(0x00);
  snes_bus_read(addr, buf, 0x1000);

  // draw our 3bpp sprites:
  spr.enabled = 1;
  spr.cgram_space = 0;
  spr.palette = 0xC0;
  spr.bpp = 4;
  spr.height = 8;
  spr.color_exemption = 0;
  spr.hflip = 0;
  spr.vflip = 0;
  spr.layer = 4;
  spr.priority = 6;
  spr.vram_space = 1;

  uint16_t t0C = 0;

  uint32_t x = 0x0480;
  uint16_t y;
  uint16_t xx = 0;
  uint16_t s00;

  // ice/fire rod:
  y = 0x7;
  spr.vram_addr = 0x9000 + x;
  spr.width = y * 0x08;
  spr.x = 16+xx;
  spr.y = 64;
  ppux_sprite_write(512, &spr);
  s00 = bus_read_u16(0x00D21D + t0C);
  x = expand3to4bpp(buf, sprites, y, x, s00);

  spr.vram_addr = 0x9000 + x;
  spr.x = 16+xx;
  spr.y = 64+8;
  ppux_sprite_write(513, &spr);
  xx += y*8;
  x = expand3to4bpp(buf, sprites, y, x, s00 + 0x180);
  t0C += 2;

  // hammer:
  y = 0x7;
  spr.vram_addr = 0x9000 + x;
  spr.width = y * 0x08;
  spr.x = 16+xx;
  spr.y = 64;
  ppux_sprite_write(514, &spr);
  s00 = bus_read_u16(0x00D21D + t0C);
  x = expand3to4bpp(buf, sprites, y, x, s00);

  spr.vram_addr = 0x9000 + x;
  spr.x = 16+xx;
  spr.y = 64+8;
  ppux_sprite_write(515, &spr);
  xx += y*8;
  x = expand3to4bpp(buf, sprites, y, x, s00 + 0x180);
  t0C += 2;

  // bow:
  y = 0x3;
  spr.vram_addr = 0x9000 + x;
  spr.width = y * 0x08;
  spr.x = 16+xx;
  spr.y = 64;
  ppux_sprite_write(516, &spr);
  s00 = bus_read_u16(0x00D21D + t0C);
  x = expand3to4bpp(buf, sprites, y, x, s00);

  spr.vram_addr = 0x9000 + x;
  spr.x = 16+xx;
  spr.y = 64+8;
  ppux_sprite_write(517, &spr);
  xx += y*8;
  x = expand3to4bpp(buf, sprites, y, x, s00 + 0x180);
  t0C += 2;

  // switch to decompressed pak $5F

  // shovel:
  y = 0x4;
  spr.vram_addr = 0x9000 + x;
  spr.width = y * 0x08;
  spr.x = 16+xx;
  spr.y = 64;
  ppux_sprite_write(518, &spr);
  s00 = bus_read_u16(0x00D21D + t0C);
  x = expand3to4bpp(pak5f, sprites, y, x, s00);

  spr.vram_addr = 0x9000 + x;
  spr.x = 16+xx;
  spr.y = 64+8;
  ppux_sprite_write(519, &spr);
  xx += y*8;
  x = expand3to4bpp(pak5f, sprites, y, x, s00 + 0x180);
  t0C += 2;

  // Zzz and music notes:
  y = 0x3;
  spr.vram_addr = 0x9000 + x;
  spr.width = y * 0x08;
  spr.x = 16+xx;
  spr.y = 64;
  ppux_sprite_write(520, &spr);
  s00 = bus_read_u16(0x00D21D + t0C);
  x = expand3to4bpp(pak5f, sprites, y, x, s00);

  spr.vram_addr = 0x9000 + x;
  spr.x = 16+xx;
  spr.y = 64+8;
  ppux_sprite_write(521, &spr);
  xx += y*8;
  x = expand3to4bpp(pak5f, sprites, y, x, s00 + 0x180);
  t0C += 2;

  // dash dust:
  y = 0x1;
  spr.vram_addr = 0x9000 + x;
  spr.width = y * 0x08;
  spr.x = 16+xx;
  spr.y = 64;
  ppux_sprite_write(522, &spr);
  s00 = bus_read_u16(0x00D21D + t0C);
  x = expand3to4bpp(pak5f, sprites, y, x, s00);

  spr.vram_addr = 0x9000 + x;
  spr.x = 16+xx;
  spr.y = 64+8;
  ppux_sprite_write(523, &spr);
  xx += y*8;
  x = expand3to4bpp(pak5f, sprites, y, x, s00 + 0x180);
  t0C += 2;

  // back to uncompressed 3bpp graphics pack #00:

  // hookshot:
  y = 0x4;
  spr.vram_addr = 0x9000 + x;
  spr.width = y * 0x08;
  spr.x = 16+xx;
  spr.y = 64;
  ppux_sprite_write(524, &spr);
  s00 = bus_read_u16(0x00D21D + t0C);
  x = expand3to4bpp(buf, sprites, y, x, s00);

  spr.vram_addr = 0x9000 + x;
  spr.x = 16+xx;
  spr.y = 64+8;
  ppux_sprite_write(525, &spr);
  xx += y*8;
  x = expand3to4bpp(buf, sprites, y, x, s00 + 0x180);
  t0C += 2;

  // decompress pak $60:
  uint8_t tmp[0x1000];
  uint8_t pak60[0x1000];
  addr = get_graphics_addr(0x60);
  snes_bus_read(addr, tmp, 0x1000);
  decomp3bpp(tmp, pak60);

  // bug net:
  xx = 0;
  y = 0xE;
  spr.vram_addr = 0x9000 + x;
  spr.width = y * 0x08;
  spr.x = 16+xx;
  spr.y = 80;
  ppux_sprite_write(526, &spr);
  s00 = bus_read_u16(0x00D21D + t0C);
  x = expand3to4bpp(pak60, sprites, y, x, s00);

  spr.vram_addr = 0x9000 + x;
  spr.x = 16+xx;
  spr.y = 80+8;
  ppux_sprite_write(527, &spr);
  xx += y*8;
  x = expand3to4bpp(pak60, sprites, y, x, s00 + 0x180);
  t0C += 2;

  // cane:
  y = 0x7;
  spr.vram_addr = 0x9000 + x;
  spr.width = y * 0x08;
  spr.x = 16+xx;
  spr.y = 80;
  ppux_sprite_write(528, &spr);
  s00 = bus_read_u16(0x00D21D + t0C);
  x = expand3to4bpp(pak60, sprites, y, x, s00);

  spr.vram_addr = 0x9000 + x;
  spr.x = 16+xx;
  spr.y = 80+8;
  ppux_sprite_write(529, &spr);
  xx += y*8;
  x = expand3to4bpp(pak60, sprites, y, x, s00 + 0x180);
  t0C += 2;

  // book opened:
  y = 0x2;
  spr.vram_addr = 0x9000 + x;
  spr.width = y * 0x08;
  spr.x = 16+xx;
  spr.y = 80;
  ppux_sprite_write(530, &spr);
  s00 = bus_read_u16(0x00D21D + t0C);
  x = expand3to4bpp(pak5f, sprites, y, x, s00);

  spr.vram_addr = 0x9000 + x;
  spr.x = 16+xx;
  spr.y = 80+8;
  ppux_sprite_write(531, &spr);
  xx += y*8;
  x = expand3to4bpp(pak5f, sprites, y, x, s00 + 0x180);
  t0C += 2;

  ppux_sprite_reset();
  //hexdump(sprites, 0x2000);
}

uint16_t oam_chr(unsigned o) {
  return (uint16_t)oam[o+2] + ((uint16_t)(oam[o+3] & 0x01) << 8);
}

int16_t oam_x(unsigned o) {
  uint8_t ex = oam[0x220 + (o>>2)];
  int16_t x = (uint16_t)oam[o+0] + ((uint16_t)(ex & 0x01) << 8);
  if (x >= 256) x -= 512;
  return x;
}

int16_t oam_y(unsigned o) {
  int16_t y = (uint8_t)(oam[o+1]);
  if (y >= 240) y -= 256;
  return y;
}

void oam_convert(unsigned o, unsigned i) {
  locs[0][i].enabled = 0;
  if (oam_used[i] == 0) return;

  if (oam[o+1] == 0xF0) return;

  uint16_t chr = oam_chr(o);
  int16_t  x = oam_x(o);
  int16_t  y = oam_y(o);

  locs[0][i].enabled = 1;
  locs[0][i].chr = chr;

  locs[0][i].x = xoffs + x;
  locs[0][i].y = yoffs + y;
  locs[0][i].hflip = (oam[o + 3] & 0x40) >> 6;
  locs[0][i].vflip = (oam[o + 3] & 0x80) >> 7;

  locs[0][i].palette = (((oam[o + 3] >> 1) & 7) << 4) + 0x80;
  locs[0][i].priority = ((oam[o + 3] >> 4) & 3);

  uint8_t ex = oam[0x220 + (o>>2)];
  locs[0][i].width = 8 << ((ex & 0x02) >> 1);
  locs[0][i].height = 8 << ((ex & 0x02) >> 1);

  if (chr < 0x20) {
    locs[0][i].offs_top = bus_read_u16(0x7E0000 + sp_addr[chr]);
    locs[0][i].offs_top += sp_offs[chr];
    if (chr < 0x10) {
      locs[0][i].offs_bot = bus_read_u16(0x7E0000 + sp_addr[chr + 0x10]);
      locs[0][i].offs_bot += sp_offs[chr + 0x10];
    }

    if (sp_bpp[chr] == 4) {
      locs[0][i].offs_top = (locs[0][i].offs_top - 0x8000);
      locs[0][i].offs_bot = (locs[0][i].offs_bot - 0x8000);
    }

    // can compress offs_top and offs_bot together into 24 bits on the wire:
    //printf("%02x: %032b\n", i, ((uint32_t)locs[0][i].offs_top >> 5) | ((uint32_t)locs[0][i].offs_bot << 7));

    locs[0][i].bpp = sp_bpp[chr];
    //locs[0][i].width = sp_size[chr];
    //locs[0][i].height = sp_size[chr];
  } else {
    locs[0][i].bpp = 4;
  }
}

void strcpy(char *d, const char *s) {
  for (; *s; s++) {
    *d++ = *s;
  }
}

__attribute__((export_name("on_frame_present")))
void on_frame_present() {
  static uint16_t cmd[] = {
    4, CMD_PIXEL, 0x1F3F, 18, 18,
    9, CMD_IMAGE, 20, 20, 2, 2,
    0x001F, 0x03E0,
    0x7C00, 0x001F,
    5, CMD_HLINE, 0x7FFF, 24, 24, 16,
    5, CMD_VLINE, 0x7FFF, 24, 24, 16,
    7, CMD_RECT, 0x7C00, 0xFFFF, 32, 32, 8, 8,
    12, CMD_TEXT_UTF8, 0x03E0, 0x001F, 0, 80, 80,
    0, 0, 0, 0, 0, 0,
  };
  strcpy((char *)&cmd[(1+4+1+9+1+5+1+5+1+7+1+6)], "jsd1982");

  // increment y:
  cmd[1+4+1+9+1+5+1+5+1+7+1+5]++;
  if (cmd[1+4+1+9+1+5+1+5+1+7+1+5] == 500) {
    cmd[1+4+1+9+1+5+1+5+1+7+1+5] = 80;
    // increment x:
    cmd[1+4+1+9+1+5+1+5+1+7+1+4] += 4;
    if (cmd[1+4+1+9+1+5+1+5+1+7+1+4] == 500) {
      cmd[1+4+1+9+1+5+1+5+1+7+1+4] = 80;
    }
  }

  ppux_draw_list_clear();
  ppux_draw_list_append(0 | 0x80, 9 | 0x80, sizeof(cmd), cmd);

  uint8_t pri_lkup[4] = { 2, 3, 6, 9 };

  for (unsigned j = 0; j < 128; j++) {
    // draw in reverse order because higher-indexed sprites and lower priority than lower-indexed ones:
    unsigned i = 127 - j;

    if (locs[79][i].enabled == 0) continue;

    int32_t x = locs[79][i].x - xoffs;
    int32_t y = locs[79][i].y - yoffs;

    // visible bounds check:
    if (y <= -16 || y >= 240) {
      continue;
    }
    if (x <= -16 || x >= 256) {
      continue;
    }

    uint16_t dl[13];
    dl[0] = 12; // length in uint16_ts
    dl[1] = CMD_VRAM_TILE;
    dl[2] = x & 511;
    dl[3] = y & 255;
    dl[4] = locs[79][i].hflip;
    dl[5] = locs[79][i].vflip;

    dl[8] = 0;
    dl[9] = locs[79][i].palette;
    uint8_t priority = pri_lkup[locs[79][i].priority];

    dl[10] = 4;
    dl[11] = locs[79][i].width;
    dl[12] = locs[79][i].height;

    if (locs[79][i].chr < 0x20) {
      dl[6] = 1;
      dl[7] = locs[79][i].offs_top;

      if (locs[79][i].bpp == 3 && dl[12] == 16) {
        dl[12] = 8;
        if (dl[5]) {
          dl[3] += 8;
        }
        ppux_draw_list_append(4, priority, sizeof(dl), dl);

        if (dl[5]) {
          dl[3] -= 8;
        } else {
          dl[3] += 8;
        }
        dl[7] = locs[79][i].offs_bot;
      }
    } else {
      dl[6] = 0;
      dl[7] = 0x8000 + (locs[79][i].chr << 5);
    }

    ppux_draw_list_append(4, priority, sizeof(dl), dl);
  }
}

// called on NMI:
__attribute__((export_name("on_nmi")))
void on_nmi() {
  uint16_t link_oam_start;

  if (!copied) {
    copied = 1;
    ppux_sprite_reset();

    decompress_sprites();
  }

  uint8_t curr_sword = bus_read_u8(0x7EF359);
  if (curr_sword != last_sword) {
    // load sword graphics:
    uint16_t sword_idx = bus_read_u16(0x00D2BE + (curr_sword << 1));

    uint32_t x = 0;
    x = expand3to4bpp(pak5e, sprites, 0xC, x, sword_idx);
    expand3to4bpp(pak5e, sprites, 0xC, x, sword_idx + 0x180);

    //hexdump(sprites, x);
  }

  uint8_t curr_shield = bus_read_u8(0x7EF35A);
  if (curr_shield != last_shield) {
    // load shield graphics:
    uint16_t shield_idx = bus_read_u16(0x00D300 + (curr_shield << 1));

    uint32_t x = 0x300;
    x = expand3to4bpp(pak5e, sprites, 0x6, x, shield_idx);
    expand3to4bpp(pak5e, sprites, 0x6, x, shield_idx + 0x180);

    //hexdump(&sprites[0x300], x);
  }

  if ((curr_sword != last_sword) || (curr_shield != last_shield)) {
#if 0
    // copy 3bpp->4bpp decompressed sprites from WRAM:
    // keep the same bank offset ($9000) in VRAM as it is in WRAM, cuz why not?
    snes_bus_read(0x7E9000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x9000, sprites, 0x2000);
    snes_bus_read(0x7EB000, sprites, 0x1000);
    ppux_ram_write(VRAM, 1, 0xB000, sprites, 0x1000);
#endif

    ppux_ram_write(VRAM, 1, 0x9000, sprites, 0x2000);

    last_shield = curr_shield;
    last_sword = curr_sword;
  }

  if (msgs != last_msgs) {
    draw_message();
    last_msgs = msgs;
  }

  link_oam_start = bus_read_u16(0x7E0352);
  if (link_oam_start >= 0x200) return;

  snes_bus_read(0x7E0800, oam, 0x2A0);

  // move all previous recorded frames down by one:
  for (int j = 79; j >= 1; j--) {
    for (int i = 0; i < 128; i++) {
      locs[j][i] = locs[j - 1][i];
    }
  }

  // get screen x,y offset by reading BG2 scroll registers:
  xoffs = bus_read_u16(0x7E00E2) - (int16_t)bus_read_u16(0x7E011A);
  yoffs = bus_read_u16(0x7E00E8) - (int16_t)bus_read_u16(0x7E011C);

  //spr.x = bus_read_u16(0x7E0022);
  //spr.y = bus_read_u16(0x7E0020);

  for (unsigned i = 0; i < 128; i++) {
    locs[0][i].enabled = 0;
  }

  // mark all visible sprites as used:
  memset(oam_used, 0, 128);
  for (unsigned i = 0; i < 128; i++) {
    unsigned o = (i<<2);
    if (oam[o+1] == 0xF0) continue;

    // ignore shadows:
    uint16_t chr = oam_chr(o);
    if (chr == 0x6C) continue;
    if (chr == 0x38) continue;
    if (chr == 0x28) continue;

    // enable visible sprites:
    oam_used[i] = 1;
  }

  // unmark undesirable sprites:
  for (unsigned i = 0; i < 128; i++) {
    if (oam_used[i] == 0) continue;

    unsigned o = (i<<2);
    if (oam[o+1] == 0xF0) continue;

    uint16_t chr = oam_chr(o);

    if (chr >= 0x100) {
      // enemy sprites:
      oam_used[i] = 0;
    } else if ((chr >= 0x40 && chr <= 0x4F) || (chr >= 0x50 && chr <= 0x5F) || chr == 0xE9) {
      // throwable items:
      oam_used[i] = 0;
    } else if ((chr >= 0x60 && chr <= 0x6B) || (chr >= 0x70 && chr <= 0x7B) || chr == 0xE0 || chr == 0xE5) {
      // item drop:
      oam_used[i] = 0;
    } else if ((chr >= 0x84 && chr <= 0x8F) || (chr >= 0x94 && chr <= 0x9F) || chr == 0xAA) {
      // explosions:
      oam_used[i] = 0;
    } else if (chr == 0x6E) {
      // bomb:
      oam_used[i] = 0;
    } else if (chr == 0xEA || chr == 0xEC) {
      // faerie / fairy:
      oam_used[i] = 0;
    } else if (chr == 0xE4 || chr == 0xF4) {
      // bee:
      oam_used[i] = 0;
    } else if (chr == 0x44 || chr == 0x70) {
      // skull rock enemy:
      oam_used[i] = 0;
    } else if (chr == 0x29 || chr == 0x39) {
      // heart drop:
      oam_used[i] = 0;
    } else if (chr == 0x0B || chr == 0x1B) {
      // rupee:
      oam_used[i] = 0;
    } else if (chr == 0x0C) {
      // movable block:
      oam_used[i] = 0;
    } else if ((chr >= 0xC8 && chr <= 0xCE) || (chr >= 0xD8 && chr <= 0xDE)) {
      // grass/water ripples:
      oam_used[i] = 0;
    } else if (chr == 0xB4 || chr == 0xB5 || chr == 0xB8 || chr == 0xB9 || chr == 0xA8) {
      // enemy death:
      oam_used[i] = 0;
    } else if (chr == 0xA9 || chr == 0x9B) {
      // enemy death only if 4 copies of it all in sequence:
      if (oam_chr(o+4) == chr && oam_chr(o+8) == chr && oam_chr(o+12) == chr) {
        oam_used[i+0] = 0;
        oam_used[i+1] = 0;
        oam_used[i+2] = 0;
        oam_used[i+3] = 0;
      }
    } else if (chr == 0xB7 || chr == 0x80) {
      // sparkle for cemetery ghost sprite:
      if (oam_chr(o+4) == 0x1E6) {
        oam_used[i+0] = 0;
        oam_used[i+1] = 0;
      }
    }
  }

  // mark all of Link's sprites as used:
  for (unsigned i = 0; i < 12; i++) {
    unsigned o = link_oam_start + (i<<2);
    if (oam[o+1] == 0xF0) continue;

    oam_used[o>>2] = 1;
  }

  // finally convert only the used sprites:
  for (unsigned i = 0; i < 128; i++) {
    unsigned o = (i<<2);
    oam_convert(o, i);
  }
}
