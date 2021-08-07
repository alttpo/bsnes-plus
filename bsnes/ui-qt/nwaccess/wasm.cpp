
QByteArray NWAccess::cmdWasmReset(QByteArray args)
{
  wasmInterface.reset();
  return makeOkReply();
}

QByteArray NWAccess::cmdWasmLoad(QByteArray args, QByteArray data)
{
  QStringList items = QString::fromUtf8(args).split(';');

  if (items.isEmpty()) {
    return makeErrorReply("missing module name");
  }
  std::string module_name  = items.takeFirst().toStdString();

  QByteArray reply;
  try {
    wasmInterface.load_module(
      module_name,
      reinterpret_cast<const uint8_t *>(data.constData()),
      data.size()
    );

    reply = makeOkReply();
  } catch (WASMError& err) {
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
    // TODO: implement

    reply = makeOkReply();
  } catch (WASMError& err) {
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
    std::string function_name = function_name_qs.toStdString();

    // TODO:
    //wasmInterface.call(function_name, 0, 0);

    if (argv) delete[] argv;
  } catch (WASMError& err) {
    if (!reply.isEmpty()) reply.append('\n');
    reply += "error:";
    reply += err.what();
  }

  return makeHashReply(reply);
}

QByteArray NWAccess::cmdWasmMsgEnqueue(QByteArray args, QByteArray data) {
  //QStringList items = QString::fromUtf8(args).split(';');
  //
  //if (items.isEmpty()) {
  //  return makeErrorReply("missing runtime name");
  //}
  //QString name = items.takeFirst();
  //
  //QString reply = "name:";
  //reply += name;

  QByteArray reply;
  try {
    wasmInterface.msg_enqueue(
      std::shared_ptr<WASMMessage>(new WASMMessage(
        (const uint8_t *)data.constData(),
        data.size()
      ))
    );
    reply = makeOkReply();
  } catch (std::out_of_range& err) {
    reply = makeErrorReply(err.what());
  }

  return reply;
}
