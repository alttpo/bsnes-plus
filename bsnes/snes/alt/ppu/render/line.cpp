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

uint8* PPU::get_extra_vram(uint8 extra) {
  if (extra == 0) {
    return memory::vram.data();
  } else if (extra < 128) {
    StaticRAM *ram = extra_vram[extra-1];
    if (!ram) {
      // allocate on demand:
      extra_vram[extra-1] = ram = new StaticRAM(0x10000);
    }
    return ram->data();
  } else {
    return nullptr;
  }
}

uint8* PPU::get_extra_cgram(uint8 extra) {
  if (extra == 0) {
    return memory::cgram.data();
  } else if (extra < 128) {
    StaticRAM *ram = extra_cgram[extra-1];
    if (!ram) {
      // allocate on demand:
      extra_cgram[extra-1] = ram = new StaticRAM(0x200);
    }
    return ram->data();
  } else {
    return nullptr;
  }
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
    unsigned y = (t->vflip == false) ? (line - t->y) : (t->height-1 - (line - t->y));

    uint8 *vram = get_extra_vram(t->extra);
    if (!vram) continue;

    uint8 *cgram = get_extra_cgram(t->extra);
    if(!cgram) continue;

    vram += t->vram_addr << 1;
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
