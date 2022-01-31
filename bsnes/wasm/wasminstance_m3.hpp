
struct WASMFunctionM3 : public WASMFunction {
  explicit WASMFunctionM3(const std::string& name, IM3Function m3fn);

  explicit operator bool() const final;

  IM3Function m_fn;
};

struct WASMInstanceM3 : public WASMInstanceBase {
  explicit WASMInstanceM3(WASMInterface* intf, const std::string &key, const std::shared_ptr<ZipArchive> &za);
  ~WASMInstanceM3();

  bool link_module() override;
  bool load_module() override;

private:
  bool _catchM3(M3Result err, const char* contextFunctionName = NULL);

public:
  bool filter_error() final;

public:
  bool func_find(const std::string &i_name, std::shared_ptr<WASMFunction> &o_func) final;
  bool func_invoke(const std::shared_ptr<WASMFunction>& fn, uint32_t i_retc, uint32_t i_argc, uint64_t *io_stack) final;
  uint64_t memory_size() final;

public:
  static const int stack_size_bytes = 1048576;

  IM3Environment m_env;
  IM3Runtime m_runtime;
  IM3Module m_module;

  std::set<std::string> m_warnings;
};
