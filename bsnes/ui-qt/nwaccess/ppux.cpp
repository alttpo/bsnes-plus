
QByteArray NWAccess::cmdPpuxFontUpload(QByteArray args, QByteArray data)
{
  QStringList items = QString::fromUtf8(args).split(';');

  if (items.isEmpty()) {
    return makeErrorReply("missing runtime name");
  }
  QString name = items.takeFirst();

  QString reply = "runtime:";
  reply += name;

  if (items.isEmpty()) {
    return makeErrorReply("missing font index");
  }
  QString arg = items.takeFirst();
  auto fontindex = toInt(arg);

  reply += "\nfontindex:";
  reply += arg;

  try {
    std::string runtime_name = name.toStdString();
    WASM::host.with_runtime(runtime_name, [&](const std::shared_ptr<WASM::Runtime>& runtime) {
      runtime->fonts.load_pcf(fontindex, (const uint8_t*)data.data(), data.size());
    });
  } catch (std::runtime_error& err) {
    return makeErrorReply(err.what());
  }

  return makeOkReply();
}
