
QByteArray NWAccess::cmdWasmReset(QByteArray args)
{
  WASM::host.reset();
  return makeOkReply();
}

QByteArray NWAccess::cmdWasmAdd(QByteArray args, QByteArray data)
{
  QByteArray reply;

  try {
    WASM::Module module = WASM::host.parse_module(reinterpret_cast<const uint8_t *>(data.constData()), data.size());

    // link wasm_bindings.cpp member functions:
    module.linkEx("*", "ppux_reset", wasmsig_ppux_reset, &WASM::RawCall<NWAccess>::adapter<&NWAccess::wasm_ppux_reset>, (const void *)this);
    module.linkEx("*", "ppux_sprite_write", wasmsig_ppux_sprite_write, &WASM::RawCall<NWAccess>::adapter<&NWAccess::wasm_ppux_sprite_write>, (const void *)this);
    module.linkEx("*", "ppux_ram_read", wasmsig_ppux_ram_read, &WASM::RawCall<NWAccess>::adapter<&NWAccess::wasm_ppux_ram_read>, (const void *)this);

    WASM::host.add_module(module);

    reply = makeOkReply();
  } catch (WASM::error& err) {
    reply = makeErrorReply(err.what());
  }

  return reply;
}

QByteArray NWAccess::cmdWasmInvoke(QByteArray args)
{
  QStringList items = QString::fromUtf8(args).split(';');

  QString reply = "name:";
  reply += items[0];

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

#include "wasm_bindings.cpp"
