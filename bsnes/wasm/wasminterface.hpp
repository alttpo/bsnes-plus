#pragma once

#include <functional>
#include <optional>
#include <map>
#include <string>
#include <queue>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <set>

#define WASM_USE_M3

#ifdef WASM_USE_M3
#include "wasm3.h"
#include "m3_api_libc.h"
#else
// TODO: wasmer
#endif

#define MINIZ_NO_STDIO
#include "miniz.h"

#include "ziparchive.hpp"
#include "pixelfont.hpp"
#include "drawlist.hpp"

struct WASMInterface;

#include "wasminstance.hpp"

struct WASMInterface {
  WASMInterface();

public:
  void on_nmi();
  const uint16_t *on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace);

public:
  void register_debugger(const std::function<void()>& do_break, const std::function<void()>& do_continue);

  std::function<void()> m_do_break;
  std::function<void()> m_do_continue;

public:
  void reset();

  // load a ZIP containing a main.wasm module and embedded resources:
  bool load_zip(const std::string& instanceKey, const uint8_t *data, size_t size);
  void unload_zip(const std::string& instanceKey);

  bool msg_enqueue(const std::string& instanceKey, const uint8_t *data, size_t size);

private:
  std::map<std::string, std::shared_ptr<WASMInstanceBase>> m_instances;
};

extern WASMInterface wasmInterface;
