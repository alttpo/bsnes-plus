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
  // commands which ignore state:
  CMD_VRAM_TILE,
  CMD_IMAGE,
  // commands which affect state:
  CMD_COLOR_DIRECT_BGR555,
  CMD_COLOR_DIRECT_RGB888,
  CMD_COLOR_PALETTED,
  CMD_FONT_SELECT,
  CMD_FONT_CREATE_PCF,
  CMD_FONT_DELETE,
  // commands which use state:
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

struct FontContainer {
  void load_pcf(int fontindex, const uint8_t* pcf_data, int pcf_size);

  void clear();
  void erase(int fontindex);

  std::shared_ptr<PixelFont::Font> operator[](int fontindex) const;

private:
  std::vector<std::shared_ptr<PixelFont::Font>> m_fonts;
};

struct Space {
  virtual uint8_t* vram_data() = 0;
  virtual const uint32_t vram_size() const = 0;

  virtual uint8_t* cgram_data() = 0;
  virtual const uint32_t cgram_size() const = 0;
};

struct LocalSpace : public Space {
  LocalSpace();

  uint8_t* vram_data();
  const uint32_t vram_size() const;

  uint8_t* cgram_data();
  const uint32_t cgram_size() const;
};

struct ExtraSpace : public Space {
  ExtraSpace();

  uint8_t* vram_data();
  const uint32_t vram_size() const;

  uint8_t* cgram_data();
  const uint32_t cgram_size() const;

private:
  uint8_t vram[0x10000];
  uint8_t cgram[0x200];
};

struct SpaceContainer {
  static const int MaxCount = 1024;

  SpaceContainer();

  void reset();

  std::shared_ptr<Space> operator[](int index);

  uint8_t* get_vram_space(int index);
  uint8_t* get_cgram_space(int index);

private:
  std::shared_ptr<Space> m_localSpace;
  std::vector<std::shared_ptr<Space>> m_spaces;
};

struct Target {
  explicit Target(
    unsigned p_width,
    unsigned p_height,
    const std::function<void(int x, int y, uint16_t color)>& p_px
  );

  unsigned width;
  unsigned height;
  const std::function<void(int x, int y, uint16_t color)> px;;
};

struct Context {
  Context(const Target& target, FontContainer& fonts, SpaceContainer& spaces);

  void draw_list(const std::vector<uint8_t>& cmdlist);
  inline void draw_pixel(int x, int y, uint16_t color);

  inline void draw_hline(int x, int y, int w, const plot& px);
  inline void draw_vline(int x, int y, int h, const plot& px);
  inline void draw_line(int x1, int y1, int x2, int y2, const plot& px);

  inline bool in_bounds(int x, int y);

private:
  const Target& m_target;
  FontContainer& m_fonts;
  SpaceContainer& m_spaces;
};

}
