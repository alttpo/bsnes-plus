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


void on_msg_recv() {
  uint8_t msg[65536];
  uint16_t size;

  if (!msg_size(&size)) return;
  if (!msg_recv(msg, sizeof(msg))) return;


}

int copied = 0;

// called on NMI:
void on_nmi() {
  uint16_t link_oam_start;
  uint16_t link_index[2] = { 0x200, 0x200 };
  uint32_t link_addr[2]  = { 0x7E0ACC, 0x7E0AD0 };
  uint8_t  oam[0x2A0];

  if (!copied) {
    uint8_t sprites[0x2000];
    snes_bus_read(0x108000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x0000, sprites, 0x2000);
    snes_bus_read(0x10A000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x2000, sprites, 0x2000);
    snes_bus_read(0x10C000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x4000, sprites, 0x2000);
    snes_bus_read(0x10E000, sprites, 0x2000);
    ppux_ram_write(VRAM, 1, 0x6000, sprites, 0x2000);
    copied = 1;
    ppux_sprite_reset();
  }

  snes_bus_read(0x7E0352, (uint8_t *)&link_oam_start, 2);
  if (link_oam_start >= 0x200) return;

  snes_bus_read(0x7E0800, oam, 0x2A0);

  for (unsigned i = 0; i < 0xC; i++) {
    unsigned o = link_oam_start + (i<<2);
    if (oam[o+1] == 0xF0) continue;

    uint16_t chr = (uint16_t)oam[o+2] + ((uint16_t)(oam[o+3] & 0x01) << 8);

    if (chr == 0x00) {
      link_index[0] = o;
    } else if (chr == 0x02) {
      link_index[1] = o;
    }
  }

  struct ppux_sprite spr;
  spr.enabled = 1;
  spr.vram_space = 1;
  spr.cgram_space = 0;
  spr.palette = 0xF0;
  spr.layer = 1;
  spr.priority = 6;
  spr.color_exemption = 0;
  spr.bpp = 4;
  spr.width = 16;
  spr.height = 16;
  for (unsigned i = 0; i < 2; i++) {
    unsigned o = link_index[i];
    if (o == 0x200) continue;

    uint8_t ex = oam[0x220 + (o>>2)];

    spr.x = (uint16_t)oam[o + 0] + ((uint16_t)(ex & 0x01) << 8);
    spr.y = (uint8_t)(oam[o + 1] - 0x18);
    spr.hflip = oam[o + 3] & 0x40;
    spr.vflip = oam[o + 3] & 0x80;

    spr.vram_addr = 0;
    snes_bus_read(link_addr[i], (uint8_t *)&spr.vram_addr, sizeof(uint16_t));
    spr.vram_addr = (spr.vram_addr - 0x8000);

    ppux_sprite_write(i, &spr);
  }
}
