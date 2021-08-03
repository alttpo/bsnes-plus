#pragma once

#include <functional>
#include <optional>
#include <map>
#include <string>
#include <queue>
#include <memory>
#include <stdexcept>
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

struct Module;

struct Function {
  Function(IM3Function function);

  M3Result callv(int dummy, ...);
  M3Result resultsv(int dummy, ...);

  M3Result callargv(int argc, const char * argv[]);

public:
  const IM3Function m_function;
};

struct Runtime;

struct Module {
  Module(const std::string& key, const Runtime& runtime, const uint8_t *data, size_t size);
  ~Module();

public:
  M3Result warn(M3Result res, const char *function_name);
  M3Result suppressFunctionLookupFailed(M3Result res, const char *function_name);

public:
  void link(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall);
  void linkEx(const char *module_name, const char *function_name, const char *signature, M3RawCall rawcall, const void *userdata);

public:
  bool get_global(const char * const i_globalName, IM3TaggedValue o_value);

public:
  const std::string m_key;
  const Runtime&    m_runtime;

  size_t m_size;
  const uint8_t *m_data;

  IM3Module m_module;

  std::map<std::string, bool> m_function_warned;
};

// a Runtime is an isolated group of Modules that can interact with one another
struct Runtime {
  Runtime(const std::string& key, const std::shared_ptr<struct M3Environment>& env, size_t stack_size_bytes);
  ~Runtime();

public:
  M3Result warn(M3Result res, const char *function_name);
  M3Result suppressFunctionLookupFailed(M3Result res, const char *function_name);

public:
  std::shared_ptr<Module> parse_module(const std::string& key, const uint8_t *data, size_t size);
  void load_module(const std::shared_ptr<Module>& module);

  std::shared_ptr<Module> get_module(const std::string& key);
  std::shared_ptr<Module> get_module(IM3Module module);
  void each_module(const std::function<void(const std::shared_ptr<Module>&)>& each);

public:
  void msg_enqueue(const std::shared_ptr<Message>& msg);
  std::shared_ptr<Message> msg_dequeue();
  bool msg_size(uint16_t *o_size);

public:
  M3Result with_function(const char *function_name, const std::function<void(Function&)>& f);

  uint8_t *memory(uint32_t i_offset, uint32_t &o_size);

public:
  const std::string m_key;
  std::shared_ptr<struct M3Environment> m_env;

  IM3Runtime m_runtime;

  std::queue<std::shared_ptr<Message>> m_msgs;

  std::map<std::string, bool> m_function_warned;

  std::map<std::string, std::shared_ptr<Module>>  m_modules;
  std::map<IM3Module, std::shared_ptr<Module>>    m_modules_by_ptr;
};

// A Host contains multiple Runtimes in a single Environment
struct Host {
  Host(size_t stack_size_bytes) : default_stack_size_bytes(stack_size_bytes) {
    m_env.reset(m3_NewEnvironment(), m3_FreeEnvironment);
    if (m_env == nullptr) {
      throw std::bad_alloc();
    }
  }

  void reset();
  std::shared_ptr<Runtime> get_runtime(const std::string& key);
  std::shared_ptr<Runtime> get_runtime(const IM3Runtime runtime);
  void with_runtime(const std::string& key, const std::function<void(const std::shared_ptr<Runtime>&)>& with);
  void each_runtime(const std::function<void(const std::shared_ptr<Runtime>&)>& each);
  void unload_runtime(const std::string& key);

public:
  size_t default_stack_size_bytes;

  std::shared_ptr<struct M3Environment>           m_env;
  std::map<std::string, std::shared_ptr<Runtime>> m_runtimes;
  std::map<IM3Runtime, std::shared_ptr<Runtime>>  m_runtimes_by_ptr;
};

extern Host host;

}
