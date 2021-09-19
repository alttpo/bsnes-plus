
struct WASMInstanceM3 : public WASMInstanceBase {
  explicit WASMInstanceM3(WASMInterface* interface, const std::string &key, const std::shared_ptr<ZipArchive> &za);
  ~WASMInstanceM3();

  void link_module();

public:
  void invoke(const std::string &i_name, uint64_t *io_stack) override;
  uint64_t memory_size() override;

public:
  static const int stack_size_bytes = 1048576;

  IM3Environment m_env;
  IM3Runtime m_runtime;
  IM3Module m_module;
};
