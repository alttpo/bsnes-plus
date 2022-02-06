#pragma once

namespace DrawList {

inline bool is_color_visible(uint16_t c) { return c < 0x8000; }

template<unsigned width, unsigned height>
inline bool bounds_check(int x, int y) {
  if (y < 0) return false;
  if (y >= height) return false;
  if (x < 0) return false;
  if (x >= width) return false;
  return true;
}

template<unsigned width, unsigned height, typename PLOT>
void draw_pixel(int x0, int y0, uint16_t color, PLOT plot) {
  if (!is_color_visible(color))
    return;

  if (!bounds_check<width, height>(x0, y0))
    return;

  plot(x0, y0, color);
}

template<unsigned width, unsigned height, typename PLOT>
void draw_hline(int x0, int y0, int w, uint16_t color, PLOT plot) {
  if (!is_color_visible(color))
    return;

  if (y0 < 0) return;
  if (y0 >= height) return;

  for (int x = x0; x < x0+w; x++) {
    if (x < 0) continue;
    if (x >= width) return;

    plot(x, y0, color);
  }
}

template<unsigned width, unsigned height, typename PLOT>
void draw_vline(int x0, int y0, int h, uint16_t color, PLOT plot) {
  if (!is_color_visible(color))
    return;

  if (x0 < 0) return;
  if (x0 >= width) return;

  for (int y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= height) break;

    plot(x0, y, color);
  }
}

template<unsigned width, unsigned height, typename PLOT>
void draw_rect(int x0, int y0, int w, int h, uint16_t color, PLOT plot) {
  if (!is_color_visible(color))
    return;

  draw_hline<width, height>(x0,     y0,     w,   color, plot);
  draw_hline<width, height>(x0,     y0+h-1, w,   color, plot);
  draw_vline<width, height>(x0,     y0+1,   h-2, color, plot);
  draw_vline<width, height>(x0+w-1, y0+1,   h-2, color, plot);
}

template<unsigned width, unsigned height, typename PLOT>
void draw_rect_fill(int x0, int y0, int w, int h, uint16_t fill_color, PLOT plot) {
  if (!is_color_visible(fill_color))
    return;

  for (int y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= height) break;

    for (int x = x0; x < x0+w; x++) {
      if (x < 0) continue;
      if (x >= width) break;

      plot(x, y, fill_color);
    }
  }
}

template<unsigned width, unsigned height, typename PLOT>
void draw_line(int x1, int y1, int x2, int y2, uint16_t color, PLOT plot) {
  if (!is_color_visible(color))
    return;

  int x, y, dx, dy, dx1, dy1, mx, my, xe, ye, i;

  dx = x2 - x1;
  dy = y2 - y1;
  dx1 = std::abs(dx);
  dy1 = std::abs(dy);
  mx = 2 * dy1 - dx1;
  my = 2 * dx1 - dy1;

  if (dy1 <= dx1) {
    if (dx >= 0) {
      x = x1;
      y = y1;
      xe = x2;
    } else {
      x = x2;
      y = y2;
      xe = x1;
    }

    if (bounds_check<width, height>(x, y)) {
      plot(x, y, color);
    }

    for (i = 0; x < xe; i++) {
      x = x + 1;
      if (mx < 0) {
        mx = mx + 2 * dy1;
      } else {
        if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
          y = y + 1;
        } else {
          y = y - 1;
        }
        mx = mx + 2 * (dy1 - dx1);
      }

      if (x < 0) continue;
      if (y < 0) continue;
      if (x >= width) break;
      if (y >= height) break;
      plot(x, y, color);
    }
  } else {
    if (dy >= 0) {
      x = x1;
      y = y1;
      ye = y2;
    } else {
      x = x2;
      y = y2;
      ye = y1;
    }

    if (bounds_check<width, height>(x, y)) {
      plot(x, y, color);
    }

    for (i = 0; y < ye; i++) {
      y = y + 1;
      if (my <= 0) {
        my = my + 2 * dx1;
      } else {
        if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
          x = x + 1;
        } else {
          x = x - 1;
        }
        my = my + 2 * (dx1 - dy1);
      }

      if (x < 0) continue;
      if (y < 0) continue;
      if (x >= width) break;
      if (y >= height) break;
      plot(x, y, color);
    }
  }
}

