#include <snes/snes.hpp>
#include <algorithm>
#include "wasminterface.hpp"
#include "utf8decoder.cpp"

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

        draw_pixel(data, x0, y0, color);
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
          for (int16_t y = y0+1; y < y0+h-1; y++) {
            if (y < 0) continue;
            if (y >= frame.height) break;

            for (int16_t x = x0+1; x < x0+w-1; x++) {
              if (x < 0) continue;
              if (x >= frame.width) break;

              data[(y * pitch) + x] = fillcolor;
            }
          }
        }
        if (color < 0x8000) {
          draw_hline(data, x0, y0, w, color);
          draw_hline(data, x0, y0+h-1, w, color);
          draw_vline(data, x0, y0, h, color);
          draw_vline(data, x0+w-1, y0, h, color);
        }
        break;
      case CMD_TEXT_UTF8: {
        uint8_t gw, gh;

        // stroke color:
        color = *d++;
        // 1px outline color:
        uint16_t outlinecolor = *d++;

        // select font:
        uint16_t fontindex = *d++;
        if (fontindex >= fonts.size()) {
          break;
        }
        const auto& font = *fonts[fontindex];

        x0 = (int16_t)*d++;
        y0 = (int16_t)*d++;

        if (outlinecolor < 0x8000) {
          draw_text_utf8((uint8_t*)d, x0, y0, font, [=](int rx, int ry) {
            draw_pixel(data, rx-1, ry-1, outlinecolor);
            draw_pixel(data, rx, ry-1, outlinecolor);
            draw_pixel(data, rx+1, ry-1, outlinecolor);
            draw_pixel(data, rx-1, ry, outlinecolor);
            draw_pixel(data, rx+1, ry, outlinecolor);
            draw_pixel(data, rx-1, ry+1, outlinecolor);
            draw_pixel(data, rx, ry+1, outlinecolor);
            draw_pixel(data, rx+1, ry+1, outlinecolor);
          });
        }
        if (color < 0x8000) {
          draw_text_utf8((uint8_t*)d, x0, y0, font, [=](int rx, int ry) {
            draw_pixel(data, rx, ry, color);
          });
        }

        break;
      }
    }

    p += len;
  }
}

inline void WASMInterface::draw_pixel(uint16_t* data, int16_t x0, int16_t y0, uint16_t color) {
  if (y0 < 0) return;
  if (y0 >= frame.height) return;
  if (x0 < 0) return;
  if (x0 >= frame.width) return;

  uint32_t pitch = frame.pitch>>1;
  data[(y0 * pitch) + x0] = color;
}

inline void WASMInterface::draw_hline(uint16_t* data, int16_t x0, int16_t y0, int16_t w, uint16_t color) {
  if (y0 < 0) return;
  if (y0 >= frame.height) return;

  uint32_t pitch = frame.pitch>>1;
  for (int16_t x = x0; x < x0+w; x++) {
    if (x < 0) continue;
    if (x >= frame.width) return;

    data[(y0 * pitch) + x] = color;
  }
}

inline void WASMInterface::draw_vline(uint16_t* data, int16_t x0, int16_t y0, int16_t h, uint16_t color) {
  if (x0 < 0) return;
  if (x0 >= frame.width) return;

  uint32_t pitch = frame.pitch>>1;
  for (int16_t y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= frame.height) break;

    data[(y * pitch) + x0] = color;
  }
}

void WASMInterface::draw_text_utf8(uint8_t* s, int16_t x0, int16_t y0, const Font& font, const std::function<void(int,int)>& px) {
  uint8_t gw, gh;

  uint32_t codepoint = 0;
  uint32_t state = 0;

  for (; *s; ++s) {
    if (decode(&state, &codepoint, *s)) {
      continue;
    }

    // have code point:
    font.draw_glyph(gw, gh, codepoint, [=](int rx, int ry) { px(x0+rx, y0+ry); });

    x0 += gw;
  }

  //if (state != UTF8_ACCEPT) {
  //  printf("The string is not well-formed\n");
  //}
}

// TODO: generic font upload routine

Index::Index(uint32_t codePoint) : m_minCodePoint(codePoint) {}

Font::Font(
  const std::vector<Glyph>&    glyphs,
  const std::vector<Index>&    index,
  int                          stride
) : m_glyphs(glyphs),
    m_index(index),
    m_stride(stride)
{}

bool Font::draw_glyph(uint8_t& width, uint8_t& height, uint32_t codePoint, const std::function<void(int,int)>& px) const {
  auto glyphIndex = find_glyph(codePoint);
  if (glyphIndex == UINT32_MAX) {
    return false;
  }

  const auto& g = m_glyphs[glyphIndex];
  auto b = g.m_bitmapdata.data();

  width = g.m_width;
  height = g.m_height;
  for (int y = 0; y < height; y++, b += m_stride) {
    uint32_t bits = b[0];
    if (m_stride >= 2) {
      bits |= (b[1] << 8);
    }
    if (m_stride >= 4) {
      bits |= (b[2] << 16) | (b[3] << 24);
    }

    int k = 1 << (width-1);
    for (int x = 0; x < width; x++, k >>= 1) {
      if (bits & k) {
        px(x, y);
      }
    }
  }

  return true;
}

uint32_t Font::find_glyph(uint32_t codePoint) const {
  auto it = std::lower_bound(
    m_index.begin(),
    m_index.end(),
    Index(codePoint),
    [](const Index& first, const Index& last) {
      return first.m_minCodePoint < last.m_minCodePoint;
    }
  );
  if (it == m_index.end()) {
    return UINT32_MAX;
  }
  if (codePoint > it->m_maxCodePoint) {
    return UINT32_MAX;
  }
  return it->m_glyphIndex;
}

#include "wasm_bindings.cpp"
