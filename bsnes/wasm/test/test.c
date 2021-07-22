#include <stdint.h>

struct ppux_sprite {
  uint32_t index;
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
  uint8_t  color_exemption;     // true = ignore color math, false = obey color math

  uint8_t  bpp;               // 2-, 4-, or 8-bpp tiles from vram[extra] and cgram[extra]
  uint16_t width;             // number of pixels width
  uint16_t height;            // number of pixels high
};

__attribute__((import_module("env"), import_name("ppux_reset")))
void ppux_reset();

__attribute__((import_module("env"), import_name("ppux_sprite_write")))
int32_t ppux_sprite_write(struct ppux_sprite *);


// called on NMI:
void on_nmi() {
  ppux_reset();

  struct ppux_sprite spr;
  spr.index = 0;
  spr.enabled = 1;
  spr.x = 129;
  spr.y = 99;
  spr.hflip = 0;
  spr.vflip = 0;
  spr.vram_space = 0;
  spr.vram_addr = 59024;
  spr.cgram_space = 0;
  spr.palette = 0;
  spr.layer = 1;
  spr.priority = 5;
  spr.color_exemption = 0;
  spr.bpp = 2;
  spr.width = 16;
  spr.height = 8;
  ppux_sprite_write(&spr);
}
