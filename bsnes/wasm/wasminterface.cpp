#include <snes/snes.hpp>
#include "wasminterface.hpp"
#include <sstream>

WASMInterface wasmInterface;

////////////

WASMError::WASMError(wasm_trap_t* trap)
  : std::runtime_error(get_message(trap))
{
}

std::string WASMError::get_message(wasm_trap_t* trap) {
  if (!trap) {
    return std::string();
  }

  // fetch trap message:
  wasm_message_t out;
  wasm_trap_message(trap, &out);

  // TODO: format with stack trace
  std::string s(out.data);

  wasm_name_delete(&out);
  wasm_trap_delete(trap);

  return s;
}

////////////

WASMTrapException::WASMTrapException(const std::string& msg)
  : m_msg(msg), std::runtime_error(msg)
{
}

const char* WASMTrapException::what() const noexcept {
  return m_msg.c_str();
}

const std::string& WASMTrapException::message() const noexcept {
  return m_msg;
}

////////////

WASMMessage::WASMMessage(const uint8_t * const data, uint16_t size)
  : m_data(new uint8_t[size]),
    m_size(size)
{
  memcpy((void *)m_data, (const void *)data, size);
}

WASMMessage::~WASMMessage() {
  delete[] m_data;
}

////////////

WASMInterface::WASMInterface() {
  m_engine.reset(wasm_engine_new(), wasm_engine_delete);
}

void WASMInterface::register_debugger(
  const std::function<void()>& do_break,
  const std::function<void()>& do_continue
) {
  m_do_break = do_break;
  m_do_continue = do_continue;
}

void WASMInterface::on_nmi() {
  wasm_val_t args_val[0] = { };
  wasm_val_t results_val[0] = { };
  wasm_val_vec_t args = WASM_ARRAY_VEC(args_val);
  wasm_val_vec_t results = WASM_ARRAY_VEC(results_val);

  for (auto &instance : m_instances) {
    auto func = instance->find_function("on_nmi");
    if (!func) continue;

    instance->call(func, &args, &results);
  }
}

const uint16_t *WASMInterface::on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace) {
  frame.data = data;
  frame.pitch = pitch;
  frame.width = width;
  frame.height = height;
  frame.interlace = interlace;

  wasm_val_t args_val[0] = { };
  wasm_val_t results_val[1] = { WASM_INIT_VAL };
  wasm_val_vec_t args = WASM_ARRAY_VEC(args_val);
  wasm_val_vec_t results = WASM_ARRAY_VEC(results_val);

  for (auto &instance : m_instances) {
    auto func = instance->find_function("on_frame_present");
    if (!func) continue;

    instance->call(func, &args, &results);

    const uint16_t *newdata = instance->mem<const uint16_t>(&results_val[0]);
    if (newdata) {
      // frame_acquire() copies the original frame data into wasm memory, so we
      // avoid a copy here and just present the (possibly modified) frame data
      // directly from wasm memory:
      frame.data = newdata;
    }
  }

  return frame.data;
}

void WASMInterface::msg_enqueue(const std::shared_ptr<WASMMessage>& msg) {
  for (auto& instance : m_instances) {
    instance->msg_enqueue(msg);
  }
}

void WASMInterface::reset() {
  m_instances.clear();
  m_store.reset();
}

void WASMInterface::load_module(const std::string& module_name, const uint8_t *data, const size_t size) {
  // create a store on-demand when loading a module:
  if (!m_store) {
    m_store.reset(wasm_store_new(m_engine.get()), wasm_store_delete);
  }

  wasm_byte_vec_t wat;
  wasm_byte_vec_new(&wat, size, reinterpret_cast<const wasm_byte_t *>(data));

  wasm_byte_vec_t wasm_bytes;
  wat2wasm(&wat, &wasm_bytes);

  wasm_byte_vec_delete(&wat);

  std::shared_ptr<ModuleInstance> module(new ModuleInstance(wasm_module_new(m_store.get(), &wasm_bytes)));

  wasm_byte_vec_delete(&wasm_bytes);

  // define imports:
  wasm_extern_vec_t import_object = WASM_EMPTY_VEC;
  module->link_module(&import_object);

  // instantiate module:
  module->instantiate(&import_object);

  m_instances.emplace_back(module);
}

