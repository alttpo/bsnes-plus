#pragma once

namespace DrawList {

enum draw_color_kind {
  COLOR_STROKE,
  COLOR_FILL,
  COLOR_OUTLINE,

  COLOR_MAX
};

const uint16_t color_none = 0x8000;

enum draw_cmd : uint16_t {
  // commands which affect state:
  ///////////////////////////////

  // 4, CMD_TARGET, draw_layer, pre_mode7_transform, priority
  CMD_TARGET = 1,
  CMD_COLOR_DIRECT_BGR555,
  CMD_COLOR_DIRECT_RGB888,
  CMD_COLOR_PALETTED,
  CMD_FONT_SELECT,

  // commands which use state:
  ///////////////////////////////
  CMD_TEXT_UTF8 = 0x40,
  CMD_PIXEL,
  CMD_HLINE,
  CMD_VLINE,
  CMD_LINE,
  CMD_RECT,
  CMD_RECT_FILL,

  // commands which ignore state:
  ///////////////////////////////
  CMD_VRAM_TILE = 0x80,
  CMD_IMAGE,
};

enum draw_layer : uint16_t {
  BG1 = 0,
  BG2 = 1,
  BG3 = 2,
  BG4 = 3,
  OAM = 4,
  BACK = 5,
  COL = 5
};

inline bool is_color_visible(uint16_t c) { return c < 0x8000; }

template<unsigned width, unsigned height>
inline bool bounds_check(int x, int y) {
  if (y < 0) return false;
  if (y >= height) return false;
  if (x < 0) return false;
  if (x >= width) return false;
  return true;
}

typedef std::function<void(int x, int y)> plot;
typedef std::function<void(int x, int y, uint16_t color)> PlotBGR555;

struct FontContainer {
  void load_pcf(int fontindex, const uint8_t* pcf_data, int pcf_size);

  void clear();
  void erase(int fontindex);
  int size() const;

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

  uint8_t* vram_data() final;
  const uint32_t vram_size() const final;

  uint8_t* cgram_data() final;
  const uint32_t cgram_size() const final;
};

struct ExtraSpace : public Space {
  ExtraSpace();

  uint8_t* vram_data() final;
  const uint32_t vram_size() const final;

  uint8_t* cgram_data() final;
  const uint32_t cgram_size() const final;

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

struct BaseTarget {
  BaseTarget(unsigned p_width,
             unsigned p_height);

  inline virtual void px(int x, int y, uint16_t color) {};

  inline bool in_bounds(int x, int y);

  inline void draw_pixel(int x, int y, uint16_t color);
  inline void draw_rect_fill(int16_t x0, int16_t y0, int16_t w, int16_t h, uint16_t fill_color);
  inline void draw_hline(int x, int y, int w, const plot& px);
  inline void draw_vline(int x, int y, int h, const plot& px);
  inline void draw_line(int x1, int y1, int x2, int y2, const plot& px);

  void draw_outlined_stroked(uint16_t stroke_color, uint16_t outline_color, const std::function<void(const plot& plot)>& draw);

  uint16_t* draw_image(int16_t x0, int16_t y0, int16_t w, int16_t h, uint16_t* d);

