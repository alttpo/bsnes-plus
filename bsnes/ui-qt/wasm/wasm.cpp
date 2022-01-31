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

  // TODO: add debug/info/warn/error message logging API
  wasmInterface.register_error_receiver([&](const WASMError& err) {
    QString msg;
    msg.append(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs) + " ERROR ");
    msg.append(QString::fromStdString(err.what()));
    emit appendMessage(msg);
  });
}
