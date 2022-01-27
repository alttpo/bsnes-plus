
bool WASMInstanceM3::_catch(M3Result err) {
  if (err == m3Err_none) {
    return false;
  }

  std::string functionName;
  uint32_t moduleOffset;

  // get error info:
  M3ErrorInfo errInfo;
  m3_GetErrorInfo(m_runtime, &errInfo);
  if (errInfo.function) {
     functionName = m3_GetFunctionName(errInfo.function);
  }

  // get backtrace info from last frame:
  IM3BacktraceInfo backtrace = m3_GetBacktrace(m_runtime);
  if (backtrace) {
    IM3BacktraceFrame lastFrame = backtrace->lastFrame;
    if (lastFrame) {
      functionName = m3_GetFunctionName(lastFrame->function);
      moduleOffset = lastFrame->moduleOffset;
    }
  }

  m_err = WASMError(
    err,
    m_key,
    errInfo.message,
    errInfo.file,
    errInfo.line,
    functionName,
    moduleOffset);

  return true;
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

    m_err = WASMError(err, m_key);
    return;
  }

  m3_SetModuleName(m_module, m_key.c_str());

  // load module:
  err = m3_LoadModule(m_runtime, m_module);
  if (_catch(err)) return;

  // link in libc API:
  err = m3_LinkLibC(m_module);
  if (_catch(err)) return;

  // link puts function:
  err = m3_LinkRawFunction(m_module, "env", "puts", "i(*)", m3puts);
  if (err == m3Err_functionLookupFailed) {
    err = nullptr;
  }
  if (_catch(err)) return;

  // link hexdump function:
  err = m3_LinkRawFunction(m_module, "env", "hexdump", "v(*i)", hexdump);
  if (err == m3Err_functionLookupFailed) {
    err = nullptr;
  }
  if (_catch(err)) return;

  link_module();

  //m3_PrintRuntimeInfo(m_runtime);
}

WASMInstanceM3::~WASMInstanceM3() {
  m3_FreeRuntime(m_runtime);
  m3_FreeEnvironment(m_env);
}

void WASMInstanceM3::link_module() {
  M3Result err;

// link wasm_bindings.cpp member functions:
#define wasm_link_full(name, namestr) \
  err = m3_LinkRawFunctionEx( \
    m_module,      \
    "*",           \
    namestr,       \
    wa_sig_##name, \
    [](IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem) -> const void* { \
      WASMInstanceM3 *self = ((WASMInstanceM3 *)_ctx->userdata); \
      return self->wa_fun_##name(_mem, _sp); \
    }, \
    (const void *)this \
  ); \
  if (err == m3Err_functionLookupFailed) { \
    err = m3Err_none; \
  } \
  if (_catch(err)) return;

#define wasm_link(name) wasm_link_full(name, #name)

  wasm_link_full(runtime_alloc, "runtime.alloc");

  wasm_link(debugger_break);
  wasm_link(debugger_continue);

  wasm_link(za_file_locate);
  wasm_link(za_file_size);
  wasm_link(za_file_extract);

  wasm_link(msg_recv);
  wasm_link(msg_size);

  wasm_link(snes_bus_read);
  wasm_link(snes_bus_write);

  wasm_link(ppux_spaces_reset);

  wasm_link(ppux_font_load_za);
  wasm_link(ppux_font_delete);

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

WASMFunctionM3::WASMFunctionM3(const std::string& name, IM3Function m3fn)
  : WASMFunction(name), m_fn(m3fn)
{
  if (!m3fn)
    throw std::invalid_argument("m3fn");
}

WASMFunctionM3::operator bool() const { return m_fn != nullptr; }

bool WASMInstanceM3::func_find(const std::string &i_name, std::shared_ptr<WASMFunction> &o_func) {
  IM3Function m3fn = nullptr;

  M3Result err;
  err = m3_FindFunction(&m3fn, m_runtime, i_name.c_str());
  if (err != m3Err_none) {
    // not found:
    if (_catch(err)) return false;
  }

  o_func = std::shared_ptr<WASMFunction>(new WASMFunctionM3(i_name, m3fn));
  return true;
}

bool WASMInstanceM3::func_invoke(const std::shared_ptr<WASMFunction>& fn, uint32_t i_retc, uint32_t i_argc, uint64_t *io_stack) {
  if (!(bool)fn) {
    //fprintf(stderr, "func_invoke() called with fn == nullptr\n");
    return false;
  }

  auto m3fn = (WASMFunctionM3*)fn.get();
  if (!*m3fn) {
    fprintf(stderr, "func_invoke() called with shared_ptr dereferencing to nullptr\n");
    return false;
  }

  const void** argptrs = new const void*[i_retc + i_argc];
  for (uint32_t i = 0; i < i_retc + i_argc; i++) {
    argptrs[i] = (const void*)&io_stack[i];
  }

  M3Result err;
  err = m3_Call(m3fn->m_fn, i_argc, argptrs + i_retc);
  if (_catch(err)) return false;

  err = m3_GetResults(m3fn->m_fn, i_retc, argptrs);
  if (_catch(err)) return false;

  delete[] argptrs;

  return true;
}

void WASMInstanceM3::warn() {
  if (!m_err)
    return;

  // prevent redundant warnings:
  const std::string &key = m_err.m_message;

  const std::set<std::string>::iterator &it = m_warnings.find(key);
  if (it != m_warnings.end())
    return;

  m_warnings.emplace_hint(it, key);

  WASMInstanceBase::warn();
}