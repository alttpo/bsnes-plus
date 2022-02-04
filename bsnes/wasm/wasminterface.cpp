
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

std::string WASMError::what(bool withModuleName) const {
  std::string estr;
  if (withModuleName && !m_moduleName.empty()) {
    estr.append("[");
    estr.append(m_moduleName);
    estr.append("] ");
  }
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
  if (!m_contextFunction.empty()) {
    estr.append("; in host function '");
    estr.append(m_contextFunction);
    estr.append("'");
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

void WASMInterface::report_error(const WASMError& err) {
  m_last_error = err;
  if (err.m_moduleName.empty()) {
    log_message(L_ERROR, err.what());
  } else {
    log_module_message(L_ERROR, err.m_moduleName, err.what(false));
  }
}

WASMError WASMInterface::last_error() const {
  return m_last_error;
}

void WASMInterface::register_message_receiver(const std::function<void(log_level level, const std::string& msg)>& mesage_receiver) {
  m_message_receiver = mesage_receiver;
}

void WASMInterface::log_message(log_level level, const std::string& m) {
  m_message_receiver(level, m);
}

void WASMInterface::log_message(log_level level, std::initializer_list<const std::string> parts) {
  std::string m;
  for (const auto &item : parts) {
    m.append(item);
  }
  log_message(level, m);
}

void WASMInterface::log_module_message(log_level level, const std::string& moduleName, const std::string& m) {
  std::string t;
  t.append("[");
  t.append(moduleName);
  t.append("] ");
  t.append(m);
  log_message(level, t);
}

void WASMInterface::log_module_message(log_level level, const std::string& moduleName, std::initializer_list<const std::string> parts) {
  std::string m;
  m.append("[");
  m.append(moduleName);
  m.append("] ");
  for (const auto &item : parts) {
    m.append(item);
  }
  log_message(level, m);
}

void WASMInterface::on_nmi() {
  for (auto &instance : m_instances) {
    WASMError err;

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
  for (auto &instance : m_instances) {
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
  SNES::ppu.ppux_modules.clear();
  log_message(L_INFO, "all wasm modules removed");
}

bool WASMInterface::load_zip(const std::string &instanceKey, const uint8_t *data, size_t size) {
  log_module_message(L_DEBUG, instanceKey, {"wasm module loading from zip"});

  // initialize the wasm module before inserting:
  std::shared_ptr<ZipArchive> za(new ZipArchive(data, size));
  auto m = std::shared_ptr<WASMInstanceBase>(new WASMInstanceM3(this, instanceKey, za));

  log_module_message(L_DEBUG, instanceKey, {"extracting main.wasm"});
  if (!m->extract_wasm()) {
    return false;
  }
  log_module_message(L_DEBUG, instanceKey, {"loading wasm module"});
  if (!m->load_module()) {
    return false;
  }
  log_module_message(L_DEBUG, instanceKey, {"linking wasm module"});
  if (!m->link_module()) {
    return false;
  }

  // run the start routine:
  log_module_message(L_DEBUG, instanceKey, {"start routine execute"});
  if (!m->run_start()) {
    return false;
  }
  log_module_message(L_DEBUG, instanceKey, {"start routine complete"});

  // find where to add/replace the instance:
  auto it = std::find_if(
    m_instances.begin(),
    m_instances.end(),
    [&](const std::shared_ptr<WASMInstanceBase>& mo) { return mo->m_key == instanceKey; }
  );

  m->m_index = (it - m_instances.begin());
  m_instances.emplace(it, m);
  SNES::ppu.ppux_modules.emplace(
    SNES::ppu.ppux_modules.begin() + m->m_index,
    instanceKey,
    m->m_fonts,
    m->m_spaces
  );

  log_module_message(L_INFO, instanceKey, {"wasm module loaded from zip"});

  return true;
}

void WASMInterface::unload_zip(const std::string &instanceKey) {
  auto it = std::find_if(
    m_instances.begin(),
    m_instances.end(),
    [&](const std::shared_ptr<WASMInstanceBase>& mo) { return mo->m_key == instanceKey; }
  );

  if (it == m_instances.end())
    return;

  m_instances.erase(it);
  SNES::ppu.ppux_modules.erase(SNES::ppu.ppux_modules.begin() + (it - m_instances.begin()));

  // update m_index of successive instances:
  for (; it != m_instances.end(); it++) {
    (*it)->m_index = (it - m_instances.begin());
  }

  log_module_message(L_INFO, instanceKey, {"wasm module unloaded"});
}

bool WASMInterface::msg_enqueue(const std::string &instanceKey, const uint8_t *data, size_t size) {
  auto it = std::find_if(
    m_instances.begin(),
    m_instances.end(),
    [&](const std::shared_ptr<WASMInstanceBase>& mo) { return mo->m_key == instanceKey; }
  );

  if (it == m_instances.end()) {
    return false;
  }

  auto &instance = *it;
  return instance->msg_enqueue(std::make_shared<WASMMessage>(data, size));
}

#include "wasminstance.cpp"

#include "ziparchive.cpp"
#include "pixelfont.cpp"
#include "drawlist.cpp"
#include "wasm_bindings.cpp"
