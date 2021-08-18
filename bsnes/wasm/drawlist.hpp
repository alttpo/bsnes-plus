#pragma once

namespace DrawList {

enum draw_color_kind {
  COLOR_STROKE,
  COLOR_FILL,
  COLOR_OUTLINE,

  COLOR_MAX
};

const uint16_t color_none = 0x8000;

enum draw_cmd {
  // commands which ignore color state:
  CMD_VRAM_TILE,
  CMD_IMAGE,
  // commands which affect color state:
  CMD_SET_COLOR,
  CMD_SET_COLOR_PAL,
  // commands which read color state:
  CMD_TEXT_UTF8,
  CMD_PIXEL,
  CMD_HLINE,
  CMD_VLINE,
  CMD_LINE,
  CMD_RECT,
  CMD_RECT_FILL
};

inline bool is_color_visible(uint16_t c);

typedef std::function<void(int x, int y)> plot;

struct Target {
  explicit Target(
    unsigned p_width,
    unsigned p_height,
    const std::function<void(int x, int y, uint16_t color)>& p_px,
    const std::function<uint8_t*(int space)>& p_get_vram_space,
    const std::function<uint8_t*(int space)>& p_get_cgram_space
  );

  unsigned width;
  unsigned height;
  const std::function<void(int x, int y, uint16_t color)> px;
  const std::function<uint8_t*(int space)> get_vram_space;
  const std::function<uint8_t*(int space)> get_cgram_space;
};

struct Context {
  Context(const Target& target);

  void draw_list(const std::vector<uint8_t>& cmdlist, const std::vector<std::shared_ptr<PixelFont::Font>>& fonts);
  inline void draw_pixel(int x, int y, uint16_t color);

  inline void draw_hline(int x, int y, int w, const plot& px);
  inline void draw_vline(int x, int y, int h, const plot& px);
  inline void draw_line(int x1, int y1, int x2, int y2, const plot& px);

private:
  const Target& m_target;
};

}
