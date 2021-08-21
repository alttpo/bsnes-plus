
QByteArray NWAccess::cmdWasmReset(QByteArray args)
{
  WASM::host.reset();
  return makeOkReply();
}

QByteArray NWAccess::cmdWasmLoad(QByteArray args, QByteArray data)
{
  QStringList items = QString::fromUtf8(args).split(';');

  if (items.isEmpty()) {
    return makeErrorReply("missing runtime name");
  }
  std::string runtime_name = items.takeFirst().toStdString();

  if (items.isEmpty()) {
    return makeErrorReply("missing module name");
  }
  std::string module_name  = items.takeFirst().toStdString();

  QByteArray reply;
  try {
    WASM::host.with_runtime(runtime_name, [&](const std::shared_ptr<WASM::Runtime>& runtime) {
      // instantiate and parse the module:
      std::shared_ptr<WASM::Module> module = runtime->parse_module(module_name, reinterpret_cast<const uint8_t *>(data.constData()), data.size());

      // load the module into the runtime:
      runtime->load_module(module);

      // link wasm functions:
      wasmInterface.link_module(module);
    });

    reply = makeOkReply();
  } catch (WASM::error& err) {
    reply = makeErrorReply(err.what());
  }

  return reply;
}

QByteArray NWAccess::cmdWasmUnload(QByteArray args)
{
  QStringList items = QString::fromUtf8(args).split(';');
  if (items.isEmpty()) {
    return makeErrorReply("missing runtime name");
  }
  std::string runtime_name = items.takeFirst().toStdString();

  QByteArray reply;
  try {
    WASM::host.unload_runtime(runtime_name);

    reply = makeOkReply();
  } catch (WASM::error& err) {
    reply = makeErrorReply(err.what());
  }

  return reply;
}

QByteArray NWAccess::cmdWasmInvoke(QByteArray args)
{
  QStringList items = QString::fromUtf8(args).split(';');

  if (items.isEmpty()) {
    return makeErrorReply("missing function name");
  }
  QString name = items.takeFirst();

  QString reply = "name:";
  reply += name;

  try {
    const char **argv = nullptr;
    size_t count = items.size() - 1;
    if (count > 0) {
      argv = new const char *[count];
      for (unsigned i = 0; i < count; i++) {
        QString item = items.at(i + 1);
        reply += QString("\np%1:%2").arg(i).arg(item);

        argv[i] = item.toUtf8().constData();
      }
    }

    QString function_name_qs = items[0];
    QByteArray function_name_qba = function_name_qs.toUtf8();
    const char *function_name = function_name_qba.constData();

    WASM::host.each_runtime([&](const std::shared_ptr<WASM::Runtime>& runtime) {
      runtime->with_function(function_name, [&](WASM::Function& f){
        f.callargv(count, argv);
      });
    });

    if (argv) delete[] argv;
  } catch (WASM::error& err) {
    if (!reply.isEmpty()) reply.append('\n');
    reply += "error:";
    reply += err.what();
  }

  return makeHashReply(reply);
}

QByteArray NWAccess::cmdWasmMsgEnqueue(QByteArray args, QByteArray data) {
  QStringList items = QString::fromUtf8(args).split(';');

  if (items.isEmpty()) {
    return makeErrorReply("missing runtime name");
  }
  QString name = items.takeFirst();

  QString reply = "runtime:";
  reply += name;

  try {
    std::string runtime_name = name.toStdString();
    WASM::host.with_runtime(runtime_name, [&](const std::shared_ptr<WASM::Runtime>& runtime) {
      runtime->msg_enqueue(
        std::shared_ptr<WASM::Message>(new WASM::Message((const uint8_t *)data.constData(), data.size()))
      );
    });
  } catch (std::out_of_range& err) {
    reply += "\nerror:";
    reply += err.what();
  }

  return makeHashReply(reply);
}
