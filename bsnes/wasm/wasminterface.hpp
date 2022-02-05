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

enum log_level {
  L_DEBUG,
  L_INFO,
  L_WARN,
  L_ERROR
};

struct WASMError {
  WASMError();

  explicit WASMError(const std::string &contextFunction, const std::string &result);
  explicit WASMError(const std::string &contextFunction, const std::string &result, const std::string &message);

  operator bool() const;
  std::string what(bool withModuleName = true) const;

public:
  std::string m_moduleName;
  std::string m_contextFunction;

  std::string m_result;
  std::string m_message;

  std::string m_wasmFunctionName;
  uint32_t    m_wasmModuleOffset;
};

#include "ziparchive.hpp"
#include "pixelfont.hpp"
#include "drawlist.hpp"

struct WASMInterface;

#include "wasminstance.hpp"

struct WASMInterface {
  WASMInterface() noexcept;

public:
  void on_power();
  void on_reset();
  void on_unload();
  void on_nmi();
  const uint16_t *on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace);

private:
  void run_func(const std::string& name);

public:
  void register_debugger(const std::function<void()>& do_break, const std::function<void()>& do_continue);

  std::function<void()> m_do_break;
  std::function<void()> m_do_continue;

public:
  void report_error(const WASMError& err, log_level level = L_ERROR);
  WASMError last_error() const;

private:
  WASMError m_last_error;
  std::function<void(const WASMError& err)> m_error_receiver;

public:
  void register_message_receiver(const std::function<void(log_level level, const std::string& msg)>& mesage_receiver);
  void log_message(log_level level, const std::string& m);
  void log_message(log_level level, std::initializer_list<const std::string> parts);
  void log_module_message(log_level level, const std::string& moduleName, const std::string& m);
  void log_module_message(log_level level, const std::string& moduleName, std::initializer_list<const std::string> parts);

private:
  std::function<void(log_level level, const std::string& msg)> m_message_receiver;

public:
  void reset();

  // load a ZIP containing a main.wasm module and embedded resources:
  bool load_zip(const std::string& instanceKey, const uint8_t *data, size_t size);
  void unload_zip(const std::string& instanceKey);

  bool msg_enqueue(const std::string& instanceKey, const uint8_t *data, size_t size);

private:
  std::vector<std::shared_ptr<WASMInstanceBase>> m_instances;
};

extern WASMInterface wasmInterface;
