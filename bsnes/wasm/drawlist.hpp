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
    uint16_t *data,
    unsigned pitch,
    unsigned width,
    unsigned height,
    bool interlace
  );

  uint16_t *data;
  unsigned pitch;
  unsigned width;
  unsigned height;
  bool interlace;
};

struct Context {
  Context(const Target& target);

  void draw_list(const std::vector<uint8_t>& cmdlist, const std::vector<std::shared_ptr<PixelFont::Font>> &);
  inline void draw_pixel(int16_t x0, int16_t y0, uint16_t color);
  inline void draw_hline(int16_t x0, int16_t y0, int16_t w, uint16_t color);
  inline void draw_vline(int16_t x0, int16_t y0, int16_t h, uint16_t color);

private:
  const Target& m_target;
};

}
