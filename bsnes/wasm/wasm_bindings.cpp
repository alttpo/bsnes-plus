
#include "wasminstance.hpp"

#define wasm_binding(name, sig) \
  const char* WASMInstanceBase::wa_sig_##name = sig; \
  const char* WASMInstanceBase::wa_fun_##name(void* _mem, uint64_t* _sp)

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

//int32_t za_file_locate(const char *i_filename);
wasm_binding(za_file_locate, "i(*)") {
  wa_return_type(int32_t);

  wa_arg_mem(const char*, i_filename);

  wa_check_mem(i_filename, 0);
  // TODO: upper bounds check

  auto fh = m_za->file_locate(i_filename);
  if (!fh) {
    wa_return(-1);
  }

  wa_return((int32_t)fh);
}

//int32_t za_file_size(int32_t fh, uint64_t* o_size);
wasm_binding(za_file_size, "i(i*)") {
  wa_return_type(int32_t);

  wa_arg    (int32_t,   i_fh);
  wa_arg_mem(uint64_t*, o_size);

  wa_check_mem(o_size, sizeof(uint64_t));

  if (!m_za->file_size(ZipArchive::FileHandle(i_fh), o_size)) {
    wa_return(0);
  }

  wa_return(1);
}

//int32_t za_file_extract(int32_t fh, void *o_data, uint64_t i_size);
wasm_binding(za_file_extract, "i(i*I)") {
  wa_return_type(int32_t);

  wa_arg    (int32_t,   i_fh);
  wa_arg_mem(void*, o_data);
  wa_arg    (uint64_t, i_size);

  wa_check_mem(o_data, i_size);

  if (!m_za->file_extract(ZipArchive::FileHandle(i_fh), o_data, i_size)) {
    wa_return(0);
  }

  wa_return(1);
}

//int32_t msg_size(uint16_t *o_size);
wasm_binding(msg_size, "i(*)") {
  wa_return_type(int32_t);

  wa_arg_mem(uint16_t*, o_size);

  wa_check_mem(o_size, sizeof(uint16_t));

  if (!msg_size(o_size)) {
    wa_return(-1)
  }

  wa_return(0)
}

//int32_t msg_recv(uint8_t *o_data, uint32_t i_size);
wasm_binding(msg_recv, "i(*i)") {
  wa_return_type(int32_t);

  wa_arg_mem(uint8_t *, o_data);
  wa_arg   (uint32_t,  i_size);

  wa_check_mem(o_data, i_size);

  auto msg = msg_dequeue();
  if (msg->m_size > i_size) {
    wa_return(-1)
  }

  memcpy((void *)o_data, (const void *)msg->m_data, msg->m_size);

  wa_return(0)
}

//void ppux_spaces_reset();
wasm_binding(ppux_spaces_reset, "v()") {
  // get the runtime instance caller:
  m_spaces->reset();

  wa_success();
}

