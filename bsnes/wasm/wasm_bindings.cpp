
#include "wasminstance.hpp"

#define wasm_binding(name, sig) \
  const char* WASMInstanceBase::wa_sig_##name = sig; \
  const char* WASMInstanceBase::wa_fun_##name(void* _mem, uint64_t* _sp)

//i32 runtime_alloc(i32, i32, i32)
wasm_binding(runtime_alloc, "i(iii)") {
  wa_return_type(int32_t);

  wa_return(0);
}

//void debugger_break();
wasm_binding(debugger_break, "v()") {
  m_interface->m_do_break();

  wa_success();
}

//void debugger_continue();
wasm_binding(debugger_continue, "v()") {
  m_interface->m_do_continue();

  wa_success();
}

//void log_c(int32_t level, const char* msg);
wasm_binding(log_c, "v(i*)") {
  wa_arg    (int32_t,     i_level);
  wa_arg_mem(const char*, i_msg);

  // TODO: check len of i_msg
  wa_check_mem(i_msg, 0);

  m_interface->log_module_message(static_cast<log_level>(i_level), m_key, i_msg);

  wa_success();
}

// go_string expands to `uintptr data, uintptr len` which we're hoping are just uint32_ts here:
//void log_go(int32_t level, go_string msg);
wasm_binding(log_go, "v(iii)") {
  wa_arg    (int32_t,     i_level);
  wa_arg_mem(const char*, i_msg_data);
  wa_arg    (uint32_t,    i_msg_len);

  // TODO: check len of i_msg
  wa_check_mem(i_msg_data, i_msg_len);

  m_interface->log_module_message(static_cast<log_level>(i_level), m_key, std::string(i_msg_data, i_msg_len));

  wa_success();
}

//int32_t za_file_locate_c(const char *i_filename, uint32_t* o_index);
wasm_binding(za_file_locate_c, "i(**)") {
  wa_return_type(int32_t);

  wa_arg_mem(const char*, i_filename);
  wa_arg_mem(uint32_t*,   o_index);

  // TODO: upper bounds check
  wa_check_mem(i_filename, 0);
  wa_check_mem(o_index, sizeof(uint32_t));

  if (!m_za->file_locate(i_filename, o_index)) {
    report_error(m_za->last_error());
    wa_return(-1);
  }

  wa_return(0);
}

//func za_file_locate_go(i_filename string, o_index *uint32) int32
wasm_binding(za_file_locate_go, "i(ii*)") {
  wa_return_type(int32_t);

  wa_arg_mem(const char*, i_filename_data);
  wa_arg    (uint32_t,    i_filename_len);
  wa_arg_mem(uint32_t*,   o_index);

  wa_check_mem(i_filename_data, i_filename_len);
  wa_check_mem(o_index, sizeof(uint32_t));

  std::string i_filename(i_filename_data, i_filename_len);
  if (!m_za->file_locate(i_filename.c_str(), o_index)) {
    report_error(m_za->last_error());
    wa_return(-1);
  }

  wa_return(0);
}

//int32_t za_file_size(int32_t fh, uint64_t* o_size);
wasm_binding(za_file_size, "i(i*)") {
  wa_return_type(int32_t);

  wa_arg    (uint32_t,  i_fh);
  wa_arg_mem(uint64_t*, o_size);

  wa_check_mem(o_size, sizeof(uint64_t));

  if (!m_za->file_size(i_fh, o_size)) {
    report_error(m_za->last_error());
    wa_return(-1);
  }

  wa_return(0);
}

//int32_t za_file_extract(int32_t fh, void *o_data, uint64_t i_size);
wasm_binding(za_file_extract, "i(i*I)") {
  wa_return_type(int32_t);

  wa_arg    (uint32_t, i_fh);
  wa_arg_mem(void*,    o_data);
  wa_arg    (uint64_t, i_size);

  wa_check_mem(o_data, i_size);

  if (!m_za->file_extract(i_fh, o_data, i_size)) {
    report_error(m_za->last_error());
    wa_return(-1);
  }

  wa_return(0);
}

//int32_t msg_size(uint16_t *o_size);
wasm_binding(msg_size, "i(*)") {
  wa_return_type(int32_t);

  wa_arg_mem(uint16_t*, o_size);

  wa_check_mem(o_size, sizeof(uint16_t));

  if (!msg_size(o_size)) {
    wa_return(-1);
  }

  wa_return(0);
}

