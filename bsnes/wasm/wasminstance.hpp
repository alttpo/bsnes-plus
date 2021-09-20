#pragma once

struct WASMMessage {
  WASMMessage(const uint8_t *data, uint16_t size);
  ~WASMMessage();

  const uint8_t *m_data;
  uint16_t m_size;
};

struct WASMFunction {
  virtual explicit operator bool() const = 0;
  virtual const char* name() const;

  const std::string m_name;

protected:
  explicit WASMFunction(const std::string& name);
};

struct WASMInstanceBase {
  explicit WASMInstanceBase(WASMInterface* interface, const std::string& key, const std::shared_ptr<ZipArchive>& za);
  virtual ~WASMInstanceBase();

public:
  void msg_enqueue(const std::shared_ptr<WASMMessage>& msg);
  std::shared_ptr<WASMMessage> msg_dequeue();
  bool msg_size(uint16_t *o_size);

public:
  virtual std::shared_ptr<WASMFunction> func_find(const std::string& i_name) = 0;
  virtual void func_invoke(const std::shared_ptr<WASMFunction>& fn, uint32_t i_retc, uint32_t i_argc, uint64_t* io_stack) = 0;
  virtual uint64_t memory_size() = 0;

public:
  // wasm bindings:
#define decl_binding(name) \
  static const char *wa_sig_##name; \
  const char* wa_fun_##name(void* _mem, uint64_t* _sp)

  decl_binding(debugger_break);
  decl_binding(debugger_continue);

  decl_binding(za_file_locate);
  decl_binding(za_file_size);
  decl_binding(za_file_extract);

  decl_binding(msg_recv);
  decl_binding(msg_size);

  decl_binding(snes_bus_read);
  decl_binding(snes_bus_write);

  decl_binding(ppux_spaces_reset);

  decl_binding(ppux_font_load_za);
  decl_binding(ppux_font_delete);

  decl_binding(ppux_draw_list_reset);
  decl_binding(ppux_draw_list_append);

  decl_binding(ppux_vram_write);
  decl_binding(ppux_cgram_write);
  decl_binding(ppux_oam_write);

  decl_binding(ppux_vram_read);
  decl_binding(ppux_cgram_read);
  decl_binding(ppux_oam_read);

#undef decl_binding

public:
  WASMInterface* m_interface;
  const std::string m_key;
  const std::shared_ptr<ZipArchive> m_za;

  uint8_t*  m_data;
  uint64_t  m_size;

  std::queue<std::shared_ptr<WASMMessage>> m_msgs;

  DrawList::FontContainer  fonts;
  DrawList::SpaceContainer spaces;
};

#define wa_offset_to_ptr(offset)  (void*)((uint8_t*)_mem + (uint32_t)(offset))
#define wa_ptr_to_offset(ptr)     (uint32_t)(((uint8_t*)ptr) - (uint8_t*)_mem)

#define wa_return_type(TYPE)      TYPE* raw_return = ((TYPE*) (_sp++));
#define wa_arg(TYPE, NAME)        TYPE NAME = * ((TYPE *) (_sp++));
#define wa_arg_mem(TYPE, NAME)    TYPE NAME = (TYPE)wa_offset_to_ptr(* ((uint32_t *) (_sp++)));

#define wa_return(VALUE)          { *raw_return = (VALUE); return nullptr; }
#define wa_trap(VALUE)            { return VALUE; }
#define wa_success()              { return nullptr; }

#define wa_ptr_is_null(addr)      ((void*)(addr) <= _mem)
#define wa_check_mem(addr, len)   { if (wa_ptr_is_null(addr) || ((uint64_t)(uintptr_t)(addr) + (len)) > ((uint64_t)(uintptr_t)(_mem) + memory_size())) wa_trap("out of bounds memory access"); }

#ifdef WASM_USE_M3
#  include "wasminstance_m3.hpp"
#else

// TODO: wasmer

#endif
