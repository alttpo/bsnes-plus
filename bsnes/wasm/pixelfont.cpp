
namespace PixelFont {

#include "utf8decoder.cpp"

Index::Index(
  uint32_t glyphIndex,
  uint32_t minCodePoint,
  uint32_t maxCodePoint
) : m_glyphIndex(glyphIndex),
    m_minCodePoint(minCodePoint),
    m_maxCodePoint(maxCodePoint)
{}

Index::Index(uint32_t codePoint) : m_maxCodePoint(codePoint) {}

Font::Font(
  const std::vector<Glyph>& glyphs,
  const std::vector<Index>& index,
  int height,
  int kmax
) : m_glyphs(glyphs),
    m_index(index),
    m_height(height),
    m_kmax(kmax)
{}

void Font::draw_text_utf8(uint8_t* s, uint16_t len, int x0, int y0, const std::function<void(int,int)>& px) const {
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
    draw_glyph(gw, gh, codepoint, [=](int rx, int ry) { px(x0+rx, y0+ry); });

    x0 += gw;
  }

  //if (state != UTF8_ACCEPT) {
  //  printf("The string is not well-formed\n");
  //}
}

bool Font::draw_glyph(uint8_t& width, uint8_t& height, uint32_t codePoint, const std::function<void(int,int)>& px) const {
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
        px(x, y);
      }
    }
  }

  return true;
}

uint32_t Font::find_glyph(uint32_t codePoint) const {
  auto it = std::lower_bound(
    m_index.begin(),
    m_index.end(),
    Index(codePoint),
    [](const Index& first, const Index& last) {
      return first.m_maxCodePoint < last.m_maxCodePoint;
    }
    );
  if (it == m_index.end()) {
    return UINT32_MAX;
  }

  uint32_t index;
  if (codePoint < it->m_minCodePoint) {
    index = UINT32_MAX;
  } else if (codePoint > it->m_maxCodePoint) {
    index = UINT32_MAX;
  } else {
    index = it->m_glyphIndex + (codePoint - it->m_minCodePoint);
  }
  //printf("%04x = %04x + (%04x - %04x)\n", index, it->m_glyphIndex, codePoint, it->m_minCodePoint);
  return index;
}

}