//void ppux_font_load_za(int32_t i_fontindex, int32_t i_za_fh);
wasm_binding(ppux_font_load_za, "v(ii)") {
  wa_arg(int32_t, i_fontindex);
  wa_arg(int32_t, i_za_fh);

  auto fh = ZipArchive::FileHandle(i_za_fh);
  if (!fh) {
    wa_trap("invalid zip archive file handle");
  }

  // set a font using pcf format data:
  try {
    // measure file size:
    uint64_t size;
    if (m_za->file_size(fh, &size)) {
      // allocate buffer and extract:
      std::vector<char> data(size);

      if (m_za->file_extract(fh, data.data(), size)) {
        // load pcf data:
        m_fonts->load_pcf(i_fontindex, reinterpret_cast<const uint8_t *>(data.data()), size);
      }
    }
  } catch (std::runtime_error& err) {
    wa_trap( err.what());
  }

  wa_success();
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

  wa_arg   (uint32_t,          i_space);
  wa_arg   (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          i_data);
  wa_arg   (uint32_t,          i_size);

  wa_check_mem(i_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > DrawList::SpaceContainer::MaxCount) {
    //return makeErrorReply(QString("space must be 0..%1").arg(DrawList::SpaceContainer::MaxCount-1));
    wa_return(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = m_spaces->get_vram_space(i_space);
  maxSize = 0x10000;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    wa_return(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    wa_return(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    wa_return(-5);
  }

  memcpy(t + i_offset, i_data, i_size);

  wa_return(0);
}

//int32_t ppux_cgram_write(uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);
wasm_binding(ppux_cgram_write, "i(ii*i)") {
  wa_return_type(int32_t);

  wa_arg   (uint32_t,          i_space);
  wa_arg   (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          i_data);
  wa_arg   (uint32_t,          i_size);

  wa_check_mem(i_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > DrawList::SpaceContainer::MaxCount) {
    //return makeErrorReply(QString("space must be 0..%1").arg(DrawList::SpaceContainer::MaxCount-1));
    wa_return(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = m_spaces->get_cgram_space(i_space);
  maxSize = 0x200;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    wa_return(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    wa_return(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    wa_return(-5);
  }

  memcpy(t + i_offset, i_data, i_size);

  wa_return(0);
}

//int32_t ppux_oam_write(uint32_t i_offset, uint8_t *i_data, uint32_t i_size);
wasm_binding(ppux_oam_write, "i(i*i)") {
  wa_return_type(int32_t);

  wa_arg   (uint32_t,          i_space);
  wa_arg   (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          i_data);
  wa_arg   (uint32_t,          i_size);

  wa_check_mem(i_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > DrawList::SpaceContainer::MaxCount) {
    //return makeErrorReply(QString("space must be 0..%1").arg(DrawList::SpaceContainer::MaxCount-1));
    wa_return(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = SNES::ppu.ppux_get_oam();
  maxSize = 0x220;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    wa_return(-3);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    wa_return(-5);
  }

  memcpy(t + i_offset, i_data, i_size);

  wa_return(0);
}

//int32_t ppux_vram_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);
wasm_binding(ppux_vram_read, "i(ii*i)") {
  wa_return_type(int32_t);

  wa_arg   (uint32_t,          i_space);
  wa_arg   (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          o_data);
  wa_arg   (uint32_t,          i_size);

  wa_check_mem(o_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > DrawList::SpaceContainer::MaxCount) {
    //return makeErrorReply(QString("space must be 0..%1").arg(DrawList::SpaceContainer::MaxCount-1));
    wa_return(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = m_spaces->get_vram_space(i_space);
  maxSize = 0x10000;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    wa_return(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    wa_return(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    wa_return(-5);
  }

  memcpy(o_data, t + i_offset, i_size);

  wa_return(0);
}

//int32_t ppux_cgram_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);
wasm_binding(ppux_cgram_read, "i(ii*i)") {
  wa_return_type(int32_t);

  wa_arg   (uint32_t,          i_space);
  wa_arg   (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          o_data);
  wa_arg   (uint32_t,          i_size);

  wa_check_mem(o_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > DrawList::SpaceContainer::MaxCount) {
    //return makeErrorReply(QString("space must be 0..%1").arg(DrawList::SpaceContainer::MaxCount-1));
    wa_return(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = m_spaces->get_cgram_space(i_space);
  maxSize = 0x200;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    wa_return(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    wa_return(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    wa_return(-5);
  }

  memcpy(o_data, t + i_offset, i_size);

  wa_return(0);
}

//int32_t ppux_oam_read(uint32_t i_offset, uint8_t *o_data, uint32_t i_size);
wasm_binding(ppux_oam_read, "i(i*i)") {
  wa_return_type(int32_t);

  wa_arg   (uint32_t,          i_offset);
  wa_arg_mem(uint8_t*,          o_data);
  wa_arg   (uint32_t,          i_size);

  wa_check_mem(o_data, i_size);

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = SNES::ppu.ppux_get_oam();
  maxSize = 0x220;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    wa_return(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    wa_return(-3);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    wa_return(-5);
  }

  memcpy(o_data, t + i_offset, i_size);

  wa_return(0);
}

//void snes_bus_read(uint32_t i_address, uint8_t *i_data, uint32_t i_size);
wasm_binding(snes_bus_read, "v(i*i)") {
  wa_arg   (uint32_t, i_address);
  wa_arg_mem(uint8_t*, o_data);
  wa_arg   (uint32_t, i_size);

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
  wa_arg   (uint32_t, i_address);
  wa_arg_mem(uint8_t*, i_data);
  wa_arg   (uint32_t, i_size);

  wa_check_mem(i_data, i_size);

  for (uint32_t a = i_address, o = 0; o < i_size; o++, a++) {
    uint8_t b = i_data[o];
    SNES::bus.write(a, b);
  }

  wa_success();
}

wasm_binding(ppux_draw_list_reset, "v()") {
  SNES::ppu.ppux_draw_list_reset();

  wa_success();
}

wasm_binding(ppux_draw_list_append, "v(iii*)") {
  wa_arg   (uint8_t,   i_layer);
  wa_arg   (uint8_t,   i_priority);

  wa_arg   (uint32_t,  i_size);
  wa_arg_mem(uint8_t*,  i_cmdlist);

  wa_check_mem(i_cmdlist, i_size);

  // commands are aligned to 16-bits:
  if ((i_size & 1) != 0) {
    wa_trap("ppux_draw_list_append: i_size must be even");
  }

  // extend ppux_draw_lists vector:
  int n = SNES::ppu.ppux_draw_lists.size();
  SNES::ppu.ppux_draw_lists.resize(n + 1);

  // fill in the new cmdlist:
  auto& dl = SNES::ppu.ppux_draw_lists[n];
  dl.layer = i_layer;
  dl.priority = i_priority;
  // refer to the runtime instance's fonts and spaces collections:
  dl.fonts = m_fonts;
  dl.spaces = m_spaces;

  // copy cmdlist data in:
  dl.cmdlist.resize(i_size);
  void* dst = (void *)(dl.cmdlist.data());
  memcpy(dst, (const void *)i_cmdlist, i_size);

  wa_success();
}

#undef wasm_binding
