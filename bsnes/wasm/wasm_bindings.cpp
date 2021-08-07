
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

void ModuleInstance::link_module(wasm_extern_vec_t *imports) {
  unsigned int i = 0;

  wasm_extern_vec_new_uninitialized(imports, 13);

// link wasm_bindings.cpp member functions:
#define link(name) { \
    wasm_functype_t*  host_func_type  = parse_sig(wa_sig_##name); \
    wasm_func_t*      host_func       = wasm_func_new_with_env( \
      wasmInterface.m_store.get(), \
      host_func_type, \
      /* wasm_func_callback_with_env_t: */ \
      [](void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) -> wasm_trap_t* { \
        return ((ModuleInstance*)env)->wa_fun_##name(args, results); \
      }, \
      (void *)this, \
      nullptr \
    ); \
    wasm_functype_delete(host_func_type); \
    imports->data[i++] = wasm_func_as_extern(host_func); \
  }

  link(debugger_break);
  link(debugger_continue);

  link(msg_recv);
  link(msg_size);

  link(snes_bus_read);
  link(snes_bus_write);

  link(ppux_reset);
  link(ppux_sprite_reset);
  link(ppux_sprite_read);
  link(ppux_sprite_write);
  link(ppux_ram_read);
  link(ppux_ram_write);

  link(frame_acquire);

#undef link
}

#define wasm_binding(name, sig) \
  const char *ModuleInstance::wa_sig_##name = sig; \
  wasm_trap_t* ModuleInstance::wa_fun_##name(const wasm_val_vec_t* args, wasm_val_vec_t* results)

//void debugger_break();
wasm_binding(debugger_break, "v()") {
  wasmInterface.m_do_break();

  return nullptr;
}

//void debugger_continue();
wasm_binding(debugger_continue, "v()") {
  wasmInterface.m_do_continue();

  return nullptr;
}

//int32_t msg_size(uint16_t *o_size);
wasm_binding(msg_size, "i(*)") {
  m3ApiReturnType(int32_t)

  uint16_t *o_size = mem<uint16_t>(args->data[0].of.i32);

  m3ApiCheckMem(o_size, sizeof(uint16_t));

  if (!msg_size(o_size)) {
    wasm_val_t val = WASM_I32_VAL(-1);
    wasm_val_copy(&results->data[0], &val);
    wasm_val_delete(&val);
    return nullptr;
  }

  {
    wasm_val_t val = WASM_I32_VAL(0);
    wasm_val_copy(&results->data[0], &val);
    wasm_val_delete(&val);
    return nullptr;
  }
}

//int32_t msg_recv(uint8_t *o_data, uint32_t i_size);
wasm_binding(msg_recv, "i(*i)") {
  m3ApiReturnType(int32_t)

  m3ApiGetArgMem(uint8_t *, o_data)
  m3ApiGetArg   (uint32_t,  i_size)

  m3ApiCheckMem(o_data, i_size);

  auto m_runtime = WASM::host.get_runtime(runtime);

  auto msg = m_runtime->msg_dequeue();
  if (msg->m_size > i_size) {
    m3ApiReturn(-1)
  }

  memcpy((void *)o_data, (const void *)msg->m_data, msg->m_size);

  {
    wasm_val_t val = WASM_I32_VAL(0);
    wasm_val_copy(&results->data[0], &val);
    wasm_val_delete(&val);
    return nullptr;
  }
}

//void ppux_reset();
wasm_binding(ppux_reset, "v()") {
  SNES::ppu.ppux_reset();
  return nullptr;
}

//void ppux_sprite_reset();
wasm_binding(ppux_sprite_reset, "v()") {
  SNES::ppu.ppux_sprite_reset();
  return nullptr;
}

