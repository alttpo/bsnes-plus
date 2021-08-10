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

  // draw command list:
  if (!cmdlist.empty()) {
    memcpy(tmp, frame.data, 512 * 512 * sizeof(uint16_t));
    draw_list(tmp);
    frame.data = tmp;
    cmdlist.clear();
  }

  return frame.data;
}

enum draw_cmd {
  CMD_PIXEL,
  CMD_IMAGE,
  CMD_HLINE,
  CMD_VLINE,
  CMD_RECT,
  CMD_TEXT_UTF8,
  CMD_VRAM_TILE_2BPP,
  CMD_VRAM_TILE_4BPP,
  CMD_VRAM_TILE_8BPP
};

void WASMInterface::draw_list(uint16_t* data) {
  uint32_t  pitch = frame.pitch>>1;

  uint16_t* start = (uint16_t*) cmdlist.data();
  uint32_t  end = cmdlist.size()>>1;
  uint16_t* p = start;

  while ((p - start) < end) {
    // every command starts with the number of 16-bit words in length, including command, arguments, and inline data:
    uint16_t len = *p++;

    uint16_t* d = p;

    uint16_t cmd = *d++;

    uint16_t color, fillcolor;
    int16_t  x0, y0, w, h;
    switch (cmd) {
      case CMD_PIXEL:
        color = *d++;
        if (color >= 0x8000) {
          break;
        }

        x0 = *d++;
        y0 = *d++;
        data[(y0 * pitch) + x0] = color;
        break;
      case CMD_IMAGE:
        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        w  = (int16_t)*d++;
        h  = (int16_t)*d++;
        for (int16_t y = y0; y < y0+h; y++) {
          if (y < 0) continue;
          if (y >= frame.height) break;

          for (int16_t x = x0; x < x0+w; x++) {
            if (x < 0) continue;
            if (x >= frame.width) break;

            color = *d++;
            if (color >= 0x8000) continue;

            data[(y * pitch) + x] = color;
          }
        }
        break;
      case CMD_HLINE:
        color = *d++;
        if (color >= 0x8000) {
          break;
        }

        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        w  = (int16_t)*d++;

        draw_hline(data, x0, y0, w, color);
        break;
      case CMD_VLINE:
        color = *d++;
        if (color >= 0x8000) {
          break;
        }

        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        h  = (int16_t)*d++;

        draw_vline(data, x0, y0, h, color);
        break;
      case CMD_RECT:
        color = *d++;
        fillcolor = *d++;

        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;
        w  = (int16_t)*d++;
        h  = (int16_t)*d++;

        if (fillcolor < 0x8000) {
          for (int16_t y = y0; y < y0+h; y++) {
            if (y < 0) continue;
            if (y >= frame.height) break;

            for (int16_t x = x0; x < x0+w; x++) {
              if (x < 0) continue;
              if (x >= frame.width) break;

              data[(y * pitch) + x] = fillcolor;
            }
          }
        }
        if (color < 0x8000) {
          draw_hline(data, x0, y0, w, color);
          draw_hline(data, x0, y0+h, w, color);
          draw_vline(data, x0, y0, h, color);
          draw_vline(data, x0+w, y0, h, color);
        }
        break;
      }
    }

    p += len;
  }
}

void WASMInterface::draw_hline(uint16_t* data, int16_t x0, int16_t y0, int16_t w, uint16_t color) {
  if (y0 < 0) return;
  if (y0 >= frame.height) return;

  uint32_t pitch = frame.pitch>>1;
  for (int16_t x = x0; x < x0+w; x++) {
    if (x < 0) continue;
    if (x >= frame.width) return;

    data[(y0 * pitch) + x] = color;
  }
}
void WASMInterface::draw_vline(uint16_t* data, int16_t x0, int16_t y0, int16_t h, uint16_t color) {
  if (x0 < 0) return;
  if (x0 >= frame.width) return;

  uint32_t pitch = frame.pitch>>1;
  for (int16_t y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= frame.height) break;

    data[(y * pitch) + x0] = color;
  }
}

#include "wasm_bindings.cpp"
