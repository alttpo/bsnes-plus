#ifdef PPU_CPP

void PPU::render_line_extra() {
  for(int s = 0; s < extra_count; s++) {
    struct extra_item *t = &extra_list[s];

    if(!t->enabled) continue;
    if(t->layer > 5) continue;
    if(t->priority > 12) continue;

    bool bg_enabled    = regs.bg_enabled[t->layer];
    bool bgsub_enabled = regs.bgsub_enabled[t->layer];
    if (bg_enabled == false && bgsub_enabled == false) continue;

    if(t->x > 256 && (t->x + t->width - 1) < 512) continue;
    if(line < t->y) continue;
    if(line >= t->y + t->height) continue;

    unsigned sx = t->x;
    unsigned y = (t->vflip == false) ? (line - t->y) : (t->height-1 - (line - t->y));

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
