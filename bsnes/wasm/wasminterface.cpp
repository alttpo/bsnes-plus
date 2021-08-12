#include <snes/snes.hpp>
#include <algorithm>
#include "wasminterface.hpp"

WASMInterface wasmInterface(WASM::host);

WASMInterface::WASMInterface(WASM::Host &host) : m_host(host) {}

void WASMInterface::register_debugger(const std::function<void()>& do_break, const std::function<void()>& do_continue) {
  m_do_break = do_break;
  m_do_continue = do_continue;
}

void WASMInterface::on_nmi() {
  m_host.each_runtime([&](const std::shared_ptr<WASM::Runtime>& runtime) {
    M3Result res = runtime->with_function("on_nmi", [&](WASM::Function &f) {
      M3Result res = f.callv(0);
      if (res != m3Err_none) {
        return;
      }
    });

    // warn about the result only once per runtime:
    runtime->warn(res, "on_nmi");
  });
}

const uint16_t *WASMInterface::on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace) {
  // call 'on_frame_present':
  m_host.each_runtime([&](const std::shared_ptr<WASM::Runtime>& runtime) {
    M3Result res = runtime->with_function("on_frame_present", [&](WASM::Function &f) {
      M3Result res = f.callv(0);
      if (res != m3Err_none) {
        return;
      }
    });

    // warn about the result only once per module:
    runtime->warn(res, "on_frame_present");
  });

  return data;
}

#include "pixelfont.cpp"
#include "drawlist.cpp"
#include "wasm_bindings.cpp"
