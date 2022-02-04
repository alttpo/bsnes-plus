
uint16 ppux_mode7_col[2][1024 * 1024];

uint16 ppux_layer_col[256 * 256];
uint8  ppux_layer_pri[256 * 256];
uint8  ppux_layer_lyr[256 * 256];

struct ppux_module {
  ppux_module(
    const std::string& key,
    const std::shared_ptr<DrawList::FontContainer>& fonts,
    const std::shared_ptr<DrawList::SpaceContainer>& spaces
  );

  std::string key;

  std::shared_ptr<DrawList::FontContainer> fonts;
  std::shared_ptr<DrawList::SpaceContainer> spaces;

  std::vector< std::vector<uint16_t> > draw_lists;
};
std::vector<ppux_module> ppux_modules;

// ppux.cpp
uint8* ppux_get_oam();
void   ppux_render_frame_pre();
void   ppux_render_line_pre();
void   ppux_render_line_post();
void   ppux_mode7_fetch(int32 px, int32 py, int32 tile, unsigned layer, int32 &palette, uint16& color);
