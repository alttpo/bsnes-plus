#pragma once

namespace DrawList {

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

struct Renderer {
  virtual void set_stroke_color(uint16_t color) = 0;
  virtual void set_outline_color(uint16_t color) = 0;
  virtual void set_fill_color(uint16_t color) = 0;

  virtual void draw_pixel(int x, int y) = 0;
  virtual void draw_hline(int x, int y, int w) = 0;
  virtual void draw_vline(int x, int y, int h) = 0;
  virtual void draw_rect(int x0, int y0, int w, int h) = 0;
  virtual void draw_rect_fill(int x0, int y0, int w, int h) = 0;
  virtual void draw_line(int x1, int y1, int x2, int y2) = 0;
  virtual void draw_text_utf8(uint8_t* s, uint16_t len, PixelFont::Font& font, int x0, int y0, text_alignment align) = 0;

  virtual uint16_t* draw_image(int x0, int y0, int w, int h, uint16_t* d) = 0;
  virtual void draw_vram_tile(int x0, int y0, int w, int h, bool hflip, bool vflip, uint8_t bpp, uint16_t vram_addr, uint8_t palette, uint8_t* vram, uint8_t* cgram) = 0;
};

typedef std::function<void(draw_layer i_layer, bool i_pre_mode7_transform, uint8_t i_priority, std::shared_ptr<Renderer>& o_target)> ChooseRenderer;

struct Context {
  Context(
    const ChooseRenderer& chooseRenderer,
    const std::shared_ptr<FontContainer>& fonts,
    const std::shared_ptr<SpaceContainer>& spaces
  );

  void draw_list(const std::vector<uint16_t>& cmdlist);

private:
  std::shared_ptr<Renderer> m_renderer;
  const ChooseRenderer& m_chooseRenderer;

  const std::shared_ptr<FontContainer> m_fonts;
  const std::shared_ptr<SpaceContainer> m_spaces;
};

}
