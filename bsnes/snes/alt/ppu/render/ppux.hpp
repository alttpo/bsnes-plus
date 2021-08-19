
uint16 ppux_mode7_col[2][1024 * 1024];

uint16 ppux_layer_col[256 * 256];
uint8  ppux_layer_pri[256 * 256];
uint8  ppux_layer_lyr[256 * 256];

struct ppux_draw_layer {
  uint8 layer;
  uint8 priority;
  DrawList::FontContainer* fonts;
  DrawList::SpaceContainer* spaces;
  std::vector<uint8_t> cmdlist;
};
std::vector<ppux_draw_layer> ppux_draw_lists;

// ppux.cpp
uint8* ppux_get_oam();
void   ppux_render_frame_pre();
void   ppux_render_line_pre();
void   ppux_render_line_post();
void   ppux_mode7_fetch(int32 px, int32 py, int32 tile, unsigned layer, int32 &palette, uint16& color);

void   ppux_draw_list_reset();
