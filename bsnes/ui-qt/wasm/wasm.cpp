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
  QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  monospace.setPointSize(12);
  monospace.setLetterSpacing(QFont::AbsoluteSpacing, 0);
  log->setFont(monospace);
  layout->addWidget(log);

  connect(this, SIGNAL(appendPlainText(const QString&)), log, SLOT(appendPlainText(const QString&)));
  connect(this, SIGNAL(appendHtml(const QString&)), log, SLOT(appendHtml(const QString&)));

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

  QString qmsg("<p>");
  qmsg.append("<span>");
  qmsg.append(QDateTime::currentDateTimeUtc().time().toString(Qt::ISODateWithMs).toHtmlEscaped());
  qmsg.append("</span>");

  if (level <= L_DEBUG) {
    qmsg.append(" <span style='color:green;'>DEBUG ");
  }
  else if (level <= L_INFO) {
    qmsg.append(" <span style='color:gray'>INFO&nbsp; ");
  }
  else if (level <= L_WARN) {
    qmsg.append(" <span style='color:yellow'>WARN&nbsp; ");
  }
  else if (level <= L_ERROR) {
    qmsg.append(" <span style='color:red'>ERROR ");
  }

  qmsg.append(QString::fromStdString(msg).toHtmlEscaped());
  qmsg.append("</span>");
  qmsg.append("</p>");

  emit appendHtml(qmsg);
}