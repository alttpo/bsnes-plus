#pragma once

namespace DrawList {

enum draw_cmd {
  CMD_PIXEL,
  CMD_IMAGE,
  CMD_HLINE,
  CMD_VLINE,
  CMD_RECT,
  CMD_TEXT_UTF8,
  CMD_VRAM_TILE,
};

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
  inline void draw_hline(int x, int y, int w, uint16_t color);
  inline void draw_vline(int x, int y, int h, uint16_t color);

private:
  const Target& m_target;
};

}
