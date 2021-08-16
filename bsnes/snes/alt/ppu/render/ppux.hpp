
const static unsigned extra_spaces = 255;
StaticRAM*  vram_space[extra_spaces];
StaticRAM* cgram_space[extra_spaces];

uint16 ppux_mode7_col[2][1024 * 1024];

uint16 ppux_layer_col[256 * 256];
uint8  ppux_layer_pri[256 * 256];
uint8  ppux_layer_lyr[256 * 256];
struct ppux_draw_layer {
  uint8 layer;
  uint8 priority;
  std::vector<uint8_t> cmdlist;
};
std::vector<ppux_draw_layer> ppux_draw_lists;

// ppux.cpp
inline uint16 get_palette_space(uint8 space, uint8 index);
uint8* get_vram_space(uint8 space);
uint8* get_cgram_space(uint8 space);
void   ppux_render_frame_pre();
void   ppux_render_line_pre();
void   ppux_render_line_post();
void   ppux_mode7_fetch(int32 px, int32 py, int32 tile, unsigned layer, int32 &palette, uint16& color);

void ppux_draw_list_reset();
void ppux_vram_reset();
void ppux_cgram_reset();
void ppux_reset();
