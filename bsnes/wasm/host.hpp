#pragma once

#include <optional>
#include <vector>

#include "wasm3.h"

namespace WASM {

struct error : public std::runtime_error {
  explicit error(M3Result err) : std::runtime_error(err) {}
};

template <class T>
struct RawCall;

template <class T>
struct RawCall {
  template <const void * (T::*Func) (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem)>
  static const void * adapter(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem) {
    return (reinterpret_cast<T*>(_ctx->userdata)->*Func)(runtime, _ctx, _sp, _mem);
  }
};

struct Module {
  Module(const std::shared_ptr<struct M3Environment>& env, const uint8_t *data, size_t size);
  Module(Module&&);

  void load_into(const std::shared_ptr<struct M3Runtime>& runtime);
  void link(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall);
  void linkEx(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall, const void *userdata);

  std::shared_ptr<struct M3Environment> m_env;
  const uint8_t *m_data;
  size_t m_size;
  std::shared_ptr<struct M3Module> m_module;
  bool m_loaded;
};

struct Host {
  Host(size_t stack_size_bytes) : default_stack_size_bytes(stack_size_bytes) {
    m_env.reset(m3_NewEnvironment(), m3_FreeEnvironment);
    if (m_env == nullptr) {
      throw std::bad_alloc();
    }
  }

  void reset();
  void add_module(const uint8_t *data, size_t size);
  void link(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall);
  void linkEx(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall, const void *userdata);
  void invoke_all(const char *name, int argc, const char* argv[]);

public:
  size_t default_stack_size_bytes;

  std::shared_ptr<struct M3Environment>           m_env;
  std::vector<std::shared_ptr<struct M3Runtime>>  m_runtimes;
  std::vector<Module>                             m_modules;
};

extern Host host;

}
