
QByteArray NWAccess::cmdWasmReset(QByteArray args)
{
  WASM::host.reset();
  return makeOkReply();
}

QByteArray NWAccess::cmdWasmLoad(QByteArray args, QByteArray data)
{
  QStringList items = QString::fromUtf8(args).split(';');

  if (items.isEmpty()) {
    return makeErrorReply("missing module name");
  }
  std::string module_name = items.takeFirst().toStdString();

  QByteArray reply;
  try {

    std::shared_ptr<WASM::Module> module = WASM::host.parse_module(reinterpret_cast<const uint8_t *>(data.constData()), data.size());

    // link wasm functions:
    wasm_link(module);

    WASM::host.load_module(module_name, module);

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
    return makeErrorReply("missing module name");
  }
  std::string module_name = items.takeFirst().toStdString();

  QByteArray reply;
  try {
    WASM::host.unload_module(module_name);

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

    WASM::host.invoke_all(function_name, count, argv);

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
    return makeErrorReply("missing module name");
  }
  QString name = items.takeFirst();

  QString reply = "name:";
  reply += name;

  try {
    auto module = WASM::host.get_module(name.toStdString());
    module->msg_enqueue(std::shared_ptr<WASM::Message>(new WASM::Message((const uint8_t *)data.constData(), data.size())));
  } catch (std::out_of_range& err) {
    reply += "\nerror:";
    reply += err.what();
  }

  return makeHashReply(reply);
}

#include "wasm_bindings.cpp"
