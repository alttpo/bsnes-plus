#include "host.hpp"

namespace WASM {

Host host(1024);

static void check_error(M3Result err) {
  if (err != m3Err_none) {
    throw error(err);
  }
}

Module::Module(const std::shared_ptr<struct M3Environment>& env, const uint8_t *data, size_t size) {
  m_env = env;

  // own the wasm bytes ourselves:
  m_size = size;
  m_data = new uint8_t[m_size];
  memcpy((void *)m_data, (const void *)data, size);

  IM3Module p;
  M3Result err = m3_ParseModule(env.get(), &p, m_data, m_size);
  check_error(err);

  m_module.reset(p, [this](IM3Module p) {
    /* do nothing; module is freed by runtime? */
    //printf("free module:  %p\n", p);
    if (m_loaded) {
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
}

Module::Module(Module&& other) {
  //printf("Module::move %p -> %p\n", &other, this);
  m_env = other.m_env;
  m_size = other.m_size;
  m_data = std::move(other.m_data);
  m_module = std::move(other.m_module);
  m_loaded = other.m_loaded;
}

void Module::load_into(const std::shared_ptr<struct M3Runtime>& runtime) {
  //printf("m3_LoadModule(%p, %p)\n", runtime.get(), m_module.get());
  M3Result err = m3_LoadModule(runtime.get(), m_module.get());
  check_error(err);

  m_loaded = true;
}

void Module::link(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall) {
  M3Result res = m3_LinkRawFunction(m_module.get(), module_name, function_name, signature, rawcall);
  check_error(res);
}

void Module::linkEx(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall, const void *userdata) {
  M3Result res = m3_LinkRawFunctionEx(m_module.get(), module_name, function_name, signature, rawcall, userdata);
  check_error(res);
}


void Host::reset() {
  m_runtimes.clear();
  m_modules.clear();
}

void Host::add_module(const uint8_t *data, size_t size) {
  Module module(m_env, data, size);

  std::shared_ptr<struct M3Runtime> runtime;
  runtime.reset(m3_NewRuntime(m_env.get(), default_stack_size_bytes, nullptr), [](IM3Runtime runtime){
    //printf("free runtime: %p\n", runtime);

    m3_FreeRuntime(runtime);

    // TODO: delete[] m_data
  });
  if (runtime == nullptr) {
    throw std::bad_alloc();
  }

  module.load_into(runtime);

  m_modules.push_back(std::move(module));
  m_runtimes.push_back(std::move(runtime));
}

void Host::link(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall) {
  for (auto &module: m_modules) {
    module.link(module_name, function_name, signature, rawcall);
  }
}

void Host::linkEx(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall, const void *userdata) {
  for (auto &module: m_modules) {
    module.linkEx(module_name, function_name, signature, rawcall, userdata);
  }
}

void Host::invoke_all(const char *name, int argc, const char**argv) {
  M3Result res;
  //printf("invoke_all('%s', %d, %p)\n", name, argc, argv);

  for (auto &runtime: m_runtimes) {
    M3Function *func;

    //printf("  m3_FindFunction(%p, %p, '%s')\n", &func, runtime.get(), name);
    res = m3_FindFunction(&func, runtime.get(), name);
    check_error(res);

    //printf("  m3_CallArgv(%p, %d, %p)\n", func, argc, argv);
    res = m3_CallArgv(func, argc, argv);
    check_error(res);
  }
}

}