////////////

ModuleInstance::ModuleInstance(wasm_module_t* module)
  : m_module(module, wasm_module_delete)
{
  // fetch export types:
  {
    auto exporttypes = new wasm_exporttype_vec_t();
    wasm_module_exports(m_module.get(), exporttypes);
    printf("module %zu exports\n", exporttypes->size);
    m_exporttypes.reset(exporttypes, [](wasm_exporttype_vec_t* vec) {
      wasm_exporttype_vec_delete(vec);
      delete vec;
    });

    for (size_t i = 0; i < exporttypes->size; i++) {
      const wasm_name_t* name = wasm_exporttype_name(exporttypes->data[i]);
      printf("module export[%zu] = '%.*s'\n", i, (int)name->size, (const char *)name->data);
    }
  }

  // fetch import types:
  {
    auto importtypes = new wasm_importtype_vec_t();
    wasm_module_imports(m_module.get(), importtypes);
    printf("module %zu imports\n", importtypes->size);
    m_importtypes.reset(importtypes, [](wasm_importtype_vec_t* vec) {
      wasm_importtype_vec_delete(vec);
      delete vec;
    });

    for (size_t i = 0; i < importtypes->size; i++) {
      const wasm_name_t* name = wasm_importtype_name(importtypes->data[i]);
      printf("module import[%zu] = '%.*s'\n", i, (int)name->size, (const char *)name->data);
    }
  }
}

void ModuleInstance::instantiate(wasm_extern_vec_t* imports) {
  wasm_trap_t* trap = nullptr;

  // create instance:
  m_instance.reset(
    wasm_instance_new(wasmInterface.m_store.get(), m_module.get(), imports, &trap),
    wasm_instance_delete
  );
  if (trap != nullptr) {
    throw WASMError(trap);
  }
  if (m_instance.get() == nullptr) {
    throw std::runtime_error("failed to create module instance");
  }

  // fetch instance exports:
  wasm_extern_vec_t* exports = new wasm_extern_vec_t();
  m_exports.reset(exports, [](wasm_extern_vec_t* vec){
    wasm_extern_vec_delete(vec);
    delete vec;
  });
  wasm_instance_exports(m_instance.get(), exports);

  // make sure exports and exporttypes line up:
  auto exporttypes = m_exporttypes.get();
  auto size = exports->size;
  assert(exporttypes->size == exports->size);

  // export resolution:
  for (size_t i = 0; i < size; i++) {
    auto extrn = exports->data[i];
    auto kind = wasm_extern_kind(extrn);
    auto exporttype = exporttypes->data[i];
    if (kind == WASM_EXTERN_MEMORY) {
      printf("instance export[%zu] memory\n", i);
      // should only be one memory per instance:
      assert(m_memory == nullptr);
      m_memory = wasm_extern_as_memory(extrn);
    } else if (kind == WASM_EXTERN_FUNC) {
      // track functions by name:
      const wasm_name_t* name = wasm_exporttype_name(exporttype);
      std::string name_s((const char *)name->data, name->size);
      printf("instance export[%zu] function '%s'\n", i, name_s.c_str());
      m_functions.emplace(
        name_s,
        wasm_extern_as_func(extrn)
      );
    } else if (kind == WASM_EXTERN_GLOBAL) {
      printf("instance export[%zu] global\n", i);
    } else if (kind == WASM_EXTERN_TABLE) {
      printf("instance export[%zu] table\n", i);
    }
  }
}

wasm_func_t* ModuleInstance::find_function(const std::string& function_name) {
  auto funcIt = m_functions.find(function_name);
  if (funcIt == m_functions.end()) {
    return nullptr;
  }

  return funcIt->second;
}

void ModuleInstance::call(const wasm_func_t* func, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
  wasm_trap_t *trap = wasm_func_call(func, args, results);
  if (trap) {
    throw WASMError(trap);
  }
}

void ModuleInstance::msg_enqueue(const std::shared_ptr<WASMMessage>& msg) {
  m_msgs.push(msg);

  auto func = find_function("on_msg_recv");
  if (!func) {
    return;
  }

  wasm_val_t args_val[0] = { };
  wasm_val_t results_val[0] = { };
  wasm_val_vec_t args = WASM_ARRAY_VEC(args_val);
  wasm_val_vec_t results = WASM_ARRAY_VEC(results_val);

  call(func, &args, &results);
}

