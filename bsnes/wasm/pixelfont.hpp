#pragma once

namespace PixelFont {

struct Glyph {
  uint8_t               m_width;
  std::vector<uint32_t> m_bitmapdata;
};

struct Index {
  Index() = default;
  Index(
    uint32_t glyphIndex,
    uint32_t minCodePoint,
    uint32_t maxCodePoint
  );
  explicit Index(uint32_t minCodePoint);

  uint32_t m_glyphIndex;
  uint32_t m_minCodePoint;
  uint32_t m_maxCodePoint;
};

#include "utf8decoder.cpp"

struct Font {
  Font(
    const std::vector<Glyph>& glyphs,
    const std::vector<Index>& index,
    int height,
    int kmax
  );

  uint32_t find_glyph(uint32_t codePoint) const;

  template<typename PLOT>
  bool draw_glyph(uint8_t& width, uint8_t& height, uint32_t codePoint, uint16_t color, PLOT plot) {
    auto glyphIndex = find_glyph(codePoint);
    if (glyphIndex == UINT32_MAX) {
      return false;
    }

    const auto& g = m_glyphs[glyphIndex];
    auto b = (const uint32_t *)g.m_bitmapdata.data();

    width = g.m_width;
    height = m_height;
    for (int y = 0; y < height; y++, b++) {
      uint32_t bits = *b;

      int x = 0;
      for (int k = m_kmax; k >= 0; k--, x++) {
        if (m_kmax - k > g.m_width) {
          continue;
        }

        if (bits & (1<<k)) {
          plot(x, y, color);
        }
      }
    }

    return true;
  }

  template<typename PLOT>
  void draw_text_utf8(uint8_t* s, uint16_t len, int x0, int y0, uint16_t color, PLOT plot) {
    uint8_t gw, gh;

    uint32_t codepoint = 0;
    uint32_t state = 0;

    for (unsigned n = 0; n < len; ++s, ++n) {
      uint8_t c = *s;
      if (c == 0) break;

      if (decode(&state, &codepoint, c)) {
        continue;
      }

      // have code point:
      //printf("U+%04x\n", codepoint);
      draw_glyph(gw, gh, codepoint, color, [&](int rx, int ry, uint16_t color) { plot(x0+rx, y0+ry, color); });

      x0 += gw;
    }

    //if (state != UTF8_ACCEPT) {
    //  printf("The string is not well-formed\n");
    //}
  }

  std::vector<Glyph>  m_glyphs;
  std::vector<Index>  m_index;
  int m_height;
  int m_kmax;
};

}
