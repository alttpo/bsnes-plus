#include <snes/snes.hpp>
#include "wasminterface.hpp"

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

    const uint16_t *newdata = instance->mem<const uint16_t>(results_val[0].of.i32);
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
  m_exporttypes.reset(new wasm_exporttype_vec_t());
  wasm_module_exports(m_module.get(), m_exporttypes.get());

  // fetch import types:
  m_importtypes.reset(new wasm_importtype_vec_t());
  wasm_module_imports(m_module.get(), m_importtypes.get());
}

void ModuleInstance::instantiate(wasm_extern_vec_t* imports) {
  wasm_trap_t* trap = nullptr;

  // create instance:
  m_instance.reset(
    wasm_instance_new(wasmInterface.m_store.get(), m_module.get(), imports, &trap),
    wasm_instance_delete
  );

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
    auto exporttype = exporttypes->data[i];
    if (wasm_extern_kind(extrn) == WASM_EXTERN_MEMORY) {
      // should only be one memory per instance:
      assert(m_memory == nullptr);
      m_memory = wasm_extern_as_memory(extrn);
    } else if (wasm_extern_kind(extrn) == WASM_EXTERN_FUNC) {
      // track functions by name:
      m_functions.emplace(
        std::string(wasm_exporttype_name(exporttype)->data),
        wasm_extern_as_func(extrn)
      );
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

template<typename T>
T* ModuleInstance::mem(int32_t s_offset, size_t size) {
  uint32_t offset = static_cast<uint32_t>(s_offset);
  if (offset == 0) {
    return nullptr;
  }

  size_t data_size = wasm_memory_data_size(m_memory);
  if (offset >= data_size) {
    throw WASMTrapException(std::string("out of bounds memory access"));
  }
  if (offset + size > data_size) {
    throw WASMTrapException(std::string("out of bounds memory access"));
  }

  byte_t* p = wasm_memory_data(m_memory);
  return reinterpret_cast<T*>(p + offset);
}

size_t ModuleInstance::mem_size() {
  return wasm_memory_data_size(m_memory);
}

wasm_functype_t *ModuleInstance::parse_sig(const char *sig) {
  // TODO
  return wasm_functype_new_0_1(wasm_valtype_new_i32());
}


#include "wasm_bindings.cpp"