template<unsigned width, unsigned height, typename PLOT>
uint16_t* draw_image(int x0, int y0, int w, int h, uint16_t* d, PLOT plot) {
  for (int y = y0; y < y0+h; y++) {
    if (y < 0) {
      d += w;
      continue;
    }
    if (y >= height) {
      d += w;
      continue;
    }

    for (int x = x0; x < x0+w; x++, d++) {
      if (x < 0) continue;
      if (x >= width) continue;

      uint16_t c = *d;
      if (!is_color_visible(c))
        continue;

      plot(x, y, c);
    }
  }

  return d;
}

template<unsigned bpp, bool hflip, bool vflip, typename PLOT>
void draw_vram_tile(
  int x0, int y0,
  int w, int h,

  uint16_t vram_addr,
  uint8_t  palette,

  uint8_t* vram,
  uint8_t* cgram,

  PLOT plot
) {
  // draw tile:
  unsigned sy = y0;
  for (unsigned ty = 0; ty < h; ty++, sy++) {
    sy &= 255;

    unsigned sx = x0;
    unsigned y = (vflip == false) ? (ty) : (h - 1 - ty);

    for(unsigned tx = 0; tx < w; tx++, sx++) {
      sx &= 511;
      if(sx >= 256) continue;

      unsigned x = ((hflip == false) ? tx : (w - 1 - tx));

      uint8_t col, d0, d1, d2, d3, d4, d5, d6, d7;
      uint8_t mask = 1 << (7-(x&7));
      uint8_t *tile_ptr = vram + vram_addr;

      switch (bpp) {
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
        default:
          // TODO: warn
          break;
      }

      // color 0 is always transparent:
      if (col == 0)
        continue;

      col += palette;

      // look up color in cgram:
      uint16_t bgr = *(cgram + (col<<1)) + (*(cgram + (col<<1) + 1) << 8);

      plot(sx, sy, bgr);
    }
  }
}

template<unsigned width, unsigned height, typename PLOT>
struct Outliner {
  Outliner(PLOT& p_plot) : plot(p_plot) {}

  PLOT& plot;

  void operator() (int x, int y, uint16_t color) {
    if (DrawList::bounds_check<width, height>(x-1, y-1)) {
      plot(x-1, y-1, color);
    }
    if (DrawList::bounds_check<width, height>(x+0, y-1)) {
      plot(x+0, y-1, color);
    }
    if (DrawList::bounds_check<width, height>(x+1, y-1)) {
      plot(x+1, y-1, color);
    }

    if (DrawList::bounds_check<width, height>(x-1, y+0)) {
      plot(x-1, y+0, color);
    }
    if (DrawList::bounds_check<width, height>(x+1, y+0)) {
      plot(x+1, y+0, color);
    }

    if (DrawList::bounds_check<width, height>(x-1, y+1)) {
      plot(x-1, y+1, color);
    }
    if (DrawList::bounds_check<width, height>(x+0, y+1)) {
      plot(x+0, y+1, color);
    }
    if (DrawList::bounds_check<width, height>(x+1, y+1)) {
      plot(x+1, y+1, color);
    }
  }
};

template<unsigned width, unsigned height, typename PLOT>
struct GenericRenderer : public DrawList::Renderer {
  GenericRenderer(DrawList::draw_layer p_layer, uint8_t p_priority)
    : layer(p_layer), priority(p_priority)
  {
  }

  DrawList::draw_layer layer;
  uint8_t priority;
  uint16_t stroke_color, outline_color, fill_color;

  void set_stroke_color(uint16_t color) override {
    stroke_color = color;
  }
  void set_outline_color(uint16_t color) override {
    outline_color = color;
  }
  void set_fill_color(uint16_t color) override {
    fill_color = color;
  }