  template<unsigned bpp>
  void draw_vram_tile(
    int16_t x0, int16_t y0,
    bool hflip, bool vflip,

    uint16_t vram_addr,
    uint8_t  palette,

    uint16_t twidth,
    uint16_t theight,

    uint8_t* vram,
    uint8_t* cgram
  );

public:
  unsigned width;
  unsigned height;
};

struct Renderer {
  virtual void draw_pixel(int x, int y, uint16_t color) = 0;
  virtual void draw_hline(int x, int y, int w, uint16_t color) = 0;
  virtual void draw_vline(int x, int y, int h, uint16_t color) = 0;
  virtual void draw_rect(int x0, int y0, int w, int h, uint16_t color) = 0;
  virtual void draw_rect_fill(int x0, int y0, int w, int h, uint16_t color) = 0;
  virtual void draw_line(int x1, int y1, int x2, int y2, uint16_t color) = 0;
};

template<unsigned width, unsigned height, typename PLOT>
void draw_pixel(int x0, int y0, uint16_t color, PLOT plot) {
  if (!is_color_visible(color))
    return;

  if (!bounds_check<width, height>(x0, y0))
    return;

  plot(x0, y0, color);
}

template<unsigned width, unsigned height, typename PLOT>
void draw_hline(int x0, int y0, int w, uint16_t color, PLOT plot) {
  if (!is_color_visible(color))
    return;

  if (y0 < 0) return;
  if (y0 >= height) return;

  for (int x = x0; x < x0+w; x++) {
    if (x < 0) continue;
    if (x >= width) return;

    plot(x, y0, color);
  }
}

template<unsigned width, unsigned height, typename PLOT>
void draw_vline(int x0, int y0, int h, uint16_t color, PLOT plot) {
  if (!is_color_visible(color))
    return;

  if (x0 < 0) return;
  if (x0 >= width) return;

  for (int y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= height) break;

    plot(x0, y, color);
  }
}

template<unsigned width, unsigned height, typename PLOT>
void draw_rect(int x0, int y0, int w, int h, uint16_t color, PLOT plot) {
  if (!is_color_visible(color))
    return;

  draw_hline<width, height>(x0,     y0,     w,   color, plot);
  draw_hline<width, height>(x0,     y0+h-1, w,   color, plot);
  draw_vline<width, height>(x0,     y0+1,   h-2, color, plot);
  draw_vline<width, height>(x0+w-1, y0+1,   h-2, color, plot);
}

template<unsigned width, unsigned height, typename PLOT>
void draw_rect_fill(int x0, int y0, int w, int h, uint16_t fill_color, PLOT plot) {
  if (!is_color_visible(fill_color))
    return;

  for (int y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= height) break;

    for (int x = x0; x < x0+w; x++) {
      if (x < 0) continue;
      if (x >= width) break;

      plot(x, y, fill_color);
    }
  }
}

template<unsigned width, unsigned height, typename PLOT>
void draw_line(int x1, int y1, int x2, int y2, uint16_t color, PLOT plot) {
  if (!is_color_visible(color))
    return;

  int x, y, dx, dy, dx1, dy1, mx, my, xe, ye, i;

  dx = x2 - x1;
  dy = y2 - y1;
  dx1 = std::abs(dx);
  dy1 = std::abs(dy);
  mx = 2 * dy1 - dx1;
  my = 2 * dx1 - dy1;

  if (dy1 <= dx1) {
    if (dx >= 0) {
      x = x1;
      y = y1;
      xe = x2;
    } else {
      x = x2;
      y = y2;
      xe = x1;
    }

    if (bounds_check<width, height>(x, y)) {
      plot(x, y, color);
    }

    for (i = 0; x < xe; i++) {
      x = x + 1;
      if (mx < 0) {
        mx = mx + 2 * dy1;
      } else {
        if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
          y = y + 1;
        } else {
          y = y - 1;
        }
        mx = mx + 2 * (dy1 - dx1);
      }

      if (x < 0) continue;
      if (y < 0) continue;
      if (x >= width) break;
      if (y >= height) break;
      plot(x, y, color);
    }
  } else {
    if (dy >= 0) {
      x = x1;
      y = y1;
      ye = y2;
    } else {
      x = x2;
      y = y2;
      ye = y1;
    }

    if (bounds_check<width, height>(x, y)) {
      plot(x, y, color);
    }

    for (i = 0; y < ye; i++) {
      y = y + 1;
      if (my <= 0) {
        my = my + 2 * dx1;
      } else {
        if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
          x = x + 1;
        } else {
          x = x - 1;
        }
        my = my + 2 * (dx1 - dy1);
      }

      if (x < 0) continue;
      if (y < 0) continue;
      if (x >= width) break;
      if (y >= height) break;
      plot(x, y, color);
    }
  }
}

typedef std::function<void(draw_layer i_layer, bool i_pre_mode7_transform, uint8_t i_priority, std::shared_ptr<BaseTarget>& o_target)> ChangeTarget;

typedef std::function<void(draw_layer i_layer, bool i_pre_mode7_transform, uint8_t i_priority, std::shared_ptr<Renderer>& o_target)> ChooseRenderer;

struct Context {
  Context(
    const ChangeTarget& changeTarget,
    const ChooseRenderer& chooseRenderer,
    const std::shared_ptr<FontContainer>& fonts,
    const std::shared_ptr<SpaceContainer>& spaces
  );

  void draw_list(const std::vector<uint8_t>& cmdlist);

private:
  std::shared_ptr<BaseTarget> m_target;
  const ChangeTarget& m_changeTarget;

  std::shared_ptr<Renderer> m_renderer;
  const ChooseRenderer& m_chooseRenderer;

  const std::shared_ptr<FontContainer> m_fonts;
  const std::shared_ptr<SpaceContainer> m_spaces;
};

}
