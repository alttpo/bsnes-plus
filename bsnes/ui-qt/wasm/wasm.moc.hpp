class WasmWindow : public Window {
  Q_OBJECT

public:
  QVBoxLayout     *layout;
  QPlainTextEdit  *log;

  WasmWindow();

signals:
  void appendMessage(const QString&);

public slots:
};

extern WasmWindow *wasmWindow;
