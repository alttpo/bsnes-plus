#include "wasminterface.hpp"

WASMInterface wasmInterface;

const uint16_t *WASMInterface::on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace) {
  frame.data = data;
  frame.pitch = pitch;
  frame.width = width;
  frame.height = height;
  frame.interlace = interlace;

  WASM::host.each_module([&](const std::shared_ptr<WASM::Module>& module) {
    M3Result res = module->with_function("on_frame_present", [&](WASM::Function &f) {
      M3Result res = f.callv(0);
      if (res != m3Err_none) {
        printf("on_frame_present: callv: %s\n", res);
        return;
      }

      uint32_t bufOffset;
      res = f.resultsv(0, &bufOffset);
      if (res != m3Err_none) {
        printf("on_frame_present: resultsv: %s\n", res);
        return;
      }

      uint32_t size;
      const uint16_t *data = (const uint16_t *)module->memory(bufOffset, size);
      if (data) {
        // frame_acquire() copies the original frame data into wasm memory, so we
        // avoid a copy here and just present the (possibly modified) frame data
        // directly from wasm memory:
        frame.data = data;
      }
    });

    // warn about the result only once per module:
    module->warn(res, "on_frame_present");
  });

  return frame.data;
}

#include "wasm_bindings.cpp"
