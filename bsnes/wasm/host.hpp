#pragma once

#include <optional>

#include "wasm3_cpp.h"

namespace WASM {

struct Host {
  Host(size_t stack_size) : default_stack_size(stack_size), env() {}

  void reset();
  void add_module(const uint8_t *data, size_t size);
  void invoke_all(const char *name, std::function<void(wasm3::function&)> cb);

public:
  size_t default_stack_size;
  wasm3::environment env;

  std::vector<wasm3::runtime> runtimes;
};

extern Host host;

}
