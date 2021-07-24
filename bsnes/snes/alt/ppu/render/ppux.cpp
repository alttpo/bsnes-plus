#ifdef PPU_CPP

void PPU::ppux_sprite_reset() {
  memset(extra_list, 0, sizeof(extra_list));
}

void PPU::ppux_reset() {
  ppux_sprite_reset();
  for (unsigned i = 0; i < extra_spaces; i++) {
    if (vram_space[i]) {
      delete vram_space[i];
      vram_space[i] = nullptr;
    }
    if (cgram_space[i]) {
      delete cgram_space[i];
      cgram_space[i] = nullptr;
    }
  }
}

inline uint16 PPU::get_palette_space(uint8 space, uint8 index) {
  const unsigned addr = index << 1;
  uint8 *cgram = get_cgram_space(space);
  return cgram[addr] + (cgram[addr + 1] << 8);
}

bool PPU::ppux_is_sprite_on_scanline(struct extra_item *spr) {
  //if sprite is entirely offscreen and doesn't wrap around to the left side of the screen,
  //then it is not counted. this *should* be 256, and not 255, even though dot 256 is offscreen.
  if(spr->x > 256 && (spr->x + spr->width - 1) < 512) return false;

  int spr_height = (spr->height);
  if(line >= spr->y && line < (spr->y + spr_height)) return true;
  if((spr->y + spr_height) >= 256 && line < ((spr->y + spr_height) & 255)) return true;
  return false;
}

void PPU::ppux_render_frame_pre() {
  memset(ppux_mode7_pal, 0, sizeof(ppux_mode7_pal));
  memset(ppux_mode7_space, 0, sizeof(ppux_mode7_space));
  for(int s = 0; s < extra_count; s++) {
    struct extra_item *t = &extra_list[s];

    if(!t->enabled) continue;
    if((t->layer & 0x80) == 0) continue;

    uint8 layer = (t->layer & 0x7F);
    if(layer != BG1 && layer != BG2) continue;

    if(t->x > 1024 && (t->x + t->width - 1) < 2048) continue;

    uint8 *vram = get_vram_space(t->vram_space);
    if (!vram) continue;

    vram += t->vram_addr;
    unsigned sy = t->y;
    for(unsigned ty = 0; ty < t->height; ty++, sy++) {
      sy &= 2047;
      if(sy >= 1024) continue;

      unsigned sx = t->x;
      unsigned y = (t->vflip == false) ? (ty) : (t->height-1 - ty);

      for(unsigned tx = 0; tx < t->width; tx++, sx++) {
        sx &= 2047;
        if(sx >= 1024) continue;

        unsigned x = ((t->hflip == false) ? tx : (t->width-1 - tx));

        uint8 col, d0, d1, d2, d3, d4, d5, d6, d7;
        uint8 mask = 1 << (7-(x&7));
        uint8 *tile_ptr = vram;

        switch (t->bpp) {
        case 2:
          // 16 bytes per 8x8 tile
          tile_ptr += ((x >> 3) << 4);
          tile_ptr += ((y >> 3) << 8);
          tile_ptr += (y & 7) << 1;
          d0 = *(tile_ptr    );
          d1 = *(tile_ptr + 1);
          col  = !!(d0 & mask) << 0;
          col += !!(d1 & mask) << 1;
          break;
        case 4:
          // 32 bytes per 8x8 tile
          tile_ptr += ((x >> 3) << 5);
          tile_ptr += ((y >> 3) << 9);
          tile_ptr += (y & 7) << 1;
          d0 = *(tile_ptr     );
          d1 = *(tile_ptr +  1);
          d2 = *(tile_ptr + 16);
          d3 = *(tile_ptr + 17);
          col  = !!(d0 & mask) << 0;
          col += !!(d1 & mask) << 1;
          col += !!(d2 & mask) << 2;
          col += !!(d3 & mask) << 3;
          break;
        case 8:
          // 64 bytes per 8x8 tile
          tile_ptr += ((x >> 3) << 6);
          tile_ptr += ((y >> 3) << 10);
          tile_ptr += (y & 7) << 1;
          d0 = *(tile_ptr     );
          d1 = *(tile_ptr +  1);
          d2 = *(tile_ptr + 16);
          d3 = *(tile_ptr + 17);
          d4 = *(tile_ptr + 32);
          d5 = *(tile_ptr + 33);
          d6 = *(tile_ptr + 48);
          d7 = *(tile_ptr + 49);
          col  = !!(d0 & mask) << 0;
          col += !!(d1 & mask) << 1;
          col += !!(d2 & mask) << 2;
          col += !!(d3 & mask) << 3;
          col += !!(d4 & mask) << 4;
          col += !!(d5 & mask) << 5;
          col += !!(d6 & mask) << 6;
          col += !!(d7 & mask) << 7;
          break;
        }

        if(col == 0) continue;
        col += t->palette;

        // output:
        ppux_mode7_pal[layer][(sy << 10) + sx] = col;
        ppux_mode7_space[layer][(sy << 10) + sx] = t->cgram_space;
      }
    }
  }
}

