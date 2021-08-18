
namespace DrawList {

inline bool is_color_visible(uint16_t c) { return c < 0x8000; }

Target::Target(
  unsigned p_width,
  unsigned p_height,
  const std::function<void(int x, int y, uint16_t color)>& p_px,
  const std::function<uint8_t*(int space)>& p_get_vram_space,
  const std::function<uint8_t*(int space)>& p_get_cgram_space
) : width(p_width),
    height(p_height),
    px(p_px),
    get_vram_space(p_get_vram_space),
    get_cgram_space(p_get_cgram_space)
{}

Context::Context(const Target& target) : m_target(target) {}

void Context::draw_list(const std::vector<uint8_t>& cmdlist, const std::vector<std::shared_ptr<PixelFont::Font>> &fonts) {
  uint16_t* start = (uint16_t*) cmdlist.data();
  uint32_t  end = cmdlist.size()>>1;
  uint16_t* p = start;

  uint16_t colorstate[COLOR_MAX] = { 0x7fff, };
  uint16_t& stroke_color  = colorstate[COLOR_STROKE];
  uint16_t& fill_color    = colorstate[COLOR_FILL];
  uint16_t& outline_color = colorstate[COLOR_OUTLINE];

  stroke_color = 0x7fff;
  fill_color = color_none;
  outline_color = color_none;

  auto draw_outlined_stroked = [&](const std::function<void(const plot& px)>& draw) {
    if (!is_color_visible(stroke_color) && !is_color_visible(outline_color)) {
      return;
    }

    // record all stroked points from the shape:
    std::set<int> stroked;
    draw([&](int x, int y) {
      if (y < 0) return;
      if (y >= m_target.height) return;
      if (x < 0) return;
      if (x >= m_target.width) return;

      stroked.emplace(y*1024+x);
    });

    // now outline all stroked points without overdraw:
    for (auto it = stroked.begin(); it != stroked.end(); it++) {
      int y = *it / 1024;
      int x = *it & 1023;
      if (is_color_visible(stroke_color)) {
        m_target.px(x, y, stroke_color);
      }

      if (is_color_visible(outline_color)) {
        if (stroked.find((y-1)*1024+(x-1)) == stroked.end()) {
          m_target.px(x-1, y-1, outline_color);
        }
        if (stroked.find((y-1)*1024+(x+0)) == stroked.end()) {
          m_target.px(x+0, y-1, outline_color);
        }
        if (stroked.find((y-1)*1024+(x+1)) == stroked.end()) {
          m_target.px(x+1, y-1, outline_color);
        }

        if (stroked.find((y+0)*1024+(x-1)) == stroked.end()) {
          m_target.px(x-1, y+0, outline_color);
        }
        if (stroked.find((y+0)*1024+(x+1)) == stroked.end()) {
          m_target.px(x+1, y+0, outline_color);
        }

        if (stroked.find((y+1)*1024+(x-1)) == stroked.end()) {
          m_target.px(x-1, y+1, outline_color);
        }
        if (stroked.find((y+1)*1024+(x+0)) == stroked.end()) {
          m_target.px(x+0, y+1, outline_color);
        }
        if (stroked.find((y+1)*1024+(x+1)) == stroked.end()) {
          m_target.px(x+1, y+1, outline_color);
        }
      }
    }
  };

  while ((p - start) < end) {
    // every command starts with the number of 16-bit words in length, including command, arguments, and inline data:
    uint16_t len = *p++;

    uint16_t* d = p;

    uint16_t cmd = *d++;

    int16_t  x0, y0, w, h;
    int16_t  x1, y1;
    switch (cmd) {
      case CMD_VRAM_TILE: {
        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        bool   hflip = (bool)*d++;
        bool   vflip = (bool)*d++;

        uint8_t  vram_space = *d++;       //     0 ..   255; 0 = local, 1..255 = extra
        uint16_t vram_addr = *d++;        // $0000 .. $FFFF (byte address)
        uint8_t  cgram_space = *d++;      //     0 ..   255; 0 = local, 1..255 = extra
        uint8_t  palette = *d++;          //     0 ..   255

        uint8_t  bpp = *d++;              // 2-, 4-, or 8-bpp tiles from vram[extra] and cgram[extra]
        uint16_t width = *d++;            // number of pixels width
        uint16_t height = *d++;           // number of pixels high

        uint8_t* vram = m_target.get_vram_space(vram_space);
        if (!vram) break;

        uint8_t* cgram = m_target.get_cgram_space(cgram_space);
        if(!cgram) break;

        // draw tile:
        unsigned sy = y0;
        for (unsigned ty = 0; ty < height; ty++, sy++) {
          sy &= 255;

          unsigned sx = x0;
          unsigned y = (vflip == false) ? (ty) : (height-1 - ty);

          for(unsigned tx = 0; tx < width; tx++, sx++) {
            sx &= 511;
            if(sx >= 256) continue;

            unsigned x = ((hflip == false) ? tx : (width-1 - tx));

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
            if(col == 0) continue;

            col += palette;

            // look up color in cgram:
            uint16_t bgr = *(cgram + (col<<1)) + (*(cgram + (col<<1) + 1) << 8);

            m_target.px(sx, sy, bgr);
          }
        }

        break;
      }
      case CMD_IMAGE: {
        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        w  = (int16_t)*d++;
        h  = (int16_t)*d++;
        for (int16_t y = y0; y < y0+h; y++) {
          if (y < 0) continue;
          if (y >= m_target.height) break;

          for (int16_t x = x0; x < x0+w; x++) {
            if (x < 0) continue;
            if (x >= m_target.width) break;

            uint16_t c = *d++;
            if (!is_color_visible(c)) continue;

            m_target.px(x, y, c);
          }
        }
        break;
      }
      case CMD_SET_COLOR: {
        uint16_t index = *d++;
        if (index >= COLOR_MAX) break;

        colorstate[index] = *d++;

        break;
      }
      case CMD_SET_COLOR_PAL: {
        uint16_t index = *d++;
        if (index >= COLOR_MAX) break;

        uint16_t space = *d++;

        uint8_t* cgram = m_target.get_cgram_space(space);
        if(!cgram) break;

        // read litle-endian 16-bit color from CGRAM:
        unsigned addr = ((uint16_t)*d++) << 1;
        colorstate[index] = cgram[addr] + (cgram[addr + 1] << 8);

        break;
      }
      case CMD_TEXT_UTF8: {
        // select font:
        uint16_t fontindex = *d++;
        if (fontindex >= fonts.size()) {
          break;
        }

        const auto& font = *fonts[fontindex];

        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        uint16_t textlen = *d++;

        draw_outlined_stroked([=](const std::function<void(int,int)>& px) {
          font.draw_text_utf8((uint8_t*)d, textlen, x0, y0, px);
        });

        break;
      }
      case CMD_PIXEL: {
        x0 = *d++;
        y0 = *d++;

        draw_outlined_stroked([=](const std::function<void(int,int)>& px) {
          px(x0, y0);
        });
        break;
      }
      case CMD_HLINE: {
        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        w  = (int16_t)*d++;

        draw_outlined_stroked([=](const plot& px) {
          draw_hline(x0, y0, w, px);
        });
        break;
      }
      case CMD_VLINE: {
        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        h  = (int16_t)*d++;

        draw_outlined_stroked([=](const plot& px) {
          draw_vline(x0, y0, h, px);
        });
        break;
      }
      case CMD_LINE: {
        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        x1 = (int16_t)*d++;
        y1 = (int16_t)*d++;

        draw_outlined_stroked([=](const plot& px) {
          draw_line(x0, y0, x1, y1, px);
        });
        break;
      }
      case CMD_RECT: {
        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        w  = (int16_t)*d++;
        h  = (int16_t)*d++;

        draw_outlined_stroked([=](const plot& px) {
          draw_hline(x0,     y0,     w,   px);
          draw_hline(x0,     y0+h-1, w,   px);
          draw_vline(x0,     y0+1,   h-2, px);
          draw_vline(x0+w-1, y0+1,   h-2, px);
        });
        break;
      }
      case CMD_RECT_FILL: {
        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        w  = (int16_t)*d++;
        h  = (int16_t)*d++;

        if (is_color_visible(fill_color)) {
          for (int16_t y = y0; y < y0+h; y++) {
            if (y < 0) continue;
            if (y >= m_target.height) break;

            for (int16_t x = x0; x < x0+w; x++) {
              if (x < 0) continue;
              if (x >= m_target.width) break;

              m_target.px(x, y, fill_color);
            }
          }
        }
        break;
      }
    }

    p += len;
  }
}

inline void Context::draw_pixel(int x0, int y0, uint16_t color) {
  if (y0 < 0) return;
  if (y0 >= m_target.height) return;
  if (x0 < 0) return;
  if (x0 >= m_target.width) return;

  m_target.px(x0, y0, color);
}

inline void Context::draw_hline(int x0, int y0, int w, const plot& px) {
  if (y0 < 0) return;
  if (y0 >= m_target.height) return;

  for (int x = x0; x < x0+w; x++) {
    if (x < 0) continue;
    if (x >= m_target.width) return;

    px(x, y0);
  }
}

inline void Context::draw_vline(int x0, int y0, int h, const plot& px) {
  if (x0 < 0) return;
  if (x0 >= m_target.width) return;

  for (int y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= m_target.height) break;

    px(x0, y);
  }
}

inline void Context::draw_line(int x1, int y1, int x2, int y2, const plot& px) {
  int m   = 2 * (y2 - y1);
  int err = m - (x2 - x1);

  int x = x1;
  int y = y1;
  for (; x <= x2; x++) {
    if (x >= 0 && x < m_target.width &&
        y >= 0 && y < m_target.height)
    {
      px(x, y);
    }

    err += m;

    if (err >= 0) {
      y++;
      err  -= 2 * (x2 - x1);
    }
  }
}


}