std::shared_ptr<WASMMessage> ModuleInstance::msg_dequeue() {
  auto msg = m_msgs.front();
  //printf("msg_dequeue() -> {%p, %u}\n", msg->m_data, msg->m_size);
  m_msgs.pop();
  return msg;
}

bool ModuleInstance::msg_size(uint16_t *o_size) {
  if (m_msgs.empty()) {
    //printf("msg_size() -> false\n");
    return false;
  }

  *o_size = m_msgs.front()->m_size;
  //printf("msg_size() -> true, %u\n", *o_size);
  return true;
}

inline size_t ModuleInstance::mem_size() {
  return wasm_memory_data_size(m_memory);
}

template<typename T>
T* ModuleInstance::mem(wasm_val_t* val, size_t size) {
  T* ptr = mem_unchecked<T>(val);
  if (ptr == nullptr) {
    return nullptr;
  }

  return mem_check<T>(ptr, size);
}

template<typename T>
T* ModuleInstance::mem_unchecked(wasm_val_t* val) {
  uint32_t offset = static_cast<uint32_t>(val->of.i32);
  if (offset == 0) {
    return nullptr;
  }

  byte_t* base = wasm_memory_data(m_memory);
  return reinterpret_cast<T*>(base + offset);
}

template<typename T>
T* ModuleInstance::mem_check(T* ptr, size_t size) {
  byte_t* base = wasm_memory_data(m_memory);
  uint32_t offset = (uint32_t)((uintptr_t)ptr - (uintptr_t)base);

  size_t data_size = mem_size();
  if (offset >= data_size) {
    throw WASMTrapException(std::string("out of bounds memory access"));
  }
  if (offset + size > data_size) {
    throw WASMTrapException(std::string("out of bounds memory access"));
  }

  return ptr;
}

template<typename T>
size_t ModuleInstance::mem_remaining(T* ptr) {
  byte_t* base = wasm_memory_data(m_memory);
  return ((uintptr_t)ptr + (uintptr_t)mem_size()) - (uintptr_t)base;
}

template<typename T>
void ModuleInstance::offset(wasm_val_t* out, T* ptr) {
  byte_t* base = wasm_memory_data(m_memory);
  out->of.i32 = (int32_t)((uintptr_t)ptr - (uintptr_t)base);
  out->kind = WASM_I32;
}

wasm_functype_t *ModuleInstance::parse_sig(const char *sig) {
  wasm_valtype_t* ps[1024];
  wasm_valtype_t* rs[1024];

  size_t max_types = strlen(sig);
  if (max_types < 2) {
    throw std::runtime_error("malformed signature; must contain at least '()'");
  }
  max_types -= 2;

  size_t num_rets = 0, num_params = 0;

  size_t* counter = &num_rets;
  wasm_valtype_t** list = rs;

  bool parsing_rets = true;
  const char *s = sig;
  while (*s) {
    char c = *s++;

    if (c == '(') {
      if (!parsing_rets) {
        throw std::runtime_error("malformed signature; unexpected '('");
      }
      list = ps;
      counter = &num_params;
      parsing_rets = false;
      continue;
    } else if (c == ' ') {
      continue;
    } else if (c == 'v') {
      // void type
      continue;
    } else if (c == ')') {
      break;
    }

    switch (c) {
      case 'i': *list++ = wasm_valtype_new(WASM_I32); break;
      case 'I': *list++ = wasm_valtype_new(WASM_I64); break;
      case 'f': *list++ = wasm_valtype_new(WASM_F32); break;
      case 'F': *list++ = wasm_valtype_new(WASM_F64); break;
      case '*': *list++ = wasm_valtype_new(WASM_I32); break;
      default: {
        std::ostringstream e;
        e << "malformed signature; unrecognized type character '";
        e << c;
        e << "'";
        throw std::runtime_error(e.str());
      }
    }

    ++(*counter);
  }

  wasm_valtype_vec_t params, results;
  wasm_valtype_vec_new(&params, num_params, ps);
  wasm_valtype_vec_new(&results, num_rets, rs);

  return wasm_functype_new(&params, &results);
}


#include "wasm_bindings.cpp"
