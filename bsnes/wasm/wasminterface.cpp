
#include "wasminterface.hpp"

#include <memory>
#include <snes/snes.hpp>

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

WASMInterface wasmInterface;

WASMInterface::WASMInterface() noexcept
{}

void WASMInterface::register_debugger(const std::function<void()>& do_break, const std::function<void()>& do_continue) {
  m_do_break = do_break;
  m_do_continue = do_continue;
}

void WASMInterface::register_error_receiver(const std::function<void(const WASMError &)> &error_receiver) {
  m_error_receiver = error_receiver;
}

void WASMInterface::report_error(const WASMError& err) {
  m_last_error = err;
  m_error_receiver(err);
}

WASMError WASMInterface::last_error() const {
  return m_last_error;
}

void WASMInterface::on_nmi() {
  for (auto &it : m_instances) {
    WASMError err;
    auto &instance = it.second;

    std::shared_ptr<WASMFunction> fn;
    if (!instance->func_find("on_nmi", fn)) {
      continue;
    }

    if (!instance->func_invoke(fn, 0, 0, nullptr)) {
      continue;
    }
  }
}

const uint16_t *WASMInterface::on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace) {
  for (auto &it : m_instances) {
    auto &instance = it.second;

    std::shared_ptr<WASMFunction> fn;
    if (!instance->func_find("on_frame_present", fn)) {
      continue;
    }

    if (!instance->func_invoke(fn, 0, 0, nullptr)) {
      continue;
    }
  }

  return data;
}

void WASMInterface::reset() {
  m_instances.clear();
}

bool WASMInterface::load_zip(const std::string &instanceKey, const uint8_t *data, size_t size) {
  // initialize the wasm module before inserting:
  std::shared_ptr<ZipArchive> za(new ZipArchive(data, size));
  auto m = std::shared_ptr<WASMInstanceBase>(new WASMInstanceM3(this, instanceKey, za));
  if (!m->extract_wasm()) {
    return false;
  }
  if (!m->load_module()) {
    return false;
  }
  if (!m->link_module()) {
    return false;
  }

  auto it = m_instances.find(instanceKey);
  if (it != m_instances.end()) {
    // existing instance found so erase it:
    //fprintf(stderr, "load_zip(): erasing existing instance key \"%s\"\n", instanceKey.c_str());
    m_instances.erase(it);
  }

  // emplace a new instance of the module:
  //fprintf(stderr, "load_zip(): inserting new instance key \"%s\"\n", instanceKey.c_str());
  m_instances.emplace_hint(it, instanceKey, m);

  return true;
}

void WASMInterface::unload_zip(const std::string &instanceKey) {
  m_instances.erase(instanceKey);
}

bool WASMInterface::msg_enqueue(const std::string &instanceKey, const uint8_t *data, size_t size) {
  auto it = m_instances.find(instanceKey);
  if (it == m_instances.end()) {
    return false;
  }

  auto &instance = it->second;
  return instance->msg_enqueue(std::make_shared<WASMMessage>(data, size));
}

#include "wasminstance.cpp"

#include "ziparchive.cpp"
#include "pixelfont.cpp"
#include "drawlist.cpp"
#include "wasm_bindings.cpp"
