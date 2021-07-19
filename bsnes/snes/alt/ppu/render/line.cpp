#ifdef PPU_CPP

inline uint16 PPU::get_palette(uint8 index) {
  const unsigned addr = index << 1;
  return memory::cgram[addr] + (memory::cgram[addr + 1] << 8);
}

//p = 00000bgr <palette data>
//t = BBGGGRRR <tilemap data>
//r = 0BBb00GGGg0RRRr0 <return data>
inline uint16 PPU::get_direct_color(uint8 p, uint8 t) {
  return ((t & 7) << 2) | ((p & 1) << 1) |
    (((t >> 3) & 7) << 7) | (((p >> 1) & 1) << 6) |
    ((t >> 6) << 13) | ((p >> 2) << 12);
}

inline uint16 PPU::get_pixel_normal(uint32 x) {
  pixel_t &p = pixel_cache[x];
  uint16 src_main, src_sub;
  uint8  bg_sub;
  src_main = p.src_main;

  if(!regs.addsub_mode) {
    bg_sub  = BACK;
    src_sub = regs.color_rgb;
  } else {
    bg_sub  = p.bg_sub;
    src_sub = p.src_sub;
  }

  if(!window[COL].main[x]) {
    if(!window[COL].sub[x]) {
      return 0x0000;
    }
    src_main = 0x0000;
  }

  if(!p.ce_main && regs.color_enabled[p.bg_main] && window[COL].sub[x]) {
    bool halve = false;
    if(regs.color_halve && window[COL].main[x]) {
      if(regs.addsub_mode && bg_sub == BACK);
      else {
        halve = true;
      }
    }
    return addsub(src_main, src_sub, halve);
  }

  return src_main;
}

inline uint16 PPU::get_pixel_swap(uint32 x) {
  pixel_t &p = pixel_cache[x];
  uint16 src_main, src_sub;
  uint8  bg_sub;
  src_main = p.src_sub;

  if(!regs.addsub_mode) {
    bg_sub  = BACK;
    src_sub = regs.color_rgb;
  } else {
    bg_sub  = p.bg_main;
    src_sub = p.src_main;
  }

  if(!window[COL].main[x]) {
    if(!window[COL].sub[x]) {
      return 0x0000;
    }
    src_main = 0x0000;
  }

  if(!p.ce_sub && regs.color_enabled[p.bg_sub] && window[COL].sub[x]) {
    bool halve = false;
    if(regs.color_halve && window[COL].main[x]) {
      if(regs.addsub_mode && bg_sub == BACK);
      else {
        halve = true;
      }
    }
    return addsub(src_main, src_sub, halve);
  }

  return src_main;
}

void PPU::render_line_extra() {
  for(int s = 0; s < 128; s++) {
    struct extra_item *t = &extra_list[s];

    if(!t->enabled) continue;
    if(t->layer > 5) continue;
    if(t->priority > 12) continue;

    bool bg_enabled    = regs.bg_enabled[t->layer];
    bool bgsub_enabled = regs.bgsub_enabled[t->layer];
    if (bg_enabled == false && bgsub_enabled == false) continue;

    uint8 *wt_main = window[t->layer].main;
    uint8 *wt_sub  = window[t->layer].sub;

    if(t->x > 256 && (t->x + t->width - 1) < 512) continue;
    if(line < t->y) continue;
    if(line >= t->y + t->height) continue;

    unsigned sx = t->x;
    unsigned y = (t->vflip == false) ? t->y : (t->height-1 - t->y);

    uint16 *tile_ptr = t->colors + (t->stride * (line - y));
    for(unsigned x = 0; x < t->width; x++, sx++) {
      sx &= 511;
      if(sx >= 256) continue;

      uint16 col = *(tile_ptr + ((t->hflip == false) ? x : (t->width-1 - x)));
      if((col&0x8000) == 0) continue;
      col &= 0x7FFF;

      if(bg_enabled    == true && !wt_main[x]) {
        if(pixel_cache[sx].pri_main < t->priority) {
          pixel_cache[sx].pri_main = t->priority;
          pixel_cache[sx].bg_main  = t->layer;
          pixel_cache[sx].src_main = col;
          pixel_cache[sx].ce_main  = t->color_exemption;
        }
      }
      if(bgsub_enabled == true && !wt_sub[x]) {
        if(pixel_cache[sx].pri_sub < t->priority) {
          pixel_cache[sx].pri_sub = t->priority;
          pixel_cache[sx].bg_sub  = t->layer;
          pixel_cache[sx].src_sub = col;
          pixel_cache[sx].ce_sub  = t->color_exemption;
        }
      }
    }
  }
}

inline void PPU::render_line_output() {
  uint16 *ptr = (uint16*)output + (line * 1024) + ((interlace() && field()) ? 512 : 0);
  uint16 *luma = light_table[regs.display_brightness];

  if(!regs.pseudo_hires && regs.bg_mode != 5 && regs.bg_mode != 6) {
    for(unsigned x = 0; x < 256; x++) {
      *ptr++ = luma[get_pixel_normal(x)];
    }
  } else {
    for(unsigned x = 0, prev = 0; x < 256; x++) {
      *ptr++ = luma[get_pixel_swap(x)];
      *ptr++ = luma[get_pixel_normal(x)];
    }
  }
}

inline void PPU::render_line_clear() {
  uint16 *ptr = (uint16*)output + (line * 1024) + ((interlace() && field()) ? 512 : 0);
  uint16 width = (!regs.pseudo_hires && regs.bg_mode != 5 && regs.bg_mode != 6) ? 256 : 512;
  memset(ptr, 0, width * 2 * sizeof(uint16));
}

#endif
