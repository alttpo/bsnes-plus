#include "host.hpp"
#include "m3_api_libc.h"

namespace WASM {

Host host(1024 * 1024);

Message::Message(const uint8_t * const data, uint16_t size) : m_data(new uint8_t[size]), m_size(size) {
  memcpy((void *)m_data, (const void *)data, size);
}

Message::~Message() {
  delete[] m_data;
}

static void check_error(M3Result err) {
  if (err != m3Err_none) {
    throw error(err);
  }
}

Function::Function(IM3Function function) : m_function(function) {}

M3Result Function::dumpBacktrace(M3Result res) {
  if (res == m3Err_none) {
    return res;
  }

  //M3ErrorInfo errorInfo;
  //m3_GetErrorInfo(runtime->m_runtime, &errorInfo);
  //if (errorInfo.file || errorInfo.message) {
  //  printf("  %s(%d): %s\n", errorInfo.file, errorInfo.line, errorInfo.message);
  //}

  IM3Module module = m3_GetFunctionModule(m_function);
  IM3Runtime runtime = m3_GetModuleRuntime(module);

  fprintf(stderr, "callv(%s.%s): %s\n", m3_GetModuleName(module), m3_GetFunctionName(m_function), res);

  IM3BacktraceInfo bt = m3_GetBacktrace(runtime);
  if (bt) {
    IM3BacktraceFrame frame = bt->frames;
    while (frame) {
      IM3Module module = m3_GetFunctionModule(frame->function);
      fprintf(stderr, "  %s.%s; offs %06x\n", m3_GetModuleName(module), m3_GetFunctionName(frame->function), frame->moduleOffset);
      frame = frame->next;
    }
  }

  return res;
}

M3Result Function::callv(int dummy, ...) {
  va_list va;
  va_start(va, dummy);
  M3Result res = m3_CallVL(m_function, va);
  va_end(va);

  dumpBacktrace(res);

  return res;
}

M3Result Function::resultsv(int dummy, ...) {
  va_list va;
  va_start(va, dummy);
  M3Result res = m3_GetResultsVL(m_function, va);
  va_end(va);

  return res;
}

M3Result Function::callargv(int argc, const char * argv[]) {
  M3Result res = m3_CallArgv(m_function, argc, argv);

  dumpBacktrace(res);

  return res;
}

//////

Module::Module(const std::string& key, const Runtime& runtime, const uint8_t *data, size_t size)
  : m_key(key), m_runtime(runtime)
{
  // own the wasm bytes ourselves:
  m_size = size;
  m_data = new uint8_t[m_size];
  memcpy((void *)m_data, (const void *)data, size);

  //printf("m3_ParseModule(%p, %p, %p, %zu)\n", env.get(), &p, m_data, m_size);
  M3Result err = m3_ParseModule(runtime.m_env.get(), &m_module, m_data, m_size);
  if (err != m3Err_none) {
    m3_FreeModule(m_module);
    m_module = nullptr;
    delete[] m_data;
    m_data = nullptr;
    throw error(err);
  }

  m3_SetModuleName(m_module, m_key.c_str());

  M3Result res = m3_LoadModule(runtime.m_runtime, m_module);
  check_error(res);
}

Module::~Module() {
  // TODO: delete this after m3_FreeRuntime()
  //delete[] m_data;
  //m_data = nullptr;
}

M3Result Module::warn(M3Result res, const char *function_name) {
  if (res == m3Err_none) {
    return res;
  }

  std::string key(function_name);
  key.append(res);

  auto it = m_function_warned.find(key);
  if (it == m_function_warned.end()) {
    fprintf(stderr, "wasm: module '%s': function '%s': %s\n", m_key.c_str(), function_name, res);
    m_function_warned.emplace_hint(it, key, true);
  }

  return res;
}

M3Result Module::suppressFunctionLookupFailed(M3Result res, const char *function_name) {
  if (res == m3Err_functionLookupFailed) {
    warn(res, function_name);
    return m3Err_none;
  }

  return res;
}

void Module::link(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall) {
  //printf("m3_LinkRawFunction(%p, '%s', '%s', '%s', %p)\n", m_module, module_name, function_name, signature, rawcall);
  M3Result res = m3_LinkRawFunction(m_module, module_name, function_name, signature, rawcall);
  res = suppressFunctionLookupFailed(res, function_name);
  check_error(res);
}

void Module::linkEx(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall, const void *userdata) {
  //printf("m3_LinkRawFunctionEx(%p, '%s', '%s', '%s', %p, %p)\n", m_module, module_name, function_name, signature, rawcall, userdata);
  M3Result res = m3_LinkRawFunctionEx(m_module, module_name, function_name, signature, rawcall, userdata);
  res = suppressFunctionLookupFailed(res, function_name);
  check_error(res);
}

bool Module::get_global(const char * const i_globalName, IM3TaggedValue o_value) {
  IM3Module io_module = m_module;
  IM3Global global = m3_FindGlobal(io_module, i_globalName);
  if (!global) {
    return false;
  }
  M3Result err = m3_GetGlobal(global, o_value);
  if (err != m3Err_none) {
    return false;
  }
  return true;
}

////////////////////////

Runtime::Runtime(const std::string& key, const std::shared_ptr<struct M3Environment>& env, size_t stack_size_bytes)
  : m_key(key), m_env(env)
{
  m_runtime = m3_NewRuntime(env.get(), stack_size_bytes, (void *)this);
}

Runtime::~Runtime() {
  m_modules.clear();
  m_modules_by_ptr.clear();

  m3_FreeRuntime(m_runtime);
  m_runtime = nullptr;
}

M3Result Runtime::warn(M3Result res, const char *function_name) {
  if (res == m3Err_none) {
    return res;
  }

  std::string key(function_name);
  key.append(res);

  auto it = m_function_warned.find(key);
  if (it == m_function_warned.end()) {
    fprintf(stderr, "wasm: runtime '%s': function '%s': %s\n", m_key.c_str(), function_name, res);
    m_function_warned.emplace_hint(it, key, true);
  }

  return res;
}

M3Result Runtime::suppressFunctionLookupFailed(M3Result res, const char *function_name) {
  if (res == m3Err_functionLookupFailed) {
    warn(res, function_name);
    return m3Err_none;
  }

  return res;
}

uint8_t *Runtime::memory(uint32_t i_offset, uint32_t &o_size) {
  if (i_offset == 0) {
    return nullptr;
  }

  uint8_t *mem = m3_GetMemory(m_runtime, &o_size, 0);
  if (i_offset >= o_size) {
    return nullptr;
  }

  return mem + i_offset;
}

M3Result Runtime::with_function(const char *function_name, const std::function<void(Function&)>& with) {
  M3Result res;
  M3Function *func;

  //printf("with_function: m3_FindFunction(%p, %p, '%s')\n", &func, m_runtime, function_name);
  res = m3_FindFunction(&func, m_runtime, function_name);
  if (res != m3Err_none) {
    return res;
  }

  Function fn(func);

  with(fn);

  return m3Err_none;
}

void Runtime::msg_enqueue(const std::shared_ptr<Message>& msg) {
  //printf("msg_enqueue(%p, %u)\n", msg->m_data, msg->m_size);
  m_msgs.push(msg);

  const char *function_name = "on_msg_recv";
  M3Result res = with_function(function_name, [](Function& f) {
    f.callv(0);
  });
  warn(res, function_name);
}

std::shared_ptr<Message> Runtime::msg_dequeue() {
  auto msg = m_msgs.front();
  //printf("msg_dequeue() -> {%p, %u}\n", msg->m_data, msg->m_size);
  m_msgs.pop();
  return msg;
}

bool Runtime::msg_size(uint16_t *o_size) {
  if (m_msgs.empty()) {
    //printf("msg_size() -> false\n");
    return false;
  }

  *o_size = m_msgs.front()->m_size;
  //printf("msg_size() -> true, %u\n", *o_size);
  return true;
}

m3ApiRawFunction(hexdump) {
  m3ApiGetArgMem(const uint8_t*, i_data)
  m3ApiGetArg   (uint32_t,       i_size)

  for (uint32_t i = 0; i < i_size; i += 16) {
    char line[8+2+(3*16)+2];
    int n;
    n = sprintf(line, "%08x ", i);
    for (uint32_t j = 0; j < 16; j++) {
      if (i+j >= i_size) break;
      n += sprintf(&line[n], "%02x ", i_data[i + j]);
    }
    line[n-1] = 0;
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
  size_t fmt_len = strnlen(i_str, ((uintptr_t)(_mem) + m3_GetMemorySize(runtime)) - (uintptr_t)(i_str));
  m3ApiCheckMem(i_str, fmt_len+1); // include `\0`

  // use fputs to avoid redundant newline
  m3ApiReturn(fputs(i_str, stdout));
}

std::shared_ptr<Module> Runtime::parse_module(const std::string &key, const uint8_t *data, size_t size) {
  std::shared_ptr<Module> module(new Module(key, *this, data, size));

  return module;
}

void Runtime::load_module(const std::shared_ptr<Module>& module) {
  // link in libc API:
  M3Result res = m3_LinkLibC(module->m_module);
  check_error(res);

  // link puts function:
  res = m3_LinkRawFunction(module->m_module, "env", "puts", "i(*)", m3puts);
  res = suppressFunctionLookupFailed(res, "puts");
  check_error(res);

  // link hexdump function:
  res = m3_LinkRawFunction(module->m_module, "env", "hexdump", "v(*i)", hexdump);
  res = suppressFunctionLookupFailed(res, "hexdump");
  check_error(res);

  //printf("load_module()\n");
  m_modules.emplace(module->m_key, module);
  m_modules_by_ptr.emplace(module->m_module, module);
}

std::shared_ptr<Module> Runtime::get_module(const std::string& key) {
  return m_modules.at(key);
}

std::shared_ptr<Module> Runtime::get_module(IM3Module module) {
  return m_modules_by_ptr.at(module);
}

void Runtime::each_module(const std::function<void(const std::shared_ptr<Module>&)>& each) {
  for(std::map<std::string, std::shared_ptr<Module>>::iterator it = m_modules.begin(); it != m_modules.end(); ++it) {
    each(it->second);
  }
}

//
Host::Host(size_t stack_size_bytes) : default_stack_size_bytes(stack_size_bytes) {
  m_env.reset(m3_NewEnvironment(), m3_FreeEnvironment);
  if (m_env == nullptr) {
    throw std::bad_alloc();
  }
}

void Host::reset() {
  m_runtimes.clear();
  m_runtimes_by_ptr.clear();
}

std::shared_ptr<Runtime> Host::get_runtime(const std::string& key) {
  auto it = m_runtimes.find(key);
  if (it == m_runtimes.end()) {
    return {};
  } else {
    return it->second;
  }
}

std::shared_ptr<Runtime> Host::get_runtime(const IM3Runtime runtime) {
  auto it = m_runtimes_by_ptr.find(runtime);
  if (it == m_runtimes_by_ptr.end()) {
    return {};
  } else {
    return it->second;
  }
}

void Host::with_runtime(const std::string& key, const std::function<void(const std::shared_ptr<Runtime>&)>& with) {
  auto it = m_runtimes.find(key);
  if (it == m_runtimes.end()) {
    std::shared_ptr<Runtime> runtime(new Runtime(key, m_env, default_stack_size_bytes));
    m_runtimes.emplace(key, runtime);
    with(runtime);
  } else {
    with(it->second);
  }
}

void Host::each_runtime(const std::function<void(const std::shared_ptr<Runtime>&)>& each) {
  for(std::map<std::string, std::shared_ptr<Runtime>>::iterator it = m_runtimes.begin(); it != m_runtimes.end(); ++it) {
    each(it->second);
  }
}

void Host::unload_runtime(const std::string& key) {
  auto it = m_runtimes.find(key);
  if (it == m_runtimes.end()) return;

  m_runtimes_by_ptr.erase(it->second->m_runtime);
  m_runtimes.erase(key);
}

}
