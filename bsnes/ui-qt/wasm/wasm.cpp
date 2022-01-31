#include "../ui-base.hpp"

#include "wasm.moc"
WasmWindow *wasmWindow;

WasmWindow::WasmWindow() {
  setObjectName("wasm-window");
  setWindowTitle("WASM Modules");
  resize(600, 360);
  setGeometryString(&config().geometry.settingsWindow);
  application.windowList.append(this);

  layout = new QVBoxLayout;
  layout->setMargin(Style::WindowMargin);
  layout->setSpacing(Style::WidgetSpacing);
  setLayout(layout);

  auto labelLog = new QLabel;
  labelLog->setText("Messages:");
  layout->addWidget(labelLog);

  log = new QPlainTextEdit;
  log->setFrameStyle(0);
  log->setReadOnly(true);
  log->setMaximumBlockCount(10000);
  layout->addWidget(log);

  // emit appendMessage(msg); to append to the log widget:
  connect(this, SIGNAL(appendMessage(const QString&)), log, SLOT(appendPlainText(const QString&)));

  wasmInterface.register_error_receiver([this](const WASMError& err) {
    logMessage(L_ERROR, err.what());
  });

  std::function<void(log_level level, const std::string& msg)> receiver =
    std::bind(&WasmWindow::logMessage, this, std::placeholders::_1, std::placeholders::_2);
  wasmInterface.register_message_receiver(receiver);
}

void WasmWindow::logMessage(log_level level, const std::string& msg) {
  if (level < minLevel)
    return;

  QString qmsg;
  qmsg.append(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

  if (level <= L_DEBUG)
    qmsg.append(" DEBUG ");
  else if (level <= L_INFO)
    qmsg.append(" INFO  ");
  else if (level <= L_WARN)
    qmsg.append(" WARN  ");
  else if (level <= L_ERROR)
    qmsg.append(" ERROR ");

  qmsg.append(QString::fromStdString(msg));
  emit appendMessage(qmsg);
}