  void draw_pixel(int x0, int y0) override {
    PLOT plot(layer, priority);
    DrawList::draw_pixel<width, height>(x0, y0, outline_color, Outliner<width, height, PLOT>(plot));
    DrawList::draw_pixel<width, height>(x0, y0, stroke_color, plot);
  }

  void draw_hline(int x0, int y0, int w) override {
    PLOT plot(layer, priority);
    DrawList::draw_hline<width, height>(x0, y0, w, outline_color, Outliner<width, height, PLOT>(plot));
    DrawList::draw_hline<width, height>(x0, y0, w, stroke_color, plot);
  }

  void draw_vline(int x0, int y0, int h) override {
    PLOT plot(layer, priority);
    DrawList::draw_vline<width, height>(x0, y0, h, outline_color, Outliner<width, height, PLOT>(plot));
    DrawList::draw_vline<width, height>(x0, y0, h, stroke_color, plot);
  }

  void draw_rect(int x0, int y0, int w, int h) override {
    PLOT plot(layer, priority);
    DrawList::draw_rect<width, height>(x0, y0, w, h, outline_color, Outliner<width, height, PLOT>(plot));
    DrawList::draw_rect<width, height>(x0, y0, w, h, stroke_color, plot);
  }

  void draw_rect_fill(int x0, int y0, int w, int h) override {
    PLOT plot(layer, priority);
    DrawList::draw_rect_fill<width, height>(x0, y0, w, h, fill_color, plot);
  }

  void draw_line(int x0, int y0, int x1, int y1) override {
    PLOT plot(layer, priority);
    DrawList::draw_line<width, height>(x0, y0, x1, y1, outline_color, Outliner<width, height, PLOT>(plot));
    DrawList::draw_line<width, height>(x0, y0, x1, y1, stroke_color, plot);
  }

  void draw_text_utf8(uint8_t* s, uint16_t len, PixelFont::Font& font, int x0, int y0) override {
    PLOT plot(layer, priority);
    font.draw_text_utf8<width, height>(s, len, x0, y0, outline_color, Outliner<width, height, PLOT>(plot));
    font.draw_text_utf8<width, height>(s, len, x0, y0, stroke_color, plot);
  }


  uint16_t* draw_image(int x0, int y0, int w, int h, uint16_t* d) override {
    PLOT plot(layer, priority);
    return DrawList::draw_image<width, height>(x0, y0, w, h, d, plot);
  }

  void draw_vram_tile(int x0, int y0, int w, int h, bool hflip, bool vflip, uint8_t bpp, uint16_t vram_addr, uint8_t palette, uint8_t* vram, uint8_t* cgram) override {
    PLOT plot(layer, priority);
    switch (bpp) {
      case 4:
        if (!hflip && !vflip)
          DrawList::draw_vram_tile<4, false, false>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        else if (hflip && !vflip)
          DrawList::draw_vram_tile<4, true, false>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        else if (!hflip && vflip)
          DrawList::draw_vram_tile<4, false, true>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        else if (hflip && vflip)
          DrawList::draw_vram_tile<4, true, true>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        break;
      case 2:
        if (!hflip && !vflip)
          DrawList::draw_vram_tile<2, false, false>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        else if (hflip && !vflip)
          DrawList::draw_vram_tile<2, true, false>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        else if (!hflip && vflip)
          DrawList::draw_vram_tile<2, false, true>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        else if (hflip && vflip)
          DrawList::draw_vram_tile<2, true, true>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        break;
      case 8:
        if (!hflip && !vflip)
          DrawList::draw_vram_tile<8, false, false>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        else if (hflip && !vflip)
          DrawList::draw_vram_tile<8, true, false>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        else if (!hflip && vflip)
          DrawList::draw_vram_tile<8, false, true>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        else if (hflip && vflip)
          DrawList::draw_vram_tile<8, true, true>(x0, y0, w, h, vram_addr, palette, vram, cgram, plot);
        break;
    }
  }
};

}
