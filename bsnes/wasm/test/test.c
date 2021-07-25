#include <stdint.h>

struct ppux_sprite {
  uint8_t  enabled;

  uint16_t x;
  uint16_t y;
  uint8_t  hflip;
  uint8_t  vflip;

  uint8_t  vram_space;        //     0 ..   255; 0 = local, 1..255 = extra
  uint16_t vram_addr;         // $0000 .. $FFFF (byte address)
  uint8_t  cgram_space;       //     0 ..   255; 0 = local, 1..255 = extra
  uint8_t  palette;           //     0 ..   255

  uint8_t  layer;             // 0.. 4;  BG1 = 0, BG2 = 1, BG3 = 2, BG4 = 3, OAM = 4
  uint8_t  priority;          // 1..12
  uint8_t  color_exemption;   // true = ignore color math, false = obey color math

  uint8_t  bpp;               // 2-, 4-, or 8-bpp tiles from vram[extra] and cgram[extra]
  uint16_t width;             // number of pixels width
  uint16_t height;            // number of pixels high
};

enum ppux_memory_type : uint32_t {
  VRAM,
  CGRAM
};

__attribute__((import_module("env"), import_name("ppux_reset")))
void ppux_reset();

__attribute__((import_module("env"), import_name("ppux_sprite_reset")))
void ppux_sprite_reset();

__attribute__((import_module("env"), import_name("ppux_sprite_read")))
int32_t ppux_sprite_read(uint32_t i_index, struct ppux_sprite *o_spr);

__attribute__((import_module("env"), import_name("ppux_sprite_write")))
int32_t ppux_sprite_write(uint32_t i_index, struct ppux_sprite *i_spr);

__attribute__((import_module("env"), import_name("ppux_ram_write")))
int32_t ppux_ram_write(enum ppux_memory_type i_memorytype, uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("snes_bus_read")))
void snes_bus_read(uint32_t i_address, uint8_t *i_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("snes_bus_write")))
void snes_bus_write(uint32_t i_address, uint8_t *o_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("msg_recv")))
int32_t msg_recv(uint8_t *o_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("msg_size")))
int32_t msg_size(uint16_t *o_size);

uint16_t bus_read_u16(uint32_t i_address) {
  uint16_t d;
  snes_bus_read(i_address, (uint8_t *)&d, sizeof(uint16_t));
  return d;
}

uint32_t msgs = 0, last_msgs = 0;
uint8_t  msg[65536];
uint16_t last_msg_size = 0;

int copied = 0;

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

struct loc {
  uint8_t  enabled;
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
} locs[80][12];
uint8_t loc_tail = 0;

uint8_t sprites[0x2000];

