#pragma once

#include <optional>
#include <map>
#include <string>
#include <queue>

#include "wasm3.h"

namespace WASM {

struct error : public std::runtime_error {
  explicit error(M3Result err) : std::runtime_error(err) {}
};

template <class T>
struct RawCall;

template <class T>
struct RawCall {
  template <const void * (T::*Func) (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem)>
  static const void * adapter(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem) {
    return (reinterpret_cast<T*>(_ctx->userdata)->*Func)(runtime, _ctx, _sp, _mem);
  }
};

struct Message {
  Message(const uint8_t *data, uint16_t size);
  ~Message();

  const uint8_t *m_data;
  uint16_t m_size;
};

struct Module {
  Module(const std::string& key, const std::shared_ptr<struct M3Environment>& env, size_t stack_size_bytes, const uint8_t *data, size_t size);
  ~Module();

  M3Result suppressFunctionLookupFailed(M3Result res, const char *function_name);

  void link(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall);
  void linkEx(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall, const void *userdata);

  bool get_global(const char * const i_globalName, IM3TaggedValue o_value);

  void msg_enqueue(const std::shared_ptr<Message>& msg);
  std::shared_ptr<Message> msg_dequeue();
  bool msg_size(uint16_t *o_size);

  M3Result invoke(const char *function_name, int argc, const char *argv[]);

public:
  const std::string m_key;
  std::shared_ptr<struct M3Environment> m_env;

  size_t m_size;
  const uint8_t *m_data;

  IM3Module  m_module;
  IM3Runtime m_runtime;

  std::queue<std::shared_ptr<Message>> m_msgs;
  std::map<std::string, bool> m_function_warned;
};

struct Host {
  Host(size_t stack_size_bytes) : default_stack_size_bytes(stack_size_bytes) {
    m_env.reset(m3_NewEnvironment(), m3_FreeEnvironment);
    if (m_env == nullptr) {
      throw std::bad_alloc();
    }
  }

  void reset();
  std::shared_ptr<Module> parse_module(const std::string& key, const uint8_t *data, size_t size);
  void load_module(const std::shared_ptr<Module>& module);
  void unload_module(const std::string& key);
  void invoke_all(const char *function_name, int argc, const char* argv[]);
  std::shared_ptr<Module> get_module(const std::string& key);
  std::shared_ptr<Module> get_module(IM3Module module);

public:
  size_t default_stack_size_bytes;

  std::shared_ptr<struct M3Environment>           m_env;
  std::map<std::string, std::shared_ptr<Module>>  m_modules;
  std::map<IM3Module, std::shared_ptr<Module>>    m_modules_by_ptr;
};

extern Host host;

}
