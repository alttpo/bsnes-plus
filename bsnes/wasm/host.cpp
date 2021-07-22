#include "host.hpp"

namespace WASM {

Host host(1024);

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

  IM3Module p;
  //printf("m3_ParseModule(%p, %p, %p, %zu)\n", env.get(), &p, m_data, m_size);
  M3Result err = m3_ParseModule(env.get(), &p, m_data, m_size);
  check_error(err);

  // create module:
  m_module.reset(p, [this](IM3Module p) {
    //printf("free module:  %p\n", p);
    if (m_loaded) {
      /* do nothing; module is freed by runtime */
      return;
    }

    //printf("m3_FreeModule(%p)\n", p);
    m3_FreeModule(p);
    if (m_data) {
      //printf("m3_FreeModule(%p): delete[] m_data\n", p);
      delete[] m_data;
    }
    m_data = nullptr;
  });

  if (m_module == nullptr) {
    throw std::bad_alloc();
  }

  // create runtime for the module:
  m_runtime.reset(m3_NewRuntime(m_env.get(), stack_size_bytes, nullptr), [](IM3Runtime runtime){
    //printf("free runtime: %p\n", runtime);

    m3_FreeRuntime(runtime);

    // TODO: delete[] m_data
  });
  if (m_runtime == nullptr) {
    throw std::bad_alloc();
  }

  //printf("m3_LoadModule(%p, %p)\n", m_runtime.get(), m_module.get());
  err = m3_LoadModule(m_runtime.get(), m_module.get());
  check_error(err);

  m_loaded = true;
}

Module::Module(Module&& other) {
  //printf("Module::move %p -> %p\n", &other, this);
  m_env = (other.m_env);
  m_size = other.m_size;
  m_data = (other.m_data);
  m_module = (other.m_module);
  m_runtime = (other.m_runtime);
  m_loaded = other.m_loaded;
}

void Module::link(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall) {
  //printf("m3_LinkRawFunction(%p, '%s', '%s', '%s', %p)\n", m_module.get(), module_name, function_name, signature, rawcall);
  M3Result res = m3_LinkRawFunction(m_module.get(), module_name, function_name, signature, rawcall);
  if (res == m3Err_functionLookupFailed) {
    fprintf(stderr, "wasm: error while linking '%s': %s\n", function_name, res);
    res = NULL;
  }
  check_error(res);
}

void Module::linkEx(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall, const void *userdata) {
  //printf("m3_LinkRawFunctionEx(%p, '%s', '%s', '%s', %p, %p)\n", m_module.get(), module_name, function_name, signature, rawcall, userdata);
  M3Result res = m3_LinkRawFunctionEx(m_module.get(), module_name, function_name, signature, rawcall, userdata);
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

    //printf("  m3_FindFunction(%p, %p, '%s')\n", &func, runtime.get(), name);
    res = m3_FindFunction(&func, it->second.m_runtime.get(), name);
    check_error(res);

    //printf("  m3_CallArgv(%p, %d, %p)\n", func, argc, argv);
    res = m3_CallArgv(func, argc, argv);
    check_error(res);
  }
}

}
