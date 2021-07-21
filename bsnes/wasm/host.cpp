#include "host.hpp"

namespace WASM {

Host host(1024);

void Host::reset() {
  runtimes.clear();
}

void Host::add_module(const uint8_t *data, size_t size) {
  wasm3::module mod = env.parse_module(data, size);

  auto runtime = env.new_runtime(default_stack_size);
  runtime.load(mod);

  runtimes.push_back(runtime);
}

void Host::invoke_all(const char *name, std::function<void(wasm3::function&)> cb) {
  for (auto &runtime: runtimes) {
    auto func = runtime.find_function(name);
    cb(func);
  }
}

}
