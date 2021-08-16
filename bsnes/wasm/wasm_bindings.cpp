
struct ppux_sprite {
  uint8_t  enabled;

  uint16_t x;
  uint16_t y;
  uint8_t  hflip;
  uint8_t  vflip;

  uint8_t  vram_space;        //     0 ..   255; 0 = local, 1..255 = extra
  uint16_t vram_addr;         // $0000 .. $FFFF (byte address)
  uint8_t  cgram_space;       //     0 ..   255; 0 = local, 1..255 = extra
  uint8_t  palette;           //     0 ..   255

  uint8_t  layer;             // 0.. 4;  BG1 = 0, BG2 = 1, BG3 = 2, BG4 = 3, OAM = 4
  uint8_t  priority;          // 1..12
  uint8_t  color_exemption;   // true = ignore color math, false = obey color math

  uint8_t  bpp;               // 2-, 4-, or 8-bpp tiles from vram[extra] and cgram[extra]
  uint16_t width;             // number of pixels width
  uint16_t height;            // number of pixels high
};

enum ppux_memory_type : uint32_t {
  VRAM,
  CGRAM
};

void WASMInterface::link_module(const std::shared_ptr<WASM::Module>& module) {
// link wasm_bindings.cpp member functions:
#define link(name) \
  module->linkEx("*", #name, wa_sig_##name, &WASM::RawCall<WASMInterface>::adapter<&WASMInterface::wa_fun_##name>, (const void *)this)

  link(debugger_break);
  link(debugger_continue);

  link(msg_recv);
  link(msg_size);

  link(snes_bus_read);
  link(snes_bus_write);

  link(ppux_reset);
  link(ppux_vram_reset);
  link(ppux_cgram_reset);
  link(ppux_draw_list_reset);
  link(ppux_draw_list_append);

  link(ppux_vram_write);
  link(ppux_cgram_write);
  link(ppux_oam_write);

  link(ppux_vram_read);
  link(ppux_cgram_read);
  link(ppux_oam_read);

#undef link
}

#define wasm_binding(name, sig) \
  const char *WASMInterface::wa_sig_##name = sig; \
  m3ApiRawFunction(WASMInterface::wa_fun_##name)

//void debugger_break();
wasm_binding(debugger_break, "v()") {
  m_do_break();

  m3ApiSuccess();
}

//void debugger_continue();
wasm_binding(debugger_continue, "v()") {
  m_do_continue();

  m3ApiSuccess();
}

//int32_t msg_size(uint16_t *o_size);
wasm_binding(msg_size, "i(*)") {
  m3ApiReturnType(int32_t);

  m3ApiGetArgMem(uint16_t *, o_size);

  m3ApiCheckMem(o_size, sizeof(uint16_t));

  auto m_runtime = WASM::host.get_runtime(runtime);

  if (!m_runtime->msg_size(o_size)) {
    m3ApiReturn(-1)
  }

  m3ApiReturn(0)
}

//int32_t msg_recv(uint8_t *o_data, uint32_t i_size);
wasm_binding(msg_recv, "i(*i)") {
  m3ApiReturnType(int32_t);

  m3ApiGetArgMem(uint8_t *, o_data);
  m3ApiGetArg   (uint32_t,  i_size);

  m3ApiCheckMem(o_data, i_size);

  auto m_runtime = WASM::host.get_runtime(runtime);

  auto msg = m_runtime->msg_dequeue();
  if (msg->m_size > i_size) {
    m3ApiReturn(-1)
  }

  memcpy((void *)o_data, (const void *)msg->m_data, msg->m_size);

  m3ApiReturn(0)
}

//void ppux_reset();
wasm_binding(ppux_reset, "v()") {
  SNES::ppu.ppux_reset();
  m3ApiSuccess();
}

//void ppux_sprite_reset();
wasm_binding(ppux_vram_reset, "v()") {
  SNES::ppu.ppux_vram_reset();
  m3ApiSuccess();
}

//void ppux_sprite_reset();
wasm_binding(ppux_cgram_reset, "v()") {
  SNES::ppu.ppux_cgram_reset();
  m3ApiSuccess();
}

//int32_t ppux_vram_write(uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);
wasm_binding(ppux_vram_write, "i(ii*i)") {
  m3ApiReturnType(int32_t);

  m3ApiGetArg   (uint32_t,          i_space);
  m3ApiGetArg   (uint32_t,          i_offset);
  m3ApiGetArgMem(uint8_t*,          i_data);
  m3ApiGetArg   (uint32_t,          i_size);

  m3ApiCheckMem(i_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > SNES::PPU::extra_spaces) {
    //return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
    m3ApiReturn(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = SNES::ppu.get_vram_space(i_space);
  maxSize = 0x10000;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    m3ApiReturn(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    m3ApiReturn(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    m3ApiReturn(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    m3ApiReturn(-5);
  }

  memcpy(t + i_offset, i_data, i_size);

  m3ApiReturn(0);
}

//int32_t ppux_cgram_write(uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);
wasm_binding(ppux_cgram_write, "i(ii*i)") {
  m3ApiReturnType(int32_t);

  m3ApiGetArg   (uint32_t,          i_space);
  m3ApiGetArg   (uint32_t,          i_offset);
  m3ApiGetArgMem(uint8_t*,          i_data);
  m3ApiGetArg   (uint32_t,          i_size);

  m3ApiCheckMem(i_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > SNES::PPU::extra_spaces) {
    //return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
    m3ApiReturn(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = SNES::ppu.get_cgram_space(i_space);
  maxSize = 0x200;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    m3ApiReturn(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    m3ApiReturn(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    m3ApiReturn(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    m3ApiReturn(-5);
  }

  memcpy(t + i_offset, i_data, i_size);

  m3ApiReturn(0);
}

//int32_t ppux_oam_write(uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);
wasm_binding(ppux_oam_write, "i(ii*i)") {
  m3ApiReturnType(int32_t);

  m3ApiGetArg   (uint32_t,          i_space);
  m3ApiGetArg   (uint32_t,          i_offset);
  m3ApiGetArgMem(uint8_t*,          i_data);
  m3ApiGetArg   (uint32_t,          i_size);

  m3ApiCheckMem(i_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > SNES::PPU::extra_spaces) {
    //return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
    m3ApiReturn(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = SNES::ppu.get_oam_space(i_space);
  maxSize = 0x220;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    m3ApiReturn(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    m3ApiReturn(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    m3ApiReturn(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    m3ApiReturn(-5);
  }

  memcpy(t + i_offset, i_data, i_size);

  m3ApiReturn(0);
}

//int32_t ppux_vram_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);
wasm_binding(ppux_vram_read, "i(ii*i)") {
  m3ApiReturnType(int32_t);

  m3ApiGetArg   (uint32_t,          i_space);
  m3ApiGetArg   (uint32_t,          i_offset);
  m3ApiGetArgMem(uint8_t*,          o_data);
  m3ApiGetArg   (uint32_t,          i_size);

  m3ApiCheckMem(o_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > SNES::PPU::extra_spaces) {
    //return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
    m3ApiReturn(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = SNES::ppu.get_vram_space(i_space);
  maxSize = 0x10000;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    m3ApiReturn(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    m3ApiReturn(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    m3ApiReturn(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    m3ApiReturn(-5);
  }

  memcpy(o_data, t + i_offset, i_size);

  m3ApiReturn(0);
}

//int32_t ppux_cgram_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);
wasm_binding(ppux_cgram_read, "i(ii*i)") {
  m3ApiReturnType(int32_t);

  m3ApiGetArg   (uint32_t,          i_space);
  m3ApiGetArg   (uint32_t,          i_offset);
  m3ApiGetArgMem(uint8_t*,          o_data);
  m3ApiGetArg   (uint32_t,          i_size);

  m3ApiCheckMem(o_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > SNES::PPU::extra_spaces) {
    //return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
    m3ApiReturn(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = SNES::ppu.get_cgram_space(i_space);
  maxSize = 0x200;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    m3ApiReturn(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    m3ApiReturn(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    m3ApiReturn(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    m3ApiReturn(-5);
  }

  memcpy(o_data, t + i_offset, i_size);

  m3ApiReturn(0);
}

//int32_t ppux_oam_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);
wasm_binding(ppux_oam_read, "i(ii*i)") {
  m3ApiReturnType(int32_t);

  m3ApiGetArg   (uint32_t,          i_space);
  m3ApiGetArg   (uint32_t,          i_offset);
  m3ApiGetArgMem(uint8_t*,          o_data);
  m3ApiGetArg   (uint32_t,          i_size);

  m3ApiCheckMem(o_data, i_size);

  // 0 = local, 1..255 = extra
  if (i_space > SNES::PPU::extra_spaces) {
    //return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
    m3ApiReturn(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  t = SNES::ppu.get_oam_space(i_space);
  maxSize = 0x220;
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    m3ApiReturn(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    m3ApiReturn(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    m3ApiReturn(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    m3ApiReturn(-5);
  }

  memcpy(o_data, t + i_offset, i_size);

  m3ApiReturn(0);
}

//void snes_bus_read(uint32_t i_address, uint8_t *i_data, uint32_t i_size);
wasm_binding(snes_bus_read, "v(i*i)") {
  m3ApiGetArg   (uint32_t, i_address);
  m3ApiGetArgMem(uint8_t*, o_data);
  m3ApiGetArg   (uint32_t, i_size);

  m3ApiCheckMem(o_data, i_size);

  for (uint32_t a = i_address, o = 0; o < i_size; o++, a++) {
    uint8_t b;
    b = SNES::bus.read(a);
    o_data[o] = b;
  }

  m3ApiSuccess();
}

//void snes_bus_write(uint32_t i_address, uint8_t *o_data, uint32_t i_size);
wasm_binding(snes_bus_write, "v(i*i)") {
  m3ApiGetArg   (uint32_t, i_address);
  m3ApiGetArgMem(uint8_t*, i_data);
  m3ApiGetArg   (uint32_t, i_size);

  m3ApiCheckMem(i_data, i_size);

  for (uint32_t a = i_address, o = 0; o < i_size; o++, a++) {
    uint8_t b = i_data[o];
    SNES::bus.write(a, b);
  }

  m3ApiSuccess();
}

wasm_binding(ppux_draw_list_reset, "v()") {
  SNES::ppu.ppux_draw_list_reset();

  m3ApiSuccess();
}

wasm_binding(ppux_draw_list_append, "v(iii*)") {
  m3ApiGetArg   (uint8_t,   i_layer);
  m3ApiGetArg   (uint8_t,   i_priority);

  m3ApiGetArg   (uint32_t,  i_size);
  m3ApiGetArgMem(uint8_t*,  i_cmdlist);

  m3ApiCheckMem(i_cmdlist, i_size);

  // commands are aligned to 16-bits:
  if ((i_size & 1) != 0) {
    m3ApiTrap("ppux_draw_list_append: i_size must be even");
  }

  // extend ppux_draw_lists vector:
  int n = SNES::ppu.ppux_draw_lists.size();
  SNES::ppu.ppux_draw_lists.resize(n + 1);

  auto& dl = SNES::ppu.ppux_draw_lists[n];
  dl.layer = i_layer;
  dl.priority = i_priority;
  dl.cmdlist.resize(i_size);

  // copy cmdlist data in:
  void* dst = (void *)(dl.cmdlist.data());
  memcpy(dst, (const void *)i_cmdlist, i_size);

  m3ApiSuccess();
}

#undef wasm_binding
