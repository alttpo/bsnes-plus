#pragma once

#include <stdexcept>
#include <string>
#include <map>
#include <memory>
#include <queue>
#include <vector>
#include "wasmer.h"

struct WASMError : public std::runtime_error {
  explicit WASMError(wasm_trap_t*);

private:
  static std::string get_message(wasm_trap_t* trap);
};

struct WASMTrapException : public std::runtime_error {
  explicit WASMTrapException(const std::string&);

  virtual const char* what() const noexcept;
  const std::string& message() const noexcept;

private:
  std::string m_msg;
};

struct WASMMessage {
  WASMMessage(const uint8_t *data, uint16_t size);
  ~WASMMessage();

  const uint8_t *m_data;
  uint16_t m_size;
};

struct ModuleInstance {
  explicit ModuleInstance(wasm_module_t* module);

  void instantiate(wasm_extern_vec_t* imports);

  // returns nullptr if not found
  wasm_func_t* find_function(const std::string& function_name);

  // throws WASMError
  void call(const wasm_func_t* func, const wasm_val_vec_t* args, wasm_val_vec_t* results);

  template<typename T>
  T* mem(int32_t offset, size_t size = 0);
  size_t mem_size();

public:
  void msg_enqueue(const std::shared_ptr<WASMMessage>&);
  std::shared_ptr<WASMMessage> msg_dequeue();
  bool msg_size(uint16_t *o_size);

public:
  // wasm bindings:
  void link_module(wasm_extern_vec_t *imports);
  wasm_functype_t *parse_sig(const char *sig);

#define decl_binding(name) \
  static const char *wa_sig_##name; \
  wasm_trap_t* wa_fun_##name(const wasm_val_vec_t* args, wasm_val_vec_t* results)

  decl_binding(debugger_break);
  decl_binding(debugger_continue);

  decl_binding(msg_recv);
  decl_binding(msg_size);

  decl_binding(snes_bus_read);
  decl_binding(snes_bus_write);

  decl_binding(ppux_reset);
  decl_binding(ppux_sprite_reset);
  decl_binding(ppux_sprite_read);
  decl_binding(ppux_sprite_write);
  decl_binding(ppux_ram_write);
  decl_binding(ppux_ram_read);

  decl_binding(frame_acquire);
  // TODO: frame drawing functions

#undef decl_binding

public:
  // module:
  std::shared_ptr<wasm_module_t>          m_module;
  std::shared_ptr<wasm_exporttype_vec_t>  m_exporttypes;

  // instance:
  std::shared_ptr<wasm_instance_t>    m_instance;
  std::shared_ptr<wasm_extern_vec_t>  m_exports;

  wasm_memory_t*                      m_memory;
  std::map<std::string, wasm_func_t*> m_functions;

  std::queue<std::shared_ptr<WASMMessage>> m_msgs;
};

struct WASMInterface {
  WASMInterface();

public:
  void on_nmi();
  const uint16_t *on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace);

  void msg_enqueue(const std::shared_ptr<WASMMessage>&);

public:
  void register_debugger(const std::function<void()>& do_break, const std::function<void()>& do_continue);

  std::function<void()> m_do_break;
  std::function<void()> m_do_continue;

public:
  struct {
    const uint16_t *data;
    unsigned pitch;
    unsigned width;
    unsigned height;
    bool interlace;
  } frame;

public:
  void reset();
  void load_module(const std::string& module_name, const uint8_t *data, const size_t size);

public:
  std::vector<std::shared_ptr<ModuleInstance>>  m_instances;

private:
  std::shared_ptr<wasm_engine_t>  m_engine;
  std::shared_ptr<wasm_store_t>   m_store;

  friend struct ModuleInstance;
};

extern WASMInterface wasmInterface;
