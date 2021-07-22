
QByteArray NWAccess::makeHashReply(QString reply)
{
  if (reply.isEmpty()) return "\n\n";
  return "\n" + reply.toUtf8() + (reply.endsWith("\n") ? "\n" : "\n\n");
}

QByteArray NWAccess::makeEmptyListReply()
{
  return "\nnone:none\n\n"; // NOTE: we may get rid of this
}

QByteArray NWAccess::makeErrorReply(QString error)
{
  return "\nerror:" + error.replace('\n',' ').toUtf8() + "\n\n";
}

QByteArray NWAccess::makeOkReply()
{
  return "\nok\n\n"; // NOTE: we may get rid of this
}

QByteArray NWAccess::makeBinaryReply(const QByteArray &data)
{
  QByteArray reply(5,'\0');
  qToBigEndian((quint32)data.length(), reply.data()+1);
  reply += data;
  return reply;
}


QByteArray NWAccess::cmdCoresList(QString platform)
{
  if (!platform.isEmpty() && platform.toLower() != "snes")
    return makeEmptyListReply();
  return makeHashReply("platform:snes\nname:"+coreName);
}

QByteArray NWAccess::cmdCoreInfo(QString core)
{
  if (!core.isEmpty() && core != coreName) // only one core supported
    return makeErrorReply("no such core");
  return makeHashReply("platform:snes\nname:"+coreName);
}

QByteArray NWAccess::cmdLoadCore(QString core)
{
  if (core != coreName) // can not change (load/unload) core
    return makeErrorReply("not supported");
  return makeOkReply();
}

QByteArray NWAccess::cmdCoreMemories()
{
#if defined(DEBUGGER)
  return makeHashReply("name:CARTROM\n" "access:rw\n"
                         "name:WRAM\n"    "access:rw\n"
                         "name:SRAM\n"    "access:rw\n"
                         "name:VRAM\n"    "access:rw\n"
                         "name:OAM\n"     "access:rw\n"
                         "name:CGRAM\n"   "access:rw");
#else
  // TODO: regular bsnes
  return makeEmptyListReply();
#endif
}

QByteArray NWAccess::cmdEmuStatus()
{
  bool loaded = SNES::cartridge.loaded();
  bool stopped = !application.power;
  bool paused = application.pause || application.autopause;
#if defined(DEBUGGER)
  paused |= application.debug;
#endif
  const char* state = !loaded ? "no_game" : stopped ? "stopped" : paused ? "paused" : "running";
  QString game;
  if (SNES::cartridge.loaded()) {
    QString name = SNES::cartridge.basename();
    int p = name.lastIndexOf('/');
    int q = name.lastIndexOf('\\');
    game = name.mid(p>q ? p+1 : q+1).replace("\n"," ");
  }
  return makeHashReply(QString("state:") + state + "\n" +
                       QString("game:") + game);
}

QByteArray NWAccess::cmdEmuInfo()
{
  return makeHashReply(QString("name:") + SNES::Info::Name + "\n" +
                       QString("version:") + SNES::Info::Version);
}

QByteArray NWAccess::cmdCoreReset()
{
  // this is a hard reset
  utility.modifySystemState(Utility::PowerCycle);
  return makeOkReply();
}

QByteArray NWAccess::cmdEmuReset()
{
  // this is a soft reset
  utility.modifySystemState(Utility::Reset);
  return makeOkReply();
}

QByteArray NWAccess::cmdEmuStop()
{
  utility.modifySystemState(Utility::PowerOff);
  return makeOkReply();
}

QByteArray NWAccess::cmdEmuPause()
{
  application.pause = true;
  return makeOkReply();
}

QByteArray NWAccess::cmdEmuResume()
{
  if (!SNES::cartridge.loaded()) {
    return makeErrorReply("no game loaded");
  }
#if defined(DEBUGGER)
    else if (application.debug) {
        // use DEBUG_CONTINUE to continue from breakpoint
        return makeErrorReply("in breakpoint");
    }
#endif
  else {
    application.pause = false;
    if (!application.power)
      mainWindow->power();
    return makeOkReply();
  }
}
QByteArray NWAccess::cmdEmuReload()
{
  if (SNES::cartridge.loaded()) {
    application.reloadCartridge();
    return makeOkReply();
  } else {
    cmdEmuStop();
    return cmdEmuResume();
  }
}

QByteArray NWAccess::cmdLoadGame(QString path)
{
  if (path.isEmpty()) {
    utility.modifySystemState(Utility::UnloadCartridge);
    return makeOkReply();
  }

  string f = path.toUtf8().constData();
  if (!file::exists(f)) return makeErrorReply("no such file");

  utility.modifySystemState(Utility::UnloadCartridge);
  cartridge.loadNormal(f); // TODO: make this depend on file ext
  utility.modifySystemState(Utility::LoadCartridge);
  if (SNES::cartridge.loaded()) return makeOkReply();
  return makeErrorReply("could not load game");
}
QByteArray NWAccess::cmdGameInfo()
{
  if (!SNES::cartridge.loaded()) return makeHashReply("");

  QString file = cartridge.baseName;
  QString name = SNES::cartridge.basename();
  int p = name.lastIndexOf('/');
  int q = name.lastIndexOf('\\');
  name = name.mid(p>q ? p+1 : q+1).replace("\n"," ");
  QString region = SNES::cartridge.region()==SNES::Cartridge::Region::NTSC ? "NTSC" : "PAL";
  return makeHashReply("name:" + name + "\n" +
                       "file:" + file + "\n" +
                       "region:" + region);
}

