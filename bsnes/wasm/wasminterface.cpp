#include <snes/snes.hpp>
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
  frame.data = data;
  frame.pitch = pitch;
  frame.width = width;
  frame.height = height;
  frame.interlace = interlace;

  m_host.each_runtime([&](const std::shared_ptr<WASM::Runtime>& runtime) {
    M3Result res = runtime->with_function("on_frame_present", [&](WASM::Function &f) {
      M3Result res = f.callv(0);
      if (res != m3Err_none) {
        return;
      }

      uint32_t bufOffset;
      res = f.resultsv(0, &bufOffset);
      if (res != m3Err_none) {
        return;
      }

      uint32_t size;
      const uint16_t *data = (const uint16_t *)runtime->memory(bufOffset, size);
      if (data) {
        // frame_acquire() copies the original frame data into wasm memory, so we
        // avoid a copy here and just present the (possibly modified) frame data
        // directly from wasm memory:
        frame.data = data;
      }
    });

    // warn about the result only once per module:
    runtime->warn(res, "on_frame_present");
  });

  return frame.data;
}

#include "wasm_bindings.cpp"
