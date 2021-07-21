#include "host.hpp"

namespace WASM {

Host host(1024);

void Host::reset() {
  m_runtimes.clear();
}

void Host::add_module(const uint8_t *data, size_t size) {
  IM3Module module;
  M3Result err = m3_ParseModule(m_env.get(), &module, data, size);

  std::shared_ptr<M3Runtime> runtime;
  runtime.reset(m3_NewRuntime(m_env.get(), default_stack_size_bytes, nullptr), &m3_FreeRuntime);
  if (runtime == nullptr) {
      throw std::bad_alloc();
  }

  err = m3_LoadModule(runtime.get(), module);

  m_runtimes.push_back(runtime);
}

void Host::invoke_all(const char *name, std::function<void(M3Function*)> cb) {
  for (auto &runtime: m_runtimes) {
    M3Function *func;
    M3Result err = m3_FindFunction(&func, runtime.get(), name);
    cb(func);
  }
}

}