//int32_t msg_recv(uint8_t *o_data, uint32_t i_size);
wasm_binding(msg_recv, "i(*i)") {
  wa_return_type(int32_t);

  wa_arg_mem(uint8_t *, o_data);
  wa_arg    (uint32_t,  i_size);

  wa_check_mem(o_data, i_size);

  auto msg = msg_dequeue();
  if (msg->m_size > i_size) {
    wa_return(-1);
  }

  memcpy((void *)o_data, (const void *)msg->m_data, msg->m_size);

  wa_return(0);
}

//void ppux_spaces_reset();
wasm_binding(ppux_spaces_reset, "v()") {
  // get the runtime instance caller:
  m_spaces->reset();

  wa_success();
}

//int32_t ppux_font_load_za(int32_t i_fontindex, int32_t i_za_fh);
wasm_binding(ppux_font_load_za, "i(ii)") {
  wa_return_type(int32_t); // bool

  wa_arg(int32_t,  i_fontindex);
  wa_arg(uint32_t, i_fh);

  // set a font using pcf format data:

  // measure file size:
  uint64_t size;
  if (!m_za->file_size(i_fh, &size)) {
    report_error(m_za->last_error());
    wa_return(-1);
  }

  // allocate buffer and extract:
  std::vector<char> data(size);

  if (!m_za->file_extract(i_fh, data.data(), size)) {
    report_error(m_za->last_error());
    wa_return(-1);
  }

  // load pcf data:
  m_fonts->load_pcf(i_fontindex, reinterpret_cast<const uint8_t *>(data.data()), size);

  wa_return(0);
}

//void ppux_font_delete(int32_t i_fontindex);
wasm_binding(ppux_font_delete, "v(i)") {
  wa_arg(int32_t, i_fontindex);

  // delete a font:
  m_fonts->erase(i_fontindex);

  wa_success();
}

