#pragma once

#include "host.hpp"

struct Glyph {
  uint8_t  m_height;
  uint8_t  m_width;
  uint32_t m_bitmapdataOffset;
};

struct Index {
  Index() = default;
  explicit Index(uint32_t minCodePoint);

  uint32_t m_glyphIndex;
  uint32_t m_minCodePoint;
  uint32_t m_maxCodePoint;
};

struct Font {
  bool draw_glyph(uint8_t& width, uint8_t& height, uint32_t codePoint, const std::function<void(int,int)>& px) const;
  uint32_t find_glyph(uint32_t codePoint) const;

  int                   m_stride;
  std::vector<uint8_t>  m_bitmapdata;
  std::vector<Glyph>    m_glyphs;
  std::vector<Index>    m_index;
};

struct WASMInterface {
  WASMInterface(WASM::Host &host);

  WASM::Host &m_host;

public:
  void on_nmi();
  const uint16_t *on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace);

public:
  void register_debugger(const std::function<void()>& do_break, const std::function<void()>& do_continue);

  std::function<void()> m_do_break;
  std::function<void()> m_do_continue;

public:
  struct {
    const uint16_t *data;
    unsigned pitch;
    unsigned width;
    unsigned height;
    bool interlace;
  } frame;

public:
  // link functions:
  void link_module(const std::shared_ptr<WASM::Module>& module);

  // wasm bindings:
#define decl_binding(name) \
  static const char *wa_sig_##name; \
  m3ApiRawFunction(wa_fun_##name)

  decl_binding(debugger_break);
  decl_binding(debugger_continue);

  decl_binding(msg_recv);
  decl_binding(msg_size);

  decl_binding(snes_bus_read);
  decl_binding(snes_bus_write);

  decl_binding(ppux_reset);
  decl_binding(ppux_sprite_reset);
  decl_binding(ppux_sprite_read);
  decl_binding(ppux_sprite_write);
  decl_binding(ppux_ram_write);
  decl_binding(ppux_ram_read);

  decl_binding(draw_list_clear);
  decl_binding(draw_list_append);

#undef decl_binding

private:
  void draw_list(uint16_t* data);
  inline void draw_pixel(uint16_t* data, int16_t x0, int16_t y0, uint16_t color);
  inline void draw_hline(uint16_t* data, int16_t x0, int16_t y0, int16_t w, uint16_t color);
  inline void draw_vline(uint16_t* data, int16_t x0, int16_t y0, int16_t h, uint16_t color);
  void draw_text_utf8(uint8_t* s, int16_t x0, int16_t y0, const Font& font, const std::function<void(int,int)>& px);

  std::vector<uint8_t> cmdlist;
  uint16_t tmp[512 * 512];

  std::vector<std::shared_ptr<Font>> fonts;
};

extern WASMInterface wasmInterface;
