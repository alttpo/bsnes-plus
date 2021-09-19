
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
  try {
    wasmInterface.load_zip(instanceKey, reinterpret_cast<const uint8_t *>(data.constData()), data.size());

    //WASM::host.with_runtime(runtime_name, [&](const std::shared_ptr<WASM::Runtime>& runtime) {
    //  // instantiate and parse the module:
    //  std::shared_ptr<WASM::Module> module = runtime->parse_module(module_name, reinterpret_cast<const uint8_t *>(data.constData()), data.size());
    //
    //  // load the module into the runtime:
    //  runtime->load_module(module);
    //
    //  // link wasm functions:
    //  wasmInterface.link_module(module);
    //});

    reply = makeOkReply();
  } catch (WASMTrapError& err) {
    reply = makeErrorReply(err.what());
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
  } catch (WASMTrapError& err) {
    reply = makeErrorReply(err.what());
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
