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

__attribute__((import_module("env"), import_name("ppux_reset")))
void ppux_reset();

__attribute__((import_module("env"), import_name("ppux_sprite_reset")))
void ppux_sprite_reset();

__attribute__((import_module("env"), import_name("ppux_sprite_read")))
int32_t ppux_sprite_read(uint32_t i_index, struct ppux_sprite *o_spr);

__attribute__((import_module("env"), import_name("ppux_sprite_write")))
int32_t ppux_sprite_write(uint32_t i_index, struct ppux_sprite *i_spr);

__attribute__((import_module("env"), import_name("snes_bus_read")))
void snes_bus_read(uint32_t i_address, uint8_t *i_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("snes_bus_write")))
void snes_bus_write(uint32_t i_address, uint8_t *o_data, uint32_t i_size);

// called on NMI:
void on_nmi() {
  uint8_t  oam[0x200];
  uint16_t link_index = 0;

  ppux_sprite_reset();

  snes_bus_read(0x7E0352, (uint8_t *)&link_index, 2);
  snes_bus_read(0x7E0800, oam, 0x200);

  for (unsigned i = 0; i < 0xC; i++) {
    unsigned o = (i<<2);
    if ((oam[link_index+o+1] != 0xF0) && (oam[link_index+o+2] == 0x00)) {
      link_index += o;
      break;
    }
  }

  struct ppux_sprite spr;
  spr.enabled = 1;
  spr.x = oam[(link_index)+0];
  spr.y = oam[(link_index)+1] - 0x04;
  spr.hflip = 0;
  spr.vflip = 0;
  spr.vram_space = 0;
  spr.vram_addr = 0xE690;
  spr.cgram_space = 0;
  spr.palette = 0;
  spr.layer = 1;
  spr.priority = 6;
  spr.color_exemption = 0;
  spr.bpp = 2;
  spr.width = 16;
  spr.height = 8;
  ppux_sprite_write(0, &spr);
}
