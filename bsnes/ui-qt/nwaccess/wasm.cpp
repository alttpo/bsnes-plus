
QByteArray NWAccess::Client::cmdWasmReset(QByteArray args)
{
  wasmInterface.reset();
  return makeOkReply();
}

QByteArray NWAccess::Client::cmdWasmLoad(QByteArray args, QByteArray data)
{
  QStringList items = QString::fromUtf8(args).split(';');

  if (items.isEmpty()) {
    return makeErrorReply("instance_missing", "missing instance name");
  }
  std::string instanceKey = items.takeFirst().toStdString();

  QByteArray reply;
  if (!wasmInterface.load_zip(instanceKey, reinterpret_cast<const uint8_t *>(data.constData()), data.size())) {
    auto err = wasmInterface.last_error();
    reply = makeErrorReply("wasm_error", err.what().c_str());
  } else {
    reply = makeOkReply();
  }

  return reply;
}

QByteArray NWAccess::Client::cmdWasmUnload(QByteArray args)
{
  QStringList items = QString::fromUtf8(args).split(';');
  if (items.isEmpty()) {
    return makeErrorReply("instance_missing", "missing instance name");
  }
  std::string instanceKey = items.takeFirst().toStdString();

  wasmInterface.unload_zip(instanceKey);

  return makeOkReply();
}

QByteArray NWAccess::Client::cmdWasmMsgEnqueue(QByteArray args, QByteArray data) {
  QStringList items = QString::fromUtf8(args).split(';');

  if (items.isEmpty()) {
    return makeErrorReply("instance_missing", "missing instance name");
  }
  QString name = items.takeFirst();

  std::string instanceKey = name.toStdString();
  if (!wasmInterface.msg_enqueue(instanceKey, reinterpret_cast<const uint8_t *>(data.constData()), data.size())) {
    auto err = wasmInterface.last_error();
    return makeErrorReply("wasm_error", err.what().c_str());
  }

  return makeOkReply();
}
