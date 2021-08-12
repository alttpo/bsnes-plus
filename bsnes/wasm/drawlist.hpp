#pragma once

namespace DrawList {

enum draw_cmd {
  CMD_PIXEL,
  CMD_IMAGE,
  CMD_HLINE,
  CMD_VLINE,
  CMD_RECT,
  CMD_TEXT_UTF8,
  CMD_VRAM_TILE_2BPP,
  CMD_VRAM_TILE_4BPP,
  CMD_VRAM_TILE_8BPP
};

struct Target {
  explicit Target(
    unsigned width,
    unsigned height,
    const std::function<void(int x, int y, uint16_t color)>& px
  );

  const std::function<void(int x, int y, uint16_t color)>& px;
  unsigned width;
  unsigned height;
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
