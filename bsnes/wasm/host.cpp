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

Module::Module(const std::string& key, const std::shared_ptr<struct M3Environment>& env, size_t stack_size_bytes, const uint8_t *data, size_t size)
  : m_key(key), m_env(env)
{
  // own the wasm bytes ourselves:
  m_size = size;
  m_data = new uint8_t[m_size];
  memcpy((void *)m_data, (const void *)data, size);

  //printf("m3_ParseModule(%p, %p, %p, %zu)\n", env.get(), &p, m_data, m_size);
  M3Result err = m3_ParseModule(env.get(), &m_module, m_data, m_size);
  if (err != m3Err_none) {
    m3_FreeModule(m_module);
    m_module = nullptr;
    delete[] m_data;
    m_data = nullptr;
    throw error(err);
  }

  // create runtime for the module:
  m_runtime = m3_NewRuntime(m_env.get(), stack_size_bytes, nullptr);

  //printf("m3_LoadModule(%p, %p)\n", m_runtime, m_module);
  err = m3_LoadModule(m_runtime, m_module);
  if (err != m3Err_none) {
    m3_FreeRuntime(m_runtime);
    m_runtime = nullptr;
    m3_FreeModule(m_module);
    m_module = nullptr;
    delete[] m_data;
    m_data = nullptr;
    throw error(err);
  }
}

Module::~Module() {
  if (m_runtime) {
    m3_FreeRuntime(m_runtime);
    m_runtime = nullptr;

    m_module = nullptr;

    delete[] m_data;
    m_data = nullptr;
  }
}

M3Result Module::suppressFunctionLookupFailed(M3Result res, const char *function_name) {
  if (res == m3Err_functionLookupFailed) {
    const std::string key(function_name);
    auto it = m_function_warned.find(key);
    if (it == m_function_warned.end()) {
      fprintf(stderr, "wasm: module '%s': failed to find function '%s'\n", m_key.c_str(), function_name);
      m_function_warned.emplace_hint(it, key, true);
    }
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

M3Result Module::invoke(const char *function_name, int argc, const char *argv[]) {
  M3Result res;
  M3Function *func;

  //printf("  m3_FindFunction(%p, %p, '%s')\n", &func, m_runtime, function_name);
  res = m3_FindFunction(&func, m_runtime, function_name);
  suppressFunctionLookupFailed(res, function_name);
  if (res != m3Err_none) {
    return res;
  }

  //printf("  m3_CallArgv(%p, %d, %p)\n", func, argc, argv);
  res = m3_CallArgv(func, argc, argv);
  if (res != m3Err_none) {
    return res;
  }

  return m3Err_none;
}

void Module::msg_enqueue(const std::shared_ptr<Message>& msg) {
  //printf("msg_enqueue(%p, %u)\n", msg->m_data, msg->m_size);
  m_msgs.push(msg);

  const char *function_name = "on_msg_recv";
  M3Result res = invoke(function_name, 0, nullptr);
  res = suppressFunctionLookupFailed(res, function_name);
  check_error(res);
}

std::shared_ptr<Message> Module::msg_dequeue() {
  auto msg = m_msgs.front();
  //printf("msg_dequeue() -> {%p, %u}\n", msg->m_data, msg->m_size);
  m_msgs.pop();
  return msg;
}

bool Module::msg_size(uint16_t *o_size) {
  if (m_msgs.empty()) {
    //printf("msg_size() -> false\n");
    return false;
  }

  *o_size = m_msgs.front()->m_size;
  //printf("msg_size() -> true, %u\n", *o_size);
  return true;
}


void Host::reset() {
  m_modules.clear();
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

std::shared_ptr<Module> Host::parse_module(const std::string &key, const uint8_t *data, size_t size) {
  std::shared_ptr<Module> m_module;
  m_module.reset(new Module(key, m_env, default_stack_size_bytes, data, size));

  // link in libc API:
  M3Result res = m3_LinkLibC(m_module->m_module);
  check_error(res);

  // link hexdump function:
  res = m3_LinkRawFunction(m_module->m_module, "env", "hexdump", "v(*i)", hexdump);
  res = m_module->suppressFunctionLookupFailed(res, "hexdump");
  check_error(res);

  return m_module;
}

void Host::load_module(const std::shared_ptr<Module>& module) {
  unload_module(module->m_key);

  //printf("load_module()\n");
  m_modules.emplace(module->m_key, module);
  m_modules_by_ptr.emplace(module->m_module, module);
}

void Host::unload_module(const std::string &key) {
  //printf("load_module()\n");
  auto it = m_modules.find(key);
  if (it == m_modules.end()) return;

  m_modules_by_ptr.erase(it->second->m_module);
  m_modules.erase(it);
}

void Host::invoke_all(const char *name, int argc, const char**argv) {
  M3Result res;
  //printf("invoke_all('%s', %d, %p)\n", name, argc, argv);

  for(std::map<std::string, std::shared_ptr<Module>>::iterator it = m_modules.begin(); it != m_modules.end(); ++it) {
    res = it->second->invoke(name, argc, argv);
    check_error(res);
  }
}

std::shared_ptr<Module> Host::get_module(const std::string& key) {
  return m_modules.at(key);
}

std::shared_ptr<Module> Host::get_module(IM3Module module) {
  return m_modules_by_ptr.at(module);
}

}
