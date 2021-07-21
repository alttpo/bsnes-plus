#pragma once

#include <optional>
#include <vector>

#include "wasm3.h"

namespace WASM {

struct error : public std::runtime_error {
  explicit error(M3Result err) : std::runtime_error(err) {}
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
  void invoke_all(const char *name, int argc, const char* argv[]);

public:
  size_t default_stack_size_bytes;

  std::shared_ptr<struct M3Environment>           m_env;
  std::vector<std::shared_ptr<struct M3Runtime>>  m_runtimes;
  std::vector<std::shared_ptr<struct M3Module>>   m_modules;
};

extern Host host;

}