#if defined(DEBUGGER)
bool NWAccess::mapDebuggerMemory(const QString &memory, SNES::Debugger::MemorySource &source, unsigned &offset, unsigned &size)
{
    offset = 0;
    size = 0xffffff;

    if (memory == "WRAM") {
        source = SNES::Debugger::MemorySource::CPUBus;
        offset = 0x7e0000;
        size = 0x020000;
    } else if (memory == "SRAM") {
        source = SNES::Debugger::MemorySource::CartRAM;
        size = SNES::cartridge.loaded() ? SNES::memory::cartram.size() : 0;
    } else if (memory == "CARTROM") {
        source = SNES::Debugger::MemorySource::CartROM;
        size = SNES::cartridge.loaded() ? SNES::memory::cartrom.size() : 0;
    } else if (memory == "VRAM") {
        source = SNES::Debugger::MemorySource::VRAM;
        size = SNES::memory::vram.size();
    } else if (memory == "OAM") {
        source = SNES::Debugger::MemorySource::OAM;
        size = 544;
    } else if (memory == "CGRAM") {
        source = SNES::Debugger::MemorySource::CGRAM;
        size = 512;
    } else {
        size = 0;
        return false;
    }
    return true;
}
#endif


QByteArray NWAccess::cmdCoreRead(QString memory, QList< QPair<int,int> > &regions)
{
  // verify args
  if (regions.length()>1)
    for (const auto& pair: regions)
      if (pair.second<1)
        return makeErrorReply("bad format");
  if (regions.isEmpty()) // no region = read all
    regions.push_back({0,-1});
  QByteArray data;
#if defined(DEBUGGER)
  SNES::Debugger::MemorySource source;
    unsigned offset;
    unsigned size;
    if (!mapDebuggerMemory(memory, source, offset, size))
        return makeErrorReply("unknown memory");
    if (SNES::cartridge.loaded()) {
        SNES::debugger.bus_access = true;
        for (auto it=regions.begin(); it!=regions.end();) {
            int start = (it->first>=0) ? (unsigned)it->first : 0;
            int len = (it->second>=0) ? it->second : (start>=size) ? 0 : (size-start);
            unsigned pad = 0;
            it++;
            if (it == regions.end() && len+start>size) {
                // out of bounds for last addr/offset -> short reply
                len = (start>=size) ? 0 : (size-start);
            }
            else if (start>size) {
                // completely out of bounds
                pad = len;
                len = 0;
            }
            else if (len+start>size) {
                // partially out of bounds
                pad = len-(size-start);
                len = size-start;
            }
            for (int addr=offset+start; addr<offset+start+len; addr++)
                data += SNES::debugger.read(source, addr);
            for (unsigned i=0; i<pad; i++)
                data += '\0';
        }
        SNES::debugger.bus_access = false;
    } else if (regions.length()>1) {
        // all regions out of bounds, we return empty data here (no padding)
        // the client has to handle that special case
    } else {
        // last = only region out of bounds, we return empty data
    }
#else
  // TODO: regular bsnes
  return makeErrorReply("not implemented");
#endif
  return makeBinaryReply(data);
}

QByteArray NWAccess::cmdCoreWrite(QString memory, QList< QPair<int,int> >& regions, QByteArray data)
{
  // verify args
  int expectedLen=(regions.length()) ? 0 : -1;
  for (const auto& pair: regions) {
    if (regions.length()>1)
      if (pair.second<1)
        return makeErrorReply("bad format");
    expectedLen += pair.second;
  }
  if (expectedLen>=0 && data.length() != expectedLen) // data len != regions
    return makeErrorReply("bad data length");
  if (regions.isEmpty()) // no region = write all
    regions.push_back({0,data.length()});
  else if (regions[0].second < 0) // address only
    regions[0].second = data.length();
#if defined(DEBUGGER)
  // bsnes-plus with DEBUGGER compiled in
    SNES::Debugger::MemorySource source;
    unsigned offset;
    unsigned size;
    if (!mapDebuggerMemory(memory, source, offset, size))
        return makeErrorReply("unknown memory");
    if (SNES::cartridge.loaded()) {
        if (memory == "CARTROM" && size<0x800000) size = 0x800000;
        SNES::debugger.bus_access = true;
        const uint8_t *p = (uint8_t*)data.constData();
        for (const auto& pair: regions) {
            unsigned start = (pair.first>=0) ? (unsigned)pair.first : 0;
            unsigned len = pair.second;
            for (unsigned addr=offset+start; addr<offset+start+len; addr++) {
                if (addr-offset<size) SNES::debugger.write(source, addr, *p);
                p++;
            }
        }
        SNES::debugger.bus_access = false;
    } else {
        // if no game is loaded we can just silently ignore the write
    }
#else
  // TODO: regular bsnes
  return makeErrorReply("not implemented");
#endif
  return makeOkReply();
}

#if defined(DEBUGGER)
QByteArray NWAccess::cmdDebugBreak()
{
    if (!application.debug) debugger->toggleRunStatus();
    return makeOkReply();
}

QByteArray NWAccess::cmdDebugContinue()
{
    if (application.debug) debugger->toggleRunStatus();
    return makeOkReply();
}
#endif
