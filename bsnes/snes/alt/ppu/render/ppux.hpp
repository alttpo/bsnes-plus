
const static unsigned extra_count = 4096;
struct extra_item {
  bool   enabled;

  uint16 x;
  uint16 y;
  bool   hflip;
  bool   vflip;

  uint8  vram_space;        //     0 ..   255; 0 = local, 1..255 = extra
  uint16 vram_addr;         // $0000 .. $FFFF (byte address)
  uint8  cgram_space;       //     0 ..   255; 0 = local, 1..255 = extra
  uint8  palette;           //     0 ..   255

  uint8  layer;             // 0.. 4;  BG1 = 0, BG2 = 1, BG3 = 2, BG4 = 3, OAM = 4
  uint8  priority;          // 1..12
  bool   color_exemption;   // true = ignore color math, false = obey color math

  uint8  bpp;               // 2-, 4-, or 8-bpp tiles from vram[extra] and cgram[extra]
  uint16 width;             // number of pixels width
  uint16 height;            // number of pixels high
} extra_list[extra_count];

const static unsigned extra_spaces = 255;
StaticRAM *vram_space[extra_spaces];
StaticRAM *cgram_space[extra_spaces];

// ppux.cpp
uint8* get_vram_space(uint8 space);
uint8* get_cgram_space(uint8 space);
void   render_line_extra();
