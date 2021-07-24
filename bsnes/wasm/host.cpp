#include "host.hpp"

namespace WASM {

Host host(1024 * 1024);

static void check_error(M3Result err) {
  if (err != m3Err_none) {
    throw error(err);
  }
}

Module::Module(const std::shared_ptr<struct M3Environment>& env, size_t stack_size_bytes, const uint8_t *data, size_t size) {
  m_env = env;

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

Module::Module(Module&& other) {
  //printf("Module::move %p -> %p\n", &other, this);
  m_env = other.m_env;
  m_size = other.m_size;
  m_data = other.m_data;
  m_module = other.m_module;
  m_runtime = other.m_runtime;

  other.m_data = nullptr;
  other.m_module = nullptr;
  other.m_runtime = nullptr;
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

void Module::link(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall) {
  //printf("m3_LinkRawFunction(%p, '%s', '%s', '%s', %p)\n", m_module, module_name, function_name, signature, rawcall);
  M3Result res = m3_LinkRawFunction(m_module, module_name, function_name, signature, rawcall);
  if (res == m3Err_functionLookupFailed) {
    fprintf(stderr, "wasm: error while linking '%s': %s\n", function_name, res);
    res = NULL;
  }
  check_error(res);
}

void Module::linkEx(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall, const void *userdata) {
  //printf("m3_LinkRawFunctionEx(%p, '%s', '%s', '%s', %p, %p)\n", m_module, module_name, function_name, signature, rawcall, userdata);
  M3Result res = m3_LinkRawFunctionEx(m_module, module_name, function_name, signature, rawcall, userdata);
  if (res == m3Err_functionLookupFailed) {
    fprintf(stderr, "wasm: error while linking '%s': %s\n", function_name, res);
    res = NULL;
  }
  check_error(res);
}


void Host::reset() {
  m_modules.clear();
}

Module Host::parse_module(const uint8_t *data, size_t size) {
  return Module(m_env, default_stack_size_bytes, data, size);
}

void Host::load_module(const std::string &key, Module &module) {
  m_modules.erase(key);
  //printf("load_module()\n");
  m_modules.emplace(key, std::move(module));
}

void Host::unload_module(const std::string &key) {
  //printf("load_module()\n");
  m_modules.erase(key);
}

void Host::invoke_all(const char *name, int argc, const char**argv) {
  M3Result res;
  //printf("invoke_all('%s', %d, %p)\n", name, argc, argv);

  for(std::map<std::string, Module>::iterator it = m_modules.begin(); it != m_modules.end(); ++it) {
    M3Function *func;

    //printf("  m3_FindFunction(%p, %p, '%s')\n", &func, runtime, name);
    res = m3_FindFunction(&func, it->second.m_runtime, name);
    check_error(res);

    //printf("  m3_CallArgv(%p, %d, %p)\n", func, argc, argv);
    res = m3_CallArgv(func, argc, argv);
    check_error(res);
  }
}

}