void PPU::ppux_mode7_fetch(int32 px, int32 py, int32 tile, unsigned layer, int32 &palette, unsigned &cgramspace) {
  int32 ix = (py << 10) + px;
  palette = ppux_mode7_pal[layer][ix];
  if(palette != 0) {
    cgramspace = ppux_mode7_space[layer][ix];
    return;
  }
  palette = memory::vram[(((tile << 6) + ((py & 7) << 3) + (px & 7)) << 1) + 1];
  cgramspace = 0;
}

void PPU::ppux_render_line_pre() {
}

void PPU::ppux_render_line_post() {
  for(int s = 0; s < extra_count; s++) {
    struct extra_item *t = &extra_list[s];

    if(!t->enabled) continue;
    if(t->layer > 5) continue;
    if(t->priority > 12) continue;

    bool bg_enabled    = regs.bg_enabled[t->layer];
    bool bgsub_enabled = regs.bgsub_enabled[t->layer];
    if (bg_enabled == false && bgsub_enabled == false) continue;

    if(!ppux_is_sprite_on_scanline(t)) continue;

    unsigned sx = t->x;
    unsigned ty = (line - t->y) & 0xff;
    unsigned y = (t->vflip == false) ? (ty) : (t->height-1 - ty);

    uint8 *vram = get_vram_space(t->vram_space);
    if (!vram) continue;

    uint8 *cgram = get_cgram_space(t->cgram_space);
    if(!cgram) continue;

    uint8 *wt_main = window[t->layer].main;
    uint8 *wt_sub  = window[t->layer].sub;

    vram += t->vram_addr;
    for(unsigned tx = 0; tx < t->width; tx++, sx++) {
      sx &= 511;
      if(sx >= 256) continue;

      unsigned x = ((t->hflip == false) ? tx : (t->width-1 - tx));

      uint8 col, d0, d1, d2, d3, d4, d5, d6, d7;
      uint8 mask = 1 << (7-(x&7));
      uint8 *tile_ptr = vram;

      switch (t->bpp) {
      case 2:
        // 16 bytes per 8x8 tile
        tile_ptr += ((x >> 3) << 4);
        tile_ptr += ((y >> 3) << 8);
        tile_ptr += (y & 7) << 1;
        d0 = *(tile_ptr    );
        d1 = *(tile_ptr + 1);
        col  = !!(d0 & mask) << 0;
        col += !!(d1 & mask) << 1;
        break;
      case 4:
        // 32 bytes per 8x8 tile
        tile_ptr += ((x >> 3) << 5);
        tile_ptr += ((y >> 3) << 9);
        tile_ptr += (y & 7) << 1;
        d0 = *(tile_ptr     );
        d1 = *(tile_ptr +  1);
        d2 = *(tile_ptr + 16);
        d3 = *(tile_ptr + 17);
        col  = !!(d0 & mask) << 0;
        col += !!(d1 & mask) << 1;
        col += !!(d2 & mask) << 2;
        col += !!(d3 & mask) << 3;
        break;
      case 8:
        // 64 bytes per 8x8 tile
        tile_ptr += ((x >> 3) << 6);
        tile_ptr += ((y >> 3) << 10);
        tile_ptr += (y & 7) << 1;
        d0 = *(tile_ptr     );
        d1 = *(tile_ptr +  1);
        d2 = *(tile_ptr + 16);
        d3 = *(tile_ptr + 17);
        d4 = *(tile_ptr + 32);
        d5 = *(tile_ptr + 33);
        d6 = *(tile_ptr + 48);
        d7 = *(tile_ptr + 49);
        col  = !!(d0 & mask) << 0;
        col += !!(d1 & mask) << 1;
        col += !!(d2 & mask) << 2;
        col += !!(d3 & mask) << 3;
        col += !!(d4 & mask) << 4;
        col += !!(d5 & mask) << 5;
        col += !!(d6 & mask) << 6;
        col += !!(d7 & mask) << 7;
        break;
      }

      // color 0 is always transparent:
      if(col == 0) continue;

      col += t->palette;

      // look up color in cgram:
      uint16 bgr = *(cgram + (col<<1)) + (*(cgram + (col<<1) + 1) << 8);

      if(bg_enabled    == true && !wt_main[sx]) {
        if(pixel_cache[sx].pri_main < t->priority) {
          pixel_cache[sx].pri_main = t->priority;
          pixel_cache[sx].bg_main  = t->layer;
          pixel_cache[sx].src_main = bgr;
          pixel_cache[sx].ce_main  = t->color_exemption;
        }
      }
      if(bgsub_enabled == true && !wt_sub[sx]) {
        if(pixel_cache[sx].pri_sub < t->priority) {
          pixel_cache[sx].pri_sub = t->priority;
          pixel_cache[sx].bg_sub  = t->layer;
          pixel_cache[sx].src_sub = bgr;
          pixel_cache[sx].ce_sub  = t->color_exemption;
        }
      }
    }
  }
}

#endif
