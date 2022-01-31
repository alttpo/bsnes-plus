
#include "wasminterface.hpp"

#include <memory>
#include <snes/snes.hpp>

// WASMError:
//////////

WASMError::WASMError() : m_result()
{
}

WASMError::WASMError(const std::string &contextFunction, const std::string &result) :
  m_contextFunction(contextFunction),
  m_result(result),
  m_message(),
  m_wasmFunctionName(),
  m_wasmModuleOffset(0)
{
}

WASMError::WASMError(const std::string &contextFunction, const std::string &result, const std::string &message) :
  m_contextFunction(contextFunction),
  m_result(result),
  m_message(message),
  m_wasmFunctionName(),
  m_wasmModuleOffset(0)
{
}

WASMError::operator bool() const {
  return !m_result.empty();
}

std::string WASMError::what() const {
  std::string estr("wasm ");
  if (!m_moduleName.empty()) {
    estr.append("module '");
    estr.append(m_moduleName);
    estr.append("' ");
  }
  if (!m_contextFunction.empty()) {
    estr.append("function '");
    estr.append(m_contextFunction);
    estr.append("' ");
  }
  estr.append("error: ");
  estr.append(m_result);
  if (!m_message.empty()) {
    estr.append(": ");
    estr.append(m_message);
  }
  if (!m_wasmFunctionName.empty() || m_wasmModuleOffset > 0) {
    estr.append("; at wasm offset ");
    estr.append(std::to_string(m_wasmModuleOffset));
    if (!m_wasmFunctionName.empty()) {
      estr.append(" function '");
      estr.append(m_wasmFunctionName);
      estr.append("'");
    }
  }
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

void WASMInterface::register_message_receiver(const std::function<void(log_level level, const std::string& msg)>& mesage_receiver) {
  m_message_receiver = mesage_receiver;
}

void WASMInterface::log_message(log_level level, const std::string& msg) {
  m_message_receiver(level, msg);
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
  SNES::ppu.ppux_draw_list_clear();
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
