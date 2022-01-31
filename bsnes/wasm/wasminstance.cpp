
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

// WASMError:
//////////

WASMError::WASMError() : m_moduleName(), m_result(NULL)
{
}

WASMError::WASMError(const std::string &moduleName, const char *result) :
  m_moduleName(moduleName),
  m_result(result)
{
}

WASMError::WASMError(const std::string &moduleName, const std::string &contextFunction,
                     const char *result, const std::string &message,
                     const std::string &wasmFunctionName, uint32_t wasmModuleOffset) :
    m_moduleName(moduleName),
    m_contextFunction(contextFunction),
    m_result(result),
    m_message(message),
    m_functionName(wasmFunctionName),
    m_moduleOffset(wasmModuleOffset)
{
}

WASMError::operator bool() const {
  return m_result != NULL;
}

std::string WASMError::what() const {
  char buff[1024];
  if (!m_functionName.empty() || m_moduleOffset > 0) {
    snprintf(buff, sizeof(buff), "wasm module '%s' function '%s' error: %s%s%s; from wasm function '%s' offset %u",
             m_moduleName.c_str(),
             m_contextFunction.c_str(),
             m_result,
             m_message.empty() ? "" : ": ",
             m_message.c_str(),
             m_functionName.empty() ? "<unknown>" : m_functionName.c_str(),
             m_moduleOffset);
  } else {
    snprintf(buff, sizeof(buff), "wasm module '%s' function '%s' error: %s%s%s",
             m_moduleName.c_str(),
             m_contextFunction.c_str(),
             m_result,
             m_message.empty() ? "" : ": ",
             m_message.c_str());
  }
  std::string estr = buff;
  return estr;
}

// Base:
//////////

WASMInstanceBase::WASMInstanceBase(WASMInterface* interface, const std::string &key, const std::shared_ptr<ZipArchive> &za)
  : m_interface(interface), m_key(key), m_za(za), m_data(nullptr), m_size(0),
    m_fonts(new DrawList::FontContainer()), m_spaces(new DrawList::SpaceContainer())
{
}

bool WASMInstanceBase::extract_wasm() {
  auto fh = m_za->file_locate("main.wasm");
  if (!fh) {
    m_err = WASMError(m_key, "missing required main.wasm in zip archive");
    return false;
  }

  if (!m_za->file_size(fh, &m_size)) {
    m_err = WASMError(m_key, "failed to retrieve file size of main.wasm in zip archive");
    return false;
  }

  // extract main.wasm into m_data/m_size:
  m_data = new uint8_t[m_size];
  if (!m_za->file_extract(fh, m_data, m_size)) {
    m_err = WASMError(m_key, "failed to extract main.wasm in zip archive");
    return false;
  }

  return true;
}

WASMInstanceBase::~WASMInstanceBase() {
  delete[] m_data;
}

void WASMInstanceBase::warn() {
  if (!m_err)
    return;

  fprintf(stderr, "%s\n", m_err.what().c_str());
  fflush(stderr);

  // clear error:
  m_err = WASMError();
}

bool WASMInstanceBase::msg_enqueue(const std::shared_ptr<WASMMessage>& msg) {
  //printf("msg_enqueue(%p, %u)\n", msg->m_data, msg->m_size);

  std::shared_ptr<WASMFunction> fn;
  if (!func_find("on_msg_recv", fn))
    return false;

  m_msgs.push(msg);

  if (!func_invoke(fn, 0, 0, nullptr)) {
    warn();
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