//int32_t ppux_sprite_read(uint32_t i_index, struct ppux_sprite *o_spr);
wasm_binding(ppux_sprite_read, "i(i*)") {
  m3ApiReturnType(int32_t)

  m3ApiGetArg   (uint32_t,             i_index)
  m3ApiGetArgMem(struct ppux_sprite *, o_spr)

  m3ApiCheckMem(o_spr, sizeof(struct ppux_sprite));

  if (i_index >= SNES::PPU::extra_count) {
    m3ApiReturn(-1);
  }

  if (!o_spr) {
    m3ApiReturn(-2);
  }

  auto &t = SNES::ppu.extra_list[i_index];

  o_spr->enabled = t.enabled;

  o_spr->x = t.x;
  o_spr->y = t.y;
  o_spr->hflip = t.hflip;
  o_spr->vflip = t.vflip;

  o_spr->vram_space = t.vram_space;
  o_spr->vram_addr = t.vram_addr;
  o_spr->cgram_space = t.cgram_space;
  o_spr->palette = t.palette;

  o_spr->layer = t.layer;
  o_spr->priority = t.priority;
  o_spr->color_exemption = t.color_exemption;

  o_spr->bpp = t.bpp;
  o_spr->width = t.width;
  o_spr->height = t.height;

  {
    wasm_val_t val = WASM_I32_VAL(0);
    wasm_val_copy(&results->data[0], &val);
    wasm_val_delete(&val);
    return nullptr;
  }
}

//int32_t ppux_sprite_write(uint32_t i_index, struct ppux_sprite *i_spr);
wasm_binding(ppux_sprite_write, "i(i*)") {
  m3ApiReturnType(int32_t)

  m3ApiGetArg(uint32_t, i_index)
  m3ApiGetArgMem(struct ppux_sprite *, i_spr)

  m3ApiCheckMem(i_spr, sizeof(struct ppux_sprite));

  if (i_index >= SNES::PPU::extra_count) {
    m3ApiReturn(-1);
  }

  if (!i_spr) {
    m3ApiReturn(-2);
  }

  if (i_spr->vram_space >= SNES::PPU::extra_spaces) {
    m3ApiReturn(-3);
  }

  if (i_spr->cgram_space >= SNES::PPU::extra_spaces) {
    m3ApiReturn(-4);
  }

  auto &t = SNES::ppu.extra_list[i_index];

  t.enabled = i_spr->enabled;

  t.x = i_spr->x;
  t.y = i_spr->y;
  t.hflip = i_spr->hflip;
  t.vflip = i_spr->vflip;

  t.vram_space = i_spr->vram_space;
  t.vram_addr = i_spr->vram_addr;
  t.cgram_space = i_spr->cgram_space;
  t.palette = i_spr->palette;

  t.layer = i_spr->layer;
  t.priority = i_spr->priority;
  t.color_exemption = i_spr->color_exemption;

  t.bpp = i_spr->bpp;
  t.width = i_spr->width;
  t.height = i_spr->height;

  {
    wasm_val_t val = WASM_I32_VAL(0);
    wasm_val_copy(&results->data[0], &val);
    wasm_val_delete(&val);
    return nullptr;
  }
}

