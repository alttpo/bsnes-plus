
// WASMMessage:
//////////

#include "wasminstance.hpp"

WASMMessage::WASMMessage(const uint8_t * const data, uint16_t size) : m_data(new uint8_t[size]), m_size(size) {
  memcpy((void *)m_data, (const void *)data, size);
}

WASMMessage::~WASMMessage() {
  delete[] m_data;
}

// WASMFunction:
//////////

WASMFunction::WASMFunction(const std::string &name) : m_name(name) {
}

const char *WASMFunction::name() const { return m_name.c_str(); }

// Base:
//////////

WASMInstanceBase::WASMInstanceBase(WASMInterface* interface, const std::string &key, const std::shared_ptr<ZipArchive> &za)
  : m_interface(interface), m_key(key), m_za(za), m_data(nullptr), m_size(0),
    m_fonts(new DrawList::FontContainer()), m_spaces(new DrawList::SpaceContainer())
{
}

WASMInstanceBase::~WASMInstanceBase() {
  delete[] m_data;
  m_data = nullptr;
  m_size = 0;
  m_fonts.reset();
  m_spaces.reset();
}

bool WASMInstanceBase::_throw(const std::string& contextFunction, const char* result) {
  if (result == NULL) {
    // success:
    return true;
  }

  m_err = WASMError(m_key, contextFunction, result);

  if (filter_error()) {
    m_interface->report_error(m_err);
  }

  return false;
}

bool WASMInstanceBase::filter_error() {
  return (bool)m_err;
}

bool WASMInstanceBase::extract_wasm() {
  uint32_t fh;
  if (!m_za->file_locate("main.wasm", &fh)) {
    return _throw("extract_wasm", "missing required main.wasm in zip archive");
  }

  if (!m_za->file_size(fh, &m_size)) {
    return _throw("extract_wasm", "failed to retrieve file size of main.wasm in zip archive");
  }

  // extract main.wasm into m_data/m_size:
  m_data = new uint8_t[m_size];
  if (!m_za->file_extract(fh, m_data, m_size)) {
    return _throw("extract_wasm", "failed to extract main.wasm in zip archive");
  }

  return true;
}

bool WASMInstanceBase::msg_enqueue(const std::shared_ptr<WASMMessage>& msg) {
  //printf("msg_enqueue(%p, %u)\n", msg->m_data, msg->m_size);

  std::shared_ptr<WASMFunction> fn;
  if (!func_find("on_msg_recv", fn))
    return false;

  m_msgs.push(msg);

  if (!func_invoke(fn, 0, 0, nullptr)) {
    return false;
  }

  return true;
}

std::shared_ptr<WASMMessage> WASMInstanceBase::msg_dequeue() {
  auto msg = m_msgs.front();
  //printf("msg_dequeue() -> {%p, %u}\n", msg->m_data, msg->m_size);
  m_msgs.pop();
  return msg;
}

bool WASMInstanceBase::msg_size(uint16_t *o_size) {
  if (m_msgs.empty()) {
    //printf("msg_size() -> false\n");
    return false;
  }

  *o_size = m_msgs.front()->m_size;
  //printf("msg_size() -> true, %u\n", *o_size);
  return true;
}

#ifdef WASM_USE_M3
#  include "wasminstance_m3.cpp"
#else

#endif
