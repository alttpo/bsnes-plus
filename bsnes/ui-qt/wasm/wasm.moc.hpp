class WasmWindow : public Window {
  Q_OBJECT

public:
  QVBoxLayout     *layout;
  QPlainTextEdit  *log;

  WasmWindow();

signals:
  void appendHtml(const QString&);

private:
  log_level minLevel = L_DEBUG;

  void logMessage(log_level level, const std::string& msg);
};

extern WasmWindow *wasmWindow;
