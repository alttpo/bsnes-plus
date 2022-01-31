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

struct WASMError {
  WASMError();

  explicit WASMError(const std::string &moduleName, const std::string &contextFunction, const char *result);

  explicit WASMError(const std::string &moduleName, const std::string &contextFunction,
                     const char *result, const std::string &message,
                     const std::string &functionName, uint32_t moduleOffset);

  operator bool() const;
  std::string what() const;

public:
  std::string m_moduleName;
  std::string m_contextFunction;

  // this is `const char *` to maintain reference identity
  const char *m_result;
  std::string m_message;

  std::string m_functionName;
  uint32_t    m_moduleOffset;
};

#include "ziparchive.hpp"
#include "pixelfont.hpp"
#include "drawlist.hpp"

struct WASMInterface;

#include "wasminstance.hpp"

struct WASMInterface {
  WASMInterface() noexcept;

public:
  void on_nmi();
  const uint16_t *on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace);

public:
  void register_debugger(const std::function<void()>& do_break, const std::function<void()>& do_continue);

  std::function<void()> m_do_break;
  std::function<void()> m_do_continue;

public:
  void register_error_receiver(const std::function<void(const WASMError& err)>& error_receiver);
  void report_error(const WASMError& err);
  WASMError last_error() const;

private:
  WASMError m_last_error;
  std::function<void(const WASMError& err)> m_error_receiver;

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
