
QByteArray NWAccess::cmdPpuxFontUpload(QByteArray args, QByteArray data)
{
  QStringList sargs = QString::fromUtf8(args).split(';');

  QString arg = sargs.takeFirst();
  auto fontindex = toInt(arg);

  try {
    wasmInterface.fonts.load_pcf(fontindex, (const uint8_t*)data.data(), data.size());
  } catch (std::runtime_error& err) {
    return makeErrorReply(err.what());
  }

  return makeOkReply();
}
