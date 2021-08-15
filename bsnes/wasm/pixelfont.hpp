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

struct Font {
  Font(
    const std::vector<Glyph>& glyphs,
    const std::vector<Index>& index,
    int height,
    int kmax
  );

  bool draw_glyph(uint8_t& width, uint8_t& height, uint32_t codePoint, const std::function<void(int,int)>& px) const;
  uint32_t find_glyph(uint32_t codePoint) const;
  void draw_text_utf8(uint8_t* s, uint16_t len, int x0, int y0, const std::function<void(int,int)>& px) const;

  std::vector<Glyph>  m_glyphs;
  std::vector<Index>  m_index;
  int m_height;
  int m_kmax;
};

}
