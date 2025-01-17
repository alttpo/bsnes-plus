#ifdef PPU_CPP

PPU::ppux_module::ppux_module(
  const std::string& p_key,
  const std::shared_ptr<DrawList::FontContainer>& p_fonts,
  const std::shared_ptr<DrawList::SpaceContainer>& p_spaces
) : key(p_key), fonts(p_fonts), spaces(p_spaces)
{}

uint8* PPU::ppux_get_oam() {
  return memory::oam.data();
}

struct Mode7PreTransformPlot {
  Mode7PreTransformPlot(DrawList::draw_layer p_layer, uint8_t p_priority) : layer(p_layer), priority(p_priority) {}

  DrawList::draw_layer layer;
  uint8_t priority;

  void operator() (int x, int y, uint16_t color) {
    // draw to mode7 pre-transform BG1 or BG2:
    if (layer > DrawList::BG1)
      return;

    // TODO: priority

    x &= 1023;
    y &= 1023;

    auto offs = (y << 10) + x;
    ppu.ppux_mode7_col[layer][offs] = color;
  }
};

struct LayerPlot {
  LayerPlot(DrawList::draw_layer p_layer, uint8_t p_priority) : layer(p_layer), priority(p_priority) {}

  DrawList::draw_layer layer;
  uint8_t priority;

  void operator() (int x, int y, uint16_t color) {
    // draw to any PPU layer:
    auto offs = (y << 8) + x;
    if ((ppu.ppux_layer_pri[offs] == 0xFF) || ((ppu.ppux_layer_pri[offs]&0x7F) <= priority)) {
      ppu.ppux_layer_lyr[offs] = layer;
      ppu.ppux_layer_pri[offs] = priority;
      ppu.ppux_layer_col[offs] = color;
    }
  }
};

using LayerRenderer = DrawList::GenericRenderer<256, 256, LayerPlot>;
using Mode7PreTransformRenderer = DrawList::GenericRenderer<1024, 1024, Mode7PreTransformPlot>;

void PPU::ppux_render_frame_pre() {
  // extra sprites drawn on pre-transformed mode7 BG1 or BG2 layers:
  for(int i = 0; i < 1024*1024; i++) {
    ppux_mode7_col[0][i] = 0xffff;
    ppux_mode7_col[1][i] = 0xffff;
  }

  // pre-render ppux draw_lists to frame buffers:
  memset(ppux_layer_pri, 0xFF, sizeof(ppux_layer_pri));
  for (const auto& mo : ppux_modules) {
    for (const auto& dl : mo.draw_lists) {
      // this function is called from CMD_TARGET draw list command to switch renderers:
      DrawList::ChooseRenderer chooseRenderer = [=](DrawList::draw_layer i_layer, bool i_pre_mode7_transform, uint8_t i_priority, std::shared_ptr<DrawList::Renderer>& o_renderer) {
        // select drawing target:
        o_renderer = (i_pre_mode7_transform)
          ? (std::shared_ptr<DrawList::Renderer>) std::make_shared<Mode7PreTransformRenderer>(i_layer, i_priority)
          : (std::shared_ptr<DrawList::Renderer>) std::make_shared<LayerRenderer>(i_layer, i_priority);
      };

      DrawList::Context context(chooseRenderer, mo.fonts, mo.spaces);

      // render the draw_list:
      context.draw_list(dl);
    }
  }
}

void PPU::ppux_mode7_fetch(int32 px, int32 py, int32 tile, unsigned layer, int32 &palette, uint16& color) {
  px &= 1023;
  py &= 1023;

  int32 ix = (py << 10) + px;

  color = ppux_mode7_col[layer][ix];
  if(color < 0x8000) {
    // TODO: set palette to 0 or 1 based on priority; need to capture priority values from pre-render
    palette = 0;
    return;
  }

  palette = memory::vram[(((tile << 6) + ((py & 7) << 3) + (px & 7)) << 1) + 1];
}

void PPU::ppux_render_line_pre() {
}

void PPU::ppux_render_line_post() {
  // render draw_lists:
  int offs = (line-1) * 256;
  for (int sx = 0; sx < 256; sx++, offs++) {
    uint8_t priority = ppux_layer_pri[offs];
    if (priority == 0xFF) continue;

    uint8_t layer = ppux_layer_lyr[offs];
    // (layer & 0x80) means pre-transform mode7 BG1 or BG2 layer:
    if (layer & 0x80) continue;

    // color-math applies if (priority & 0x80):
    bool ce = (priority & 0x80) ? false : true;
    priority &= 0x7f;

    bool bg_enabled = true;
    bool bgsub_enabled = false;
    uint8 wt_main = 0;
    uint8 wt_sub = 0;
    if (layer <= 5) {
      bg_enabled    = regs.bg_enabled[layer];
      bgsub_enabled = regs.bgsub_enabled[layer];
      wt_main = window[layer].main[sx];
      wt_sub  = window[layer].sub[sx];
    } else {
      layer = 5;
    }

    if(bg_enabled    == true && !wt_main) {
      if(pixel_cache[sx].pri_main < priority) {
        pixel_cache[sx].pri_main = priority;
        pixel_cache[sx].bg_main  = layer;
        pixel_cache[sx].src_main = ppux_layer_col[offs];
        pixel_cache[sx].ce_main  = ce;
      }
    }
    if(bgsub_enabled == true && !wt_sub) {
      if(pixel_cache[sx].pri_sub < priority) {
        pixel_cache[sx].pri_sub = priority;
        pixel_cache[sx].bg_sub  = layer;
        pixel_cache[sx].src_sub = ppux_layer_col[offs];
        pixel_cache[sx].ce_sub  = ce;
      }
    }
  }
}

#endif
