
#include "wasminterface.hpp"

#include <memory>
#include <snes/snes.hpp>

WASMInterface wasmInterface;

WASMInterface::WASMInterface() = default;

void WASMInterface::register_debugger(const std::function<void()>& do_break, const std::function<void()>& do_continue) {
  m_do_break = do_break;
  m_do_continue = do_continue;
}

void WASMInterface::on_nmi() {
  for (auto &it : m_instances) {
    auto &instance = it.second;

    std::shared_ptr<WASMFunction> fn;
    if (!instance->func_find("on_nmi", fn))
      continue;

    if (!instance->func_invoke(fn, 0, 0, nullptr))
      instance->warn();
  }
}

const uint16_t *WASMInterface::on_frame_present(const uint16_t *data, unsigned pitch, unsigned width, unsigned height, bool interlace) {
  for (auto &it : m_instances) {
    auto &instance = it.second;

    std::shared_ptr<WASMFunction> fn;
    if (!instance->func_find("on_frame_present", fn))
      continue;

    if (!instance->func_invoke(fn, 0, 0, nullptr))
      instance->warn();
  }

  return data;
}

void WASMInterface::reset() {
  m_instances.clear();
}

void WASMInterface::load_zip(const std::string &instanceKey, const uint8_t *data, size_t size) {
  auto it = m_instances.find(instanceKey);
  if (it != m_instances.end()) {
    // existing instance found so erase it:
    //fprintf(stderr, "load_zip(): erasing existing instance key \"%s\"\n", instanceKey.c_str());
    m_instances.erase(it);
  }

  // emplace a new instance of the module:
  std::shared_ptr<ZipArchive> za(new ZipArchive(data, size));
  //fprintf(stderr, "load_zip(): inserting new instance key \"%s\"\n", instanceKey.c_str());
  m_instances.emplace_hint(
    it,
    instanceKey,
    std::shared_ptr<WASMInstanceBase>(new WASMInstanceM3(this, instanceKey, za))
  );
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
