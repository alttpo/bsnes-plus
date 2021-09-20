#ifdef PPU_CPP

uint8* PPU::ppux_get_oam() {
  return memory::oam.data();
}

void PPU::ppux_draw_list_reset() {
  ppux_draw_lists.clear();
}

void PPU::ppux_render_frame_pre() {
  // extra sprites drawn on pre-transformed mode7 BG1 or BG2 layers:
  for(int i = 0; i < 1024*1024; i++) {
    ppux_mode7_col[0][i] = 0xffff;
    ppux_mode7_col[1][i] = 0xffff;
  }

  // pre-render ppux draw_lists to frame buffers:
  memset(ppux_layer_pri, 0xFF, sizeof(ppux_layer_pri));
  for (const auto& dl : ppux_draw_lists) {
    std::function<void(int x, int y, uint16_t color)> px;

    // select pixel-drawing function:
    uint8 layer = dl.layer & 0x7f;
    uint8 priority = dl.priority & 0x7f;
    int width = 256, height = 256;
    if (dl.layer & 0x80) {
      width = 1024;
      height = 1024;
      px = [=](int x, int y, uint16_t color) {
        // draw to mode7 pre-transform BG1 (layer=0x80) or BG2 (layer=0x81):
        if (layer > 1) return;

        auto offs = (y << 10) + x;
        ppux_mode7_col[layer][offs] = color;
        // TODO: capture priority from (dl.priority & 0x7f)
      };
    } else {
      px = [=](int x, int y, uint16_t color) {
        // draw to any PPU layer:
        auto offs = (y << 8) + x;
        if ((ppux_layer_pri[offs] == 0xFF) || ((ppux_layer_pri[offs]&0x7F) <= priority)) {
          ppux_layer_pri[offs] = dl.priority;
          ppux_layer_lyr[offs] = layer;
          ppux_layer_col[offs] = color;
        }
      };
    }

    DrawList::Target  target(width, height, px);
    DrawList::Context context(target, *dl.fonts, *dl.spaces, *dl.za);

    context.draw_list(dl.cmdlist);
  }
}

void PPU::ppux_mode7_fetch(int32 px, int32 py, int32 tile, unsigned layer, int32 &palette, uint16& color) {
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
