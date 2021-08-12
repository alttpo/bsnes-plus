
namespace DrawList {

Target::Target(
  unsigned p_width,
  unsigned p_height,
  const std::function<void(int x, int y, uint16_t color)>& p_px
) : width(p_width),
    height(p_height),
    px(p_px)
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

        if (outlinecolor < 0x8000) {
          font.draw_text_utf8((uint8_t*)d, x0, y0, [=](int rx, int ry) {
            draw_hline(rx-1, ry-1, 3, outlinecolor);
            draw_pixel(rx-1, ry, outlinecolor);
            draw_pixel(rx+1, ry, outlinecolor);
            draw_hline(rx-1, ry+1, 3, outlinecolor);
          });
        }
        if (color < 0x8000) {
          font.draw_text_utf8((uint8_t*)d, x0, y0, [=](int rx, int ry) {
            draw_pixel(rx, ry, color);
          });
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
