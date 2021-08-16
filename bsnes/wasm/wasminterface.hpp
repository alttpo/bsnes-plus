#pragma once

#include "host.hpp"
#include "pixelfont.hpp"
#include "drawlist.hpp"

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
  decl_binding(ppux_vram_reset);
  decl_binding(ppux_cgram_reset);
  decl_binding(ppux_draw_list_reset);
  decl_binding(ppux_draw_list_append);

  decl_binding(ppux_vram_write);
  decl_binding(ppux_cgram_write);
  decl_binding(ppux_oam_write);

  decl_binding(ppux_vram_read);
  decl_binding(ppux_cgram_read);
  decl_binding(ppux_oam_read);

#undef decl_binding

public:
  std::vector<std::shared_ptr<PixelFont::Font>> fonts;
};

extern WASMInterface wasmInterface;
