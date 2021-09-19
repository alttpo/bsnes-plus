
#include "wasminterface.hpp"

#include <snes/snes.hpp>

WASMInterface wasmInterface;

WASMInterface::WASMInterface() = default;

void WASMInterface::register_debugger(const std::function<void()>& do_break, const std::function<void()>& do_continue) {
  m_do_break = do_break;
  m_do_continue = do_continue;
}

void WASMInterface::on_nmi() {
  for (auto &instance : m_instances) {
    auto fn = instance.second->func_find("on_nmi");
    instance.second->func_invoke(fn, 0, 0, nullptr);
  }
}

const uint16_t *WASMInterface::on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace) {
  for (auto &instance : m_instances) {
    auto fn = instance.second->func_find("on_frame_present");
    instance.second->func_invoke(fn, 0, 0, nullptr);
  }

  return data;
}

void WASMInterface::reset() {
  m_instances.clear();
}

void WASMInterface::load_zip(const std::string &instanceKey, const uint8_t *data, size_t size) {
  std::shared_ptr<ZipArchive> za(new ZipArchive(data, size));
  m_instances.emplace(
    instanceKey,
    std::shared_ptr<WASMInstanceBase>(new WASMInstanceM3(this, instanceKey, za))
  );
}

void WASMInterface::unload_zip(const std::string &instanceKey) {
  m_instances.erase(instanceKey);
}

void WASMInterface::msg_enqueue(const std::string &instanceKey, const uint8_t *data, size_t size) {
  auto it = m_instances.find(instanceKey);
  if (it == m_instances.end()) {
    return;
  }
  it->second->msg_enqueue(std::shared_ptr<WASMMessage>(new WASMMessage(data, size)));
}

#include "wasminstance.cpp"

#include "pixelfont.cpp"
#include "drawlist.cpp"
#include "wasm_bindings.cpp"
