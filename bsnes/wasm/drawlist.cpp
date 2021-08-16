
namespace DrawList {

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

  while ((p - start) < end) {
    // every command starts with the number of 16-bit words in length, including command, arguments, and inline data:
    uint16_t len = *p++;

    uint16_t* d = p;

    uint16_t cmd = *d++;

    uint16_t color, fillcolor, outlinecolor;
    int16_t  x0, y0, w, h;
    switch (cmd) {
      case CMD_PIXEL:
        color = *d++;
        if (color >= 0x8000) {
          break;
        }

        x0 = *d++;
        y0 = *d++;

        draw_pixel(x0, y0, color);
        break;
      case CMD_IMAGE:
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

            color = *d++;
            if (color >= 0x8000) continue;

            draw_pixel(x, y, color);
          }
        }
        break;
      case CMD_HLINE:
        color = *d++;
        if (color >= 0x8000) {
          break;
        }

        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        w  = (int16_t)*d++;

        draw_hline(x0, y0, w, color);
        break;
      case CMD_VLINE:
        color = *d++;
        if (color >= 0x8000) {
          break;
        }

        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        h  = (int16_t)*d++;

        draw_vline(x0, y0, h, color);
        break;
      case CMD_RECT:
        color = *d++;
        fillcolor = *d++;

        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        w  = (int16_t)*d++;
        h  = (int16_t)*d++;

        if (fillcolor < 0x8000) {
          for (int16_t y = y0; y < y0+h; y++) {
            if (y < 0) continue;
            if (y >= m_target.height) break;

            for (int16_t x = x0; x < x0+w; x++) {
              if (x < 0) continue;
              if (x >= m_target.width) break;

              m_target.px(x, y, fillcolor);
            }
          }
        }
        if (color < 0x8000) {
          draw_hline(x0, y0, w, color);
          draw_hline(x0, y0+h-1, w, color);
          draw_vline(x0, y0, h, color);
          draw_vline(x0+w-1, y0, h, color);
        }
        break;
      case CMD_TEXT_UTF8: {
        // stroke color:
        color = *d++;
        // 1px outline color:
        outlinecolor = *d++;

        // select font:
        uint16_t fontindex = *d++;
        if (fontindex >= fonts.size()) {
          break;
        }

        const auto& font = *fonts[fontindex];

        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;

        uint16_t textlen = (len - 6) << 1;

        if (outlinecolor < 0x8000) {
          font.draw_text_utf8((uint8_t*)d, textlen, x0, y0, [=](int rx, int ry) {
            draw_hline(rx-1, ry-1, 3, outlinecolor);
            draw_pixel(rx-1, ry, outlinecolor);
            draw_pixel(rx+1, ry, outlinecolor);
            draw_hline(rx-1, ry+1, 3, outlinecolor);
          });
        }
        if (color < 0x8000) {
          font.draw_text_utf8((uint8_t*)d, textlen, x0, y0, [=](int rx, int ry) {
            draw_pixel(rx, ry, color);
          });
        }

        break;
      }
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

inline void Context::draw_hline(int x0, int y0, int w, uint16_t color) {
  if (y0 < 0) return;
  if (y0 >= m_target.height) return;

  for (int x = x0; x < x0+w; x++) {
    if (x < 0) continue;
    if (x >= m_target.width) return;

    m_target.px(x, y0, color);
  }
}

inline void Context::draw_vline(int x0, int y0, int h, uint16_t color) {
  if (x0 < 0) return;
  if (x0 >= m_target.width) return;

  for (int y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= m_target.height) break;

    m_target.px(x0, y, color);
  }
}

}
