
QByteArray NWAccess::cmdWasmReset(QByteArray args)
{
  wasmInterface.reset();
  return makeOkReply();
}

QByteArray NWAccess::cmdWasmLoad(QByteArray args, QByteArray data)
{
  QStringList items = QString::fromUtf8(args).split(';');

  if (items.isEmpty()) {
    return makeErrorReply("missing instance name");
  }
  std::string instanceKey = items.takeFirst().toStdString();

  QByteArray reply;
  if (!wasmInterface.load_zip(instanceKey, reinterpret_cast<const uint8_t *>(data.constData()), data.size())) {
    auto err = wasmInterface.last_error();
    reply = makeErrorReply(err.what().c_str());
  } else {
    reply = makeOkReply();
  }

  return reply;
}

QByteArray NWAccess::cmdWasmUnload(QByteArray args)
{
  QStringList items = QString::fromUtf8(args).split(';');
  if (items.isEmpty()) {
    return makeErrorReply("missing instance name");
  }
  std::string instanceKey = items.takeFirst().toStdString();

  QByteArray reply;
  try {
    wasmInterface.unload_zip(instanceKey);

    reply = makeOkReply();
  } catch (WASMError& err) {
    reply = makeErrorReply(err.what().c_str());
  }

  return reply;
}

QByteArray NWAccess::cmdWasmMsgEnqueue(QByteArray args, QByteArray data) {
  QStringList items = QString::fromUtf8(args).split(';');

  if (items.isEmpty()) {
    return makeErrorReply("missing instance name");
  }
  QString name = items.takeFirst();

  QString reply = "instance:";
  reply += name;

  try {
    std::string instanceKey = name.toStdString();
    wasmInterface.msg_enqueue(instanceKey, reinterpret_cast<const uint8_t *>(data.constData()), data.size());
  } catch (std::out_of_range& err) {
    reply += "\nerror:";
    reply += err.what();
  }

  return makeHashReply(reply);
}