//int32_t ppux_vram_write(uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);
wasm_binding(ppux_vram_write, "i(ii*i)") {
  wa_return_type(int32_t);

  wa_arg    (uint32_t,          i_space);
  wa_arg    (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          i_data);
  wa_arg    (uint32_t,          i_size);

  wa_check_mem(i_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > DrawList::SpaceContainer::MaxCount) {
    std::string err("space out of range; space=");
    err.append(std::to_string(i_space));
    err.append(" >= ");
    err.append(std::to_string(DrawList::SpaceContainer::MaxCount));
    report_error(WASMError("ppux_vram_write", err));
    wa_return(-1);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = m_spaces->get_vram_space(i_space);
  maxSize = 0x10000;
  if (!t) {
    std::string err("VRAM memory not allocated for space; space=");
    err.append(std::to_string(i_space));
    report_error(WASMError("ppux_vram_write", err));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    std::string err("offset out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(" >= ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_vram_write", err));
    wa_return(-1);
  }
  if (i_offset & 1) {
    std::string err("offset must be a multiple of 2; offset=");
    err.append(std::to_string(i_offset));
    report_error(WASMError("ppux_vram_write", err));
    wa_return(-1);
  }

  if (i_offset + i_size > maxSize) {
    std::string err("offset+size out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(",size=");
    err.append(std::to_string(i_size));
    err.append("; total=");
    err.append(std::to_string(i_offset+i_size));
    err.append(" > ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_vram_write", err));
    wa_return(-1);
  }

  memcpy(t + i_offset, i_data, i_size);

  wa_return(0);
}

//int32_t ppux_cgram_write(uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);
wasm_binding(ppux_cgram_write, "i(ii*i)") {
  wa_return_type(int32_t);

  wa_arg    (uint32_t,          i_space);
  wa_arg    (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          i_data);
  wa_arg    (uint32_t,          i_size);

  wa_check_mem(i_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > DrawList::SpaceContainer::MaxCount) {
    std::string err("space out of range; space=");
    err.append(std::to_string(i_space));
    err.append(" >= ");
    err.append(std::to_string(DrawList::SpaceContainer::MaxCount));
    report_error(WASMError("ppux_cgram_write", err));
    wa_return(-1);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = m_spaces->get_cgram_space(i_space);
  maxSize = 0x200;
  if (!t) {
    std::string err("CGRAM memory not allocated for space; space=");
    err.append(std::to_string(i_space));
    report_error(WASMError("ppux_cgram_write", err));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    std::string err("offset out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(" >= ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_cgram_write", err));
    wa_return(-1);
  }
  if (i_offset & 1) {
    std::string err("offset must be a multiple of 2; offset=");
    err.append(std::to_string(i_offset));
    report_error(WASMError("ppux_cgram_write", err));
    wa_return(-1);
  }

  if (i_offset + i_size > maxSize) {
    std::string err("offset+size out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(",size=");
    err.append(std::to_string(i_size));
    err.append("; total=");
    err.append(std::to_string(i_offset+i_size));
    err.append(" > ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_cgram_write", err));
    wa_return(-1);
  }

  memcpy(t + i_offset, i_data, i_size);

  wa_return(0);
}

//int32_t ppux_oam_write(uint32_t i_offset, uint8_t *i_data, uint32_t i_size);
wasm_binding(ppux_oam_write, "i(i*i)") {
  wa_return_type(int32_t);

  wa_arg    (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          i_data);
  wa_arg    (uint32_t,          i_size);

  wa_check_mem(i_data, i_size);

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = SNES::ppu.ppux_get_oam();
  maxSize = 0x220;
  if (!t) {
    std::string err("OAM memory not allocated");
    report_error(WASMError("ppux_oam_write", err));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    std::string err("offset out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(" >= ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_oam_write", err));
    wa_return(-1);
  }

  if (i_offset + i_size > maxSize) {
    std::string err("offset+size out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(",size=");
    err.append(std::to_string(i_size));
    err.append("; total=");
    err.append(std::to_string(i_offset+i_size));
    err.append(" > ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_oam_write", err));
    wa_return(-1);
  }

  memcpy(t + i_offset, i_data, i_size);

  wa_return(0);
}

//int32_t ppux_vram_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);
wasm_binding(ppux_vram_read, "i(ii*i)") {
  wa_return_type(int32_t);

  wa_arg    (uint32_t,          i_space);
  wa_arg    (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          o_data);
  wa_arg    (uint32_t,          i_size);

  wa_check_mem(o_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > DrawList::SpaceContainer::MaxCount) {
    std::string err("space out of range; space=");
    err.append(std::to_string(i_space));
    err.append(" >= ");
    err.append(std::to_string(DrawList::SpaceContainer::MaxCount));
    report_error(WASMError("ppux_vram_read", err));
    wa_return(-1);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = m_spaces->get_vram_space(i_space);
  maxSize = 0x10000;
  if (!t) {
    std::string err("VRAM memory not allocated for space; space=");
    err.append(std::to_string(i_space));
    report_error(WASMError("ppux_vram_read", err));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    std::string err("offset out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(" >= ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_vram_read", err));
    wa_return(-1);
  }
  if (i_offset & 1) {
    std::string err("offset must be a multiple of 2; offset=");
    err.append(std::to_string(i_offset));
    report_error(WASMError("ppux_vram_read", err));
    wa_return(-1);
  }

  if (i_offset + i_size > maxSize) {
    std::string err("offset+size out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(",size=");
    err.append(std::to_string(i_size));
    err.append("; total=");
    err.append(std::to_string(i_offset+i_size));
    err.append(" > ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_vram_read", err));
    wa_return(-1);
  }

  memcpy(o_data, t + i_offset, i_size);

  wa_return(0);
}

//int32_t ppux_cgram_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);
wasm_binding(ppux_cgram_read, "i(ii*i)") {
  wa_return_type(int32_t);

  wa_arg    (uint32_t,          i_space);
  wa_arg    (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          o_data);
  wa_arg    (uint32_t,          i_size);

  wa_check_mem(o_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > DrawList::SpaceContainer::MaxCount) {
    std::string err("space out of range; space=");
    err.append(std::to_string(i_space));
    err.append(" >= ");
    err.append(std::to_string(DrawList::SpaceContainer::MaxCount));
    report_error(WASMError("ppux_cgram_read", err));
    wa_return(-1);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = m_spaces->get_cgram_space(i_space);
  maxSize = 0x200;
  if (!t) {
    std::string err("CGRAM memory not allocated for space; space=");
    err.append(std::to_string(i_space));
    report_error(WASMError("ppux_cgram_read", err));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    std::string err("offset out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(" >= ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_cgram_read", err));
    wa_return(-1);
  }
  if (i_offset & 1) {
    std::string err("offset must be a multiple of 2; offset=");
    err.append(std::to_string(i_offset));
    report_error(WASMError("ppux_cgram_read", err));
    wa_return(-1);
  }

  if (i_offset + i_size > maxSize) {
    std::string err("offset+size out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(",size=");
    err.append(std::to_string(i_size));
    err.append("; total=");
    err.append(std::to_string(i_offset+i_size));
    err.append(" > ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_cgram_read", err));
    wa_return(-1);
  }

  memcpy(o_data, t + i_offset, i_size);

  wa_return(0);
}

//int32_t ppux_oam_read(uint32_t i_offset, uint8_t *o_data, uint32_t i_size);
wasm_binding(ppux_oam_read, "i(i*i)") {
  wa_return_type(int32_t);

  wa_arg    (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          o_data);
  wa_arg    (uint32_t,          i_size);

  wa_check_mem(o_data, i_size);

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = SNES::ppu.ppux_get_oam();
  maxSize = 0x220;
  if (!t) {
    std::string err("OAM memory not allocated");
    report_error(WASMError("ppux_oam_read", err));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    std::string err("offset out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(" >= ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_oam_read", err));
    wa_return(-1);
  }

  if (i_offset + i_size > maxSize) {
    std::string err("offset+size out of range; offset=");
    err.append(std::to_string(i_offset));
    err.append(",size=");
    err.append(std::to_string(i_size));
    err.append("; total=");
    err.append(std::to_string(i_offset+i_size));
    err.append(" > ");
    err.append(std::to_string(maxSize));
    report_error(WASMError("ppux_oam_read", err));
    wa_return(-1);
  }

  memcpy(o_data, t + i_offset, i_size);

  wa_return(0);
}

//void snes_bus_read(uint32_t i_address, uint8_t *i_data, uint32_t i_size);
wasm_binding(snes_bus_read, "v(i*i)") {
  wa_arg    (uint32_t, i_address);
  wa_arg_mem(uint8_t*, o_data);
  wa_arg    (uint32_t, i_size);

  wa_check_mem(o_data, i_size);

  for (uint32_t a = i_address, o = 0; o < i_size; o++, a++) {
    uint8_t b;
    b = SNES::bus.read(a);
    o_data[o] = b;
  }

  wa_success();
}

//void snes_bus_write(uint32_t i_address, uint8_t *o_data, uint32_t i_size);
wasm_binding(snes_bus_write, "v(i*i)") {
  wa_arg    (uint32_t, i_address);
  wa_arg_mem(uint8_t*, i_data);
  wa_arg    (uint32_t, i_size);

  wa_check_mem(i_data, i_size);

  for (uint32_t a = i_address, o = 0; o < i_size; o++, a++) {
    uint8_t b = i_data[o];
    SNES::bus.write(a, b);
  }

  wa_success();
}

wasm_binding(ppux_draw_list_clear, "v()") {
  SNES::ppu.ppux_draw_list_clear();

  wa_success();
}

wasm_binding(ppux_draw_list_resize, "v(i)") {
  wa_arg    (uint32_t,  i_len);

  SNES::ppu.ppux_draw_lists.resize(i_len);

  wa_success();
}

wasm_binding(ppux_draw_list_set, "i(ii*)") {
  wa_return_type(uint32_t);

  wa_arg    (uint32_t,  i_index);
  // length of cmdlist in words (uint16_t):
  wa_arg    (uint32_t,  i_len);
  wa_arg_mem(uint8_t*,  i_cmdlist);

  wa_check_mem(i_cmdlist, i_len*sizeof(uint16_t));

  if (i_index >= SNES::ppu.ppux_draw_lists.size()) {
    report_error(WASMError("ppux_draw_list_set", "index out of bounds of ppux_draw_lists vector"));
    wa_trap("[trap] ppux_draw_list_set index out of bounds of ppux_draw_lists vector");
  }

  // fill in the new cmdlist:
  auto& dl = SNES::ppu.ppux_draw_lists[i_index];
  // refer to the runtime instance's fonts and spaces collections:
  dl.fonts = m_fonts;
  dl.spaces = m_spaces;

  // copy cmdlist data in:
  dl.cmdlist.resize(i_len);
  void* dst = (void *)(dl.cmdlist.data());
  memcpy(dst, (const void *)i_cmdlist, i_len*sizeof(uint16_t));

  wa_return(i_index);
}

wasm_binding(ppux_draw_list_append, "i(i*)") {
  wa_return_type(uint32_t);

  // length of cmdlist in words (uint16_t):
  wa_arg    (uint32_t,  i_len);
  wa_arg_mem(uint8_t*,  i_cmdlist);

  wa_check_mem(i_cmdlist, i_len*sizeof(uint16_t));

  // extend ppux_draw_lists vector:
  int n = SNES::ppu.ppux_draw_lists.size();
  SNES::ppu.ppux_draw_lists.resize(n + 1);

  // fill in the new cmdlist:
  auto& dl = SNES::ppu.ppux_draw_lists[n];
  // refer to the runtime instance's fonts and spaces collections:
  dl.fonts = m_fonts;
  dl.spaces = m_spaces;

  // copy cmdlist data in:
  dl.cmdlist.resize(i_len);
  void* dst = (void *)(dl.cmdlist.data());
  memcpy(dst, (const void *)i_cmdlist, i_len*sizeof(uint16_t));

  wa_return(n);
}

#undef wasm_binding