// called on NMI:
void on_nmi() {
  uint32_t spr_index = 0;
  struct ppux_sprite spr;

  uint16_t sp_addr[32] = { 0x0ACC, 0x0ACC, 0x0AD0, 0x0AD0, 0x0AD4, 0x0AC0, 0x0AC0, 0x0AC4, 0x0AC4, 0x0AC8, 0x0AC8, 0x0AE0, 0x0AD8, 0x0AD8, 0x0AF6, 0x0AF6,
                           0x0ACE, 0x0ACE, 0x0AD2, 0x0AD2, 0x0AD6, 0x0AC2, 0x0AC2, 0x0AC6, 0x0AC6, 0x0ACA, 0x0ACA, 0x0AE2, 0x0ADA, 0x0ADA, 0x0AF8, 0x0AF8 };
  uint8_t  sp_bpp[32]  = {      4,      4,      4,      4,      4,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3,
                                4,      4,      4,      4,      4,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3,      3 };
  uint8_t  sp_offs[32] = {      0,     16,      0,     16,      0,      0,     16,      0,     16,      0,     16,      0,      0,     16,      0,     16,
                                0,     16,      0,     16,      0,      0,     16,      0,     16,      0,     16,      0,      0,     16,      0,     16 };

  uint8_t  oam[0x2A0];
  uint16_t link_oam_start;

  if (!copied) {
    snes_bus_read(0x108000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x0000, sprites, 0x2000);
    snes_bus_read(0x10A000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x2000, sprites, 0x2000);
    snes_bus_read(0x10C000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x4000, sprites, 0x2000);
    snes_bus_read(0x10E000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x6000, sprites, 0x2000);

    // TODO: determine WHEN to copy here
    // copy 3bpp->4bpp decompressed sprites from WRAM:
    // keep the same bank offset ($9000) in VRAM as it is in WRAM, cuz why not?
    snes_bus_read(0x7E9000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x9000, sprites, 0x2000);
    snes_bus_read(0x7EB000, sprites, 0x1000);
    ppux_ram_write(VRAM, 1, 0xB000, sprites, 0x1000);
    // TODO: decompress 3bpp to 4bpp ourselves and identify ROM source of 3bpp graphics depending on sword type, shield type, etc.

    copied = 1;
    ppux_sprite_reset();
  }

  if (msgs != last_msgs) {
    draw_message();
    last_msgs = msgs;
  }

  snes_bus_read(0x7E0352, (uint8_t *)&link_oam_start, 2);
  if (link_oam_start >= 0x200) return;

  snes_bus_read(0x7E0800, oam, 0x2A0);

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

  // move all previous recorded frames down by one:
  for (int j = 79; j >= 1; j--) {
    for (int i = 0; i < 12; i++) {
      locs[j][i] = locs[j - 1][i];
    }
  }

  // get screen x,y offset by reading BG2 scroll registers:
  uint16_t xoffs = bus_read_u16(0x7E00E2) - (int16_t)bus_read_u16(0x7E011A);
  uint16_t yoffs = bus_read_u16(0x7E00E8) - (int16_t)bus_read_u16(0x7E011C);

  for (unsigned i = 0; i < 12; i++) {
    unsigned o = link_oam_start + (i<<2);
    locs[0][i].enabled = 0;

    if (oam[o+1] == 0xF0) continue;

    uint16_t chr = (uint16_t)oam[o+2] + ((uint16_t)(oam[o+3] & 0x01) << 8);
    if (chr >= 0x20) continue;

    uint8_t ex = oam[0x220 + (o>>2)];
    int16_t x = (uint16_t)oam[o + 0] + ((uint16_t)(ex & 0x01) << 8);
    int16_t y = (uint8_t)(oam[o + 1]);
    if (x >= 256) x -= 512;
    if (y >= 240) y -= 256;

    locs[0][i].enabled = 1;

    locs[0][i].x = xoffs + x;
    locs[0][i].y = yoffs + y;
    //spr.x = bus_read_u16(0x7E0022);
    //spr.y = bus_read_u16(0x7E0020);

    locs[0][i].hflip = (oam[o + 3] & 0x40) >> 6;
    locs[0][i].vflip = (oam[o + 3] & 0x80) >> 7;

    locs[0][i].palette = (((oam[o + 3] >> 1) & 7) << 4) + 0x80;
    locs[0][i].priority = ((oam[o + 3] >> 4) & 3);

    locs[0][i].width = 8 << ((ex & 0x02) >> 1);
    locs[0][i].height = 8 << ((ex & 0x02) >> 1);

    snes_bus_read(0x7E0000 + sp_addr[chr], (uint8_t *)&locs[0][i].offs_top, sizeof(uint16_t));
    locs[0][i].offs_top += sp_offs[chr];
    if (chr < 0x10) {
      snes_bus_read(0x7E0000 + sp_addr[chr+0x10], (uint8_t *)&locs[0][i].offs_bot, sizeof(uint16_t));
      locs[0][i].offs_bot += sp_offs[chr+0x10];
    }

    if (sp_bpp[chr] == 4) {
      locs[0][i].offs_top = (locs[0][i].offs_top - 0x8000);
      locs[0][i].offs_bot = (locs[0][i].offs_bot - 0x8000);
    }

    locs[0][i].bpp = sp_bpp[chr];
    //locs[0][i].width = sp_size[chr];
    //locs[0][i].height = sp_size[chr];
  }

  uint8_t pri_lkup[4] = { 2, 3, 6, 9 };

  for (unsigned i = 0; i < 12; i++) {
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

    ppux_sprite_write(spr_index++, &spr);
  }
}
