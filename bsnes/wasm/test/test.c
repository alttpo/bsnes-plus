#include <stdint.h>
#include "wasm.h"

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

#define anc_sprites 10
const uint32_t anc_start = 0xBF0;
const uint32_t anc_size = 0xC9A - anc_start;

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
} locs[80][anc_sprites+12];
uint8_t loc_tail = 0;

uint32_t last_spr_index = 0;

uint16_t last_msg_size = 0;
uint32_t msgs = 0, last_msgs = 0;
uint8_t  msg[65536];

uint8_t sprites[0x2000];
uint8_t spr3bpp[0x1000];

uint8_t last_sword = 0xFF;
uint8_t last_shield = 0xFF;

const uint16_t sp_addr[32] =
  { 0x0ACC, 0x0ACC, 0x0AD0, 0x0AD0, 0x0AD4, 0x0AC0, 0x0AC0, 0x0AC4, 0x0AC4, 0x0AC8, 0x0AC8, 0x0AE0, 0x0AD8, 0x0AD8, 0x0AF6, 0x0AF6,
    0x0ACE, 0x0ACE, 0x0AD2, 0x0AD2, 0x0AD6, 0x0AC2, 0x0AC2, 0x0AC6, 0x0AC6, 0x0ACA, 0x0ACA, 0x0AE2, 0x0ADA, 0x0ADA, 0x0AF8, 0x0AF8 };
const uint8_t  sp_bpp[32]  =
  {      4,      4,      4,      4,      4,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3,
         4,      4,      4,      4,      4,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3 };
const uint8_t  sp_offs[32] =
  {      0,     32,      0,     32,      0,      0,     32,      0,     32,      0,     32,      0,      0,     32,      0,     32,
         0,     32,      0,     32,      0,      0,     32,      0,     32,      0,     32,      0,      0,     32,      0,     32 };

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

uint32_t decomp3bpp(uint8_t index, uint8_t *buf, uint8_t *dest) {
  const int D_CMD_COPY = 0;
  const int D_CMD_BYTE_REPEAT = 1;
  const int D_CMD_WORD_REPEAT = 2;
  const int D_CMD_BYTE_INC = 3;
  const int D_CMD_COPY_EXISTING = 4;

  uint8_t *d = dest;
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

  // read a large chunk of data to decompress:
  snes_bus_read(addr, buf, 0x800);

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
        m3printf("bad command\n", 0);
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

void oam_convert(uint8_t *oam, unsigned o, unsigned i, uint16_t xoffs, uint16_t yoffs) {
  locs[0][i].enabled = 0;

  if (oam[o+1] == 0xF0) return;

  uint16_t chr = (uint16_t)oam[o+2] + ((uint16_t)(oam[o+3] & 0x01) << 8);

  uint8_t ex = oam[0x220 + (o>>2)];
  int16_t x = (uint16_t)oam[o + 0] + ((uint16_t)(ex & 0x01) << 8);
  int16_t y = (uint8_t)(oam[o + 1]);
  if (x >= 256) x -= 512;
  if (y >= 240) y -= 256;

  locs[0][i].enabled = 1;
  locs[0][i].chr = chr;

  locs[0][i].x = xoffs + x;
  locs[0][i].y = yoffs + y;
  locs[0][i].hflip = (oam[o + 3] & 0x40) >> 6;
  locs[0][i].vflip = (oam[o + 3] & 0x80) >> 7;

  locs[0][i].palette = (((oam[o + 3] >> 1) & 7) << 4) + 0x80;
  locs[0][i].priority = ((oam[o + 3] >> 4) & 3);

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

    locs[0][i].bpp = sp_bpp[chr];
    //locs[0][i].width = sp_size[chr];
    //locs[0][i].height = sp_size[chr];
  } else {
    locs[0][i].bpp = 4;
  }
}

int32_t sync_ancilla(uint8_t t) {
  if (t == 0) return 0;

  // 0x26 = sparkles when swinging lvl 2 or higher sword
  //if (t == 0x26) return -1;
  // 0x2A = sparkles starting spin attack
  //if (t == 0x2A) return -1;
  // 0x2B = sparkles during spin attack
  //if (t == 0x2B) return -1;
  // 0x3C = sparkles when charing spin attach
  //if (t == 0x3C) return -1;

  // yes:
  return -1;
}

