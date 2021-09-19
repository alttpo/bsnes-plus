
struct WASMFunctionM3 : public WASMFunction {
  explicit WASMFunctionM3(const std::string& name, IM3Function m3fn);

  explicit operator bool() const override;

  IM3Function m_fn;
};

struct WASMInstanceM3 : public WASMInstanceBase {
  explicit WASMInstanceM3(WASMInterface* interface, const std::string &key, const std::shared_ptr<ZipArchive> &za);
  ~WASMInstanceM3();

  void link_module();

public:
  std::shared_ptr<WASMFunction> func_find(const std::string &i_name) override;
  void func_invoke(const std::shared_ptr<WASMFunction>& fn, uint32_t i_retc, uint32_t i_argc, uint64_t *io_stack) override;
  uint64_t memory_size() override;

public:
  static const int stack_size_bytes = 1048576;

  IM3Environment m_env;
  IM3Runtime m_runtime;
  IM3Module m_module;
};
