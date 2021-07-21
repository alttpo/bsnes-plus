#include "host.hpp"

namespace WASM {

Host host(1024);

static void check_error(M3Result err) {
  if (err != m3Err_none) {
    throw error(err);
  }
}

void Host::reset() {
  m_runtimes.clear();
  m_modules.clear();
}

void Host::add_module(const uint8_t *data, size_t size) {
  IM3Module p;
  M3Result err = m3_ParseModule(m_env.get(), &p, data, size);
  check_error(err);

  std::shared_ptr<struct M3Module> module;
  module.reset(p, [](IM3Module module){ m3_FreeModule(module); });
  if (module == nullptr) {
      throw std::bad_alloc();
  }

  std::shared_ptr<struct M3Runtime> runtime;
  runtime.reset(m3_NewRuntime(m_env.get(), default_stack_size_bytes, nullptr), &m3_FreeRuntime);
  if (runtime == nullptr) {
      throw std::bad_alloc();
  }

  err = m3_LoadModule(runtime.get(), module.get());
  check_error(err);

  m_modules.push_back(module);
  m_runtimes.push_back(runtime);
}

void Host::link(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall) {
  for (auto &module: m_modules) {
    m3_LinkRawFunction(module.get(), module_name, function_name, signature, rawcall);
  }
}

void Host::invoke_all(const char *name, int argc, const char* argv[]) {
  M3Result res;
  for (auto &runtime: m_runtimes) {
    M3Function *func;

    res = m3_FindFunction(&func, runtime.get(), name);
    check_error(res);

    res = m3_CallArgv(func, argc, argv);
    check_error(res);
  }
}

}