// called on NMI:
void on_nmi() {
  uint32_t spr_index = 0;
  struct ppux_sprite spr;

  uint8_t  oam[0x2A0];
  uint16_t link_oam_start;

  uint8_t anc[anc_size];

  if (!copied) {
    snes_bus_read(0x108000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x0000, sprites, 0x2000);
    snes_bus_read(0x10A000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x2000, sprites, 0x2000);
    snes_bus_read(0x10C000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x4000, sprites, 0x2000);
    snes_bus_read(0x10E000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x6000, sprites, 0x2000);

    // decompress sprite data:
    uint8_t  buf[0x1000];
    decomp3bpp(0x5F, buf, &spr3bpp[0x600]);
    decomp3bpp(0x5E, buf, &spr3bpp[0x000]);

    //hexdump(spr3bpp, 0xC00);

    memset(sprites, 0, 0x2000);

    copied = 1;
    ppux_sprite_reset();
  }

  uint8_t curr_sword = bus_read_u8(0x7EF359);
  if (curr_sword != last_sword) {
    // load sword graphics:
    uint16_t sword_idx = bus_read_u16(0x00D2BE + (curr_sword << 1));

    uint32_t x = 0;
    x = expand3to4bpp(spr3bpp, sprites, 0xC, x, sword_idx);
    expand3to4bpp(spr3bpp, sprites, 0xC, x, sword_idx + 0x180);

    //hexdump(sprites, x);
  }

  uint8_t curr_shield = bus_read_u8(0x7EF35A);
  if (curr_shield != last_shield) {
    // load shield graphics:
    uint16_t shield_idx = bus_read_u16(0x00D300 + (curr_shield << 1));

    uint32_t x = 0x300;
    x = expand3to4bpp(spr3bpp, sprites, 0x6, x, shield_idx);
    expand3to4bpp(spr3bpp, sprites, 0x6, x, shield_idx + 0x180);

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
    for (int i = 0; i < anc_sprites+12; i++) {
      locs[j][i] = locs[j - 1][i];
    }
  }

  // get screen x,y offset by reading BG2 scroll registers:
  uint16_t xoffs = bus_read_u16(0x7E00E2) - (int16_t)bus_read_u16(0x7E011A);
  uint16_t yoffs = bus_read_u16(0x7E00E8) - (int16_t)bus_read_u16(0x7E011C);

  //spr.x = bus_read_u16(0x7E0022);
  //spr.y = bus_read_u16(0x7E0020);

  // process ancillae:
  snes_bus_read(0x7E0BF0, anc, anc_size);
  for (unsigned i = 0; i < anc_sprites; i++) {
    locs[0][i].enabled = 0;
  }
  unsigned j = 0;
  for (unsigned i = 0; i < 10; i++) {
    // check ancilla type:
    uint8_t t = anc[(0xC4A - anc_start) + i];

    if (!sync_ancilla(t)) continue;

    // add all oam sprites for this ancilla:
    uint8_t start = anc[(0xC86 - anc_start) + i];
    uint8_t end   = anc[(0xC90 - anc_start) + i];
    if (end == 0) end = 4;
    for (uint8_t o = start; o < start+end; o += 4) {
      oam_convert(oam, o, j++, xoffs, yoffs);
    }
  }

  for (unsigned j = 0; j < 12; j++) {
    unsigned o = link_oam_start + (j<<2);
    unsigned i = anc_sprites+j;
    oam_convert(oam, o, i, xoffs, yoffs);
  }

  uint8_t pri_lkup[4] = { 2, 3, 6, 9 };

  spr.enabled = 1;
  spr.cgram_space = 0;
  spr.color_exemption = 0;
  spr.hflip = 0;
  spr.vflip = 0;
  spr.vram_space = 1;
  spr.palette = 0xF0;
  spr.layer = 4;
  spr.priority = 6;
  spr.bpp = 4;
  spr.width = 16;
  spr.height = 16;

  for (unsigned i = 0; i < anc_sprites+12; i++) {
    spr.enabled = locs[79][i].enabled;

    int32_t x = locs[79][i].x - xoffs;
    int32_t y = locs[79][i].y+1 - yoffs;

    // visible bounds check:
    if (y <= -16 || y >= 240) {
      spr.enabled = 0;
    }
    if (x <= -16 || x >= 256) {
      spr.enabled = 0;
    }

    spr.x = x & 511;
    spr.y = y & 255;
    spr.hflip = locs[79][i].hflip;
    spr.vflip = locs[79][i].vflip;
    spr.palette = locs[79][i].palette;
    spr.priority = pri_lkup[locs[79][i].priority];

    spr.bpp = 4;
    spr.width = locs[79][i].width;
    spr.height = locs[79][i].height;

    if (locs[79][i].chr < 0x20) {
      spr.vram_space = 1;
      spr.vram_addr = locs[79][i].offs_top;
      if (locs[79][i].bpp == 3 && spr.height == 16) {
        spr.height = 8;
        if (spr.vflip) {
          spr.y += 8;
        }
        ppux_sprite_write(spr_index++, &spr);

        if (spr.vflip) {
          spr.y -= 8;
        } else {
          spr.y += 8;
        }
        spr.vram_addr = locs[79][i].offs_bot;
      }
    } else {
      spr.vram_space = 0;
      spr.vram_addr = 0x8000 + (locs[79][i].chr << 5);
    }

    ppux_sprite_write(spr_index++, &spr);
  }

  for (unsigned i = spr_index; i < last_spr_index; i++) {
    spr.enabled = 0;
    ppux_sprite_write(i, &spr);
  }

  last_spr_index = spr_index;
}
