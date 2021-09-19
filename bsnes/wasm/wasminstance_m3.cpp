
static void check_error(M3Result err) {
  if (err != m3Err_none) {
    throw std::runtime_error(err);
  }
}

m3ApiRawFunction(hexdump) {
  m3ApiGetArgMem(const uint8_t*, i_data)
  m3ApiGetArg   (uint32_t, i_size)

  for (uint32_t i = 0; i < i_size; i += 16) {
    char line[8 + 2 + (3 * 16) + 2];
    int n;
    n = sprintf(line, "%08x ", i);
    for (uint32_t j = 0; j < 16; j++) {
      if (i + j >= i_size) break;
      n += sprintf(&line[n], "%02x ", i_data[i + j]);
    }
    line[n - 1] = 0;
    puts(line);
  }

  m3ApiSuccess();
}

m3ApiRawFunction(m3puts) {
  m3ApiReturnType (int32_t)

  m3ApiGetArgMem  (const char*, i_str)

  if (m3ApiIsNullPtr(i_str)) {
    m3ApiReturn(0);
  }

  m3ApiCheckMem(i_str, 1);
  size_t fmt_len = strnlen(i_str, ((uintptr_t) (_mem) + m3_GetMemorySize(runtime)) - (uintptr_t) (i_str));
  m3ApiCheckMem(i_str, fmt_len + 1); // include `\0`

  // use fputs to avoid redundant newline
  m3ApiReturn(fputs(i_str, stdout));
}

WASMInstanceM3::WASMInstanceM3(WASMInterface* interface, const std::string &key, const std::shared_ptr<ZipArchive> &za)
  : WASMInstanceBase(interface, key, za) {
  m_env = m3_NewEnvironment();
  m_runtime = m3_NewRuntime(m_env, stack_size_bytes, (void *) this);

  M3Result err;

  //printf("m3_ParseModule(%p, %p, %p, %zu)\n", m_env, &m_module, m_data, m_size);
  err = m3_ParseModule(m_env, &m_module, m_data, m_size);
  if (err != m3Err_none) {
    m3_FreeModule(m_module);
    m_module = nullptr;
    throw std::runtime_error(err);
  }

  m3_SetModuleName(m_module, m_key.c_str());

  // load module:
  err = m3_LoadModule(m_runtime, m_module);
  check_error(err);

  // link in libc API:
  err = m3_LinkLibC(m_module);
  check_error(err);

  // link puts function:
  err = m3_LinkRawFunction(m_module, "env", "puts", "i(*)", m3puts);
  if (err == m3Err_functionLookupFailed) {
    err = nullptr;
  }
  check_error(err);

  // link hexdump function:
  err = m3_LinkRawFunction(m_module, "env", "hexdump", "v(*i)", hexdump);
  if (err == m3Err_functionLookupFailed) {
    err = nullptr;
  }
  check_error(err);

  link_module();
}

WASMInstanceM3::~WASMInstanceM3() {
  m3_FreeRuntime(m_runtime);
  m3_FreeEnvironment(m_env);
}

void WASMInstanceM3::link_module() {
  M3Result err;

// link wasm_bindings.cpp member functions:
#define wasm_link(name) \
  err = m3_LinkRawFunctionEx( \
    m_module,      \
    "*",           \
    #name,         \
    wa_sig_##name, \
    [](IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem) -> const void* { \
      WASMInstanceM3 *self = ((WASMInstanceM3 *)_ctx->userdata); \
      try { \
        return self->wa_fun_##name(_mem, _sp); \
      } catch (std::runtime_error& e) { \
        return e.what(); \
      } \
    }, \
    (const void *)this \
  ); \
  if (err == m3Err_functionLookupFailed) { \
    err = m3Err_none; \
  } \
  check_error(err);

  wasm_link(debugger_break);
  wasm_link(debugger_continue);

  wasm_link(msg_recv);
  wasm_link(msg_size);

  wasm_link(snes_bus_read);
  wasm_link(snes_bus_write);

  wasm_link(ppux_spaces_reset);
  wasm_link(ppux_draw_list_reset);
  wasm_link(ppux_draw_list_append);

  wasm_link(ppux_vram_write);
  wasm_link(ppux_cgram_write);
  wasm_link(ppux_oam_write);

  wasm_link(ppux_vram_read);
  wasm_link(ppux_cgram_read);
  wasm_link(ppux_oam_read);

#undef wasm_link
}

uint64_t WASMInstanceM3::memory_size() {
  return m3_GetMemorySize(m_runtime);
}

void WASMInstanceM3::invoke(const std::string &i_name, uint64_t *io_stack) {
  // TODO
  throw std::runtime_error("TODO");
}