//int32_t ppux_ram_write(enum ppux_memory_type i_memory, uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);
wasm_binding(ppux_ram_write, "i(iii*i)") {
  m3ApiReturnType(int32_t)

  m3ApiGetArg   (ppux_memory_type,  i_memory)
  m3ApiGetArg   (uint32_t,          i_space)
  m3ApiGetArg   (uint32_t,          i_offset)
  m3ApiGetArgMem(uint8_t*,          i_data)
  m3ApiGetArg   (uint32_t,          i_size)

  m3ApiCheckMem(i_data, i_size);

  if (i_space >= SNES::PPU::extra_spaces) {
    //return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
    m3ApiReturn(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  if (i_memory == ppux_memory_type::VRAM) {
    t = SNES::ppu.get_vram_space(i_space);
    maxSize = 0x10000;
  } else if (i_memory == ppux_memory_type::CGRAM) {
    t = SNES::ppu.get_cgram_space(i_space);
    maxSize = 0x200;
  } else {
    //return makeErrorReply(QString("unknown memory type '%1' expected 'VRAM' or 'CGRAM'").arg(memory));
    m3ApiReturn(-1);
  }
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

  {
    wasm_val_t val = WASM_I32_VAL(0);
    wasm_val_copy(&results->data[0], &val);
    wasm_val_delete(&val);
    return nullptr;
  }
}

//int32_t ppux_ram_write(enum ppux_memory_type i_memory, uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);
wasm_binding(ppux_ram_read, "i(iii*i)") {
  m3ApiReturnType(int32_t)

  m3ApiGetArg   (ppux_memory_type,  i_memory)
  m3ApiGetArg   (uint32_t,          i_space)
  m3ApiGetArg   (uint32_t,          i_offset)
  m3ApiGetArgMem(uint8_t*,          o_data)
  m3ApiGetArg   (uint32_t,          i_size)

  m3ApiCheckMem(o_data, i_size);

  if (i_space >= SNES::PPU::extra_spaces) {
    //return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
    m3ApiReturn(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  if (i_memory == ppux_memory_type::VRAM) {
    t = SNES::ppu.get_vram_space(i_space);
    maxSize = 0x10000;
  } else if (i_memory == ppux_memory_type::CGRAM) {
    t = SNES::ppu.get_cgram_space(i_space);
    maxSize = 0x200;
  } else {
    //return makeErrorReply(QString("unknown memory type '%1' expected 'VRAM' or 'CGRAM'").arg(memory));
    m3ApiReturn(-1);
  }
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

  {
    wasm_val_t val = WASM_I32_VAL(0);
    wasm_val_copy(&results->data[0], &val);
    wasm_val_delete(&val);
    return nullptr;
  }
}

//void snes_bus_read(uint32_t i_address, uint8_t *i_data, uint32_t i_size);
wasm_binding(snes_bus_read, "v(i*i)") {
  m3ApiGetArg   (uint32_t, i_address)
  m3ApiGetArgMem(uint8_t*, o_data)
  m3ApiGetArg   (uint32_t, i_size)

  m3ApiCheckMem(o_data, i_size);

  for (uint32_t a = i_address, o = 0; o < i_size; o++, a++) {
    uint8_t b;
    b = SNES::bus.read(a);
    o_data[o] = b;
  }

  return nullptr;
}

//void snes_bus_write(uint32_t i_address, uint8_t *o_data, uint32_t i_size);
wasm_binding(snes_bus_write, "v(i*i)") {
  m3ApiGetArg   (uint32_t, i_address)
  m3ApiGetArgMem(uint8_t*, i_data)
  m3ApiGetArg   (uint32_t, i_size)

  m3ApiCheckMem(i_data, i_size);

  for (uint32_t a = i_address, o = 0; o < i_size; o++, a++) {
    uint8_t b = i_data[o];
    SNES::bus.write(a, b);
  }

  return nullptr;
}

// void frame_acquire(const uint16_t *io_data, uint32_t *o_pitch, uint32_t *o_width, uint32_t *o_height, bool *o_interlace)
wasm_binding(frame_acquire, "v(*****)") {
  m3ApiGetArgMem(uint16_t*, io_data)
  m3ApiGetArgMem(uint32_t*, o_pitch)
  m3ApiGetArgMem(uint32_t*, o_width)
  m3ApiGetArgMem(uint32_t*, o_height)
  m3ApiGetArgMem(bool*,     o_interlace)

  m3ApiCheckMem(io_data, frame.height * frame.pitch)

  m3ApiCheckMem(o_pitch, sizeof(uint32_t))
  m3ApiCheckMem(o_width, sizeof(uint32_t))
  m3ApiCheckMem(o_height, sizeof(uint32_t))
  m3ApiCheckMem(o_interlace, sizeof(int32_t))

  *o_pitch = frame.pitch;
  *o_width = frame.width;
  *o_height = frame.height;
  *o_interlace = frame.interlace;

  // copy from frame.data into wasm memory:
  memcpy(io_data, frame.data, frame.height * frame.pitch);

  return nullptr;
}

#undef wasm_binding
