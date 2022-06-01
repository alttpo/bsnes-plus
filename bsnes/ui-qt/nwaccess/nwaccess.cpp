#include "nwaccess.moc"
#include <QTcpSocket>
#include <QMap>
#include <QDataStream>


/* protocol: see https://github.com/usb2snes/emulator-networkaccess

 app -> emu:
 <COMMAND> <ARGS>\n
 binary data (if required by command):
   \0<len><data>

 emu -> app:
 binary ok:    \0<len><data>
 binary error: -> ascii error
 ascii ok:     \nkey:data\nkey:data\n\n
 ascii error:  \nerr:error message\n\n
*/


NWAccess *nwaccess;

static QString coreName = "bsnes-" + QString(SNES::Info::Profile).toLower();
static QString commands =
"EMULATOR_INFO,EMULATION_STATUS,EMULATION_PAUSE,EMULATION_RESUME,EMULATION_STOP,EMULATION_RESET,EMULATION_RELOAD"
",CORES_LIST,CORE_INFO,CORE_CURRENT_INFO,CORE_RESET,CORE_MEMORIES,CORE_READ,CORE_WRITE,LOAD_CORE"
",LOAD_GAME,GAME_INFO,MY_NAME_IS"
",WASM_RESET,WASM_ZIP_LOAD,WASM_ZIP_UNLOAD,WASM_MSG_ENQUEUE"
#if defined(DEBUGGER)
",DEBUG_BREAK,DEBUG_CONTINUE"
#endif
;

NWAccess::NWAccess(QObject *parent)
    : QObject(parent)
{
    server = new QTcpServer(this);
    for (quint16 port=65400; port<65410; port++) {
        if (server->listen(QHostAddress::LocalHost, port)) {
            qDebug() << "NWAccess Listening on localhost:" << port;
            break;
        }
    }
    if (!server->isListening()) {
        qDebug() << "NWAccess Error listening:" << server->errorString();
        return;
    }
    
    server->connect(server, &QTcpServer::newConnection, this, &NWAccess::newConnection);
}

void NWAccess::newConnection()
{
    QTcpSocket *conn = server->nextPendingConnection();
    if (!conn) return;
    conn->connect(conn, &QTcpSocket::disconnected, this, &NWAccess::clientDisconnected);
    conn->connect(conn, &QTcpSocket::readyRead, this, &NWAccess::clientDataReady);
}

void NWAccess::clientDisconnected()
{
    auto bufferIt = buffers.find(QObject::sender());
    if (bufferIt != buffers.end()) buffers.erase(bufferIt);
    auto clientIt = clients.find(QObject::sender());
    if (clientIt != clients.end()) clients.erase(clientIt);
}

static int toInt(const QString& s, int def=0)
{
    if (s.isEmpty()) return def;
    bool ok = false;
    int res = (s[0]=='$') ? s.midRef(1).toInt(&ok,16) : s.toInt(&ok);
    return ok ? res : def;
}

void NWAccess::clientDataReady()
{
    QAbstractSocket *socket = reinterpret_cast<QAbstractSocket*>(QObject::sender());
    QByteArray data = buffers[socket] + socket->readAll();
    Client& client = clients[socket];
    if (client.emulator_id.isEmpty()) client.emulator_id = QString::number(socket->localPort());
    while (data.length()>0) {
        if (data.front() == '\0') { // dangling binary data (from previous command that was not detected as binary)
            if (client.version == Client::Version::R1) {
                // in 1.0 we can reliably detect that there should not have been a binary block -> error out
                socket->write(client.makeErrorReply("protocol_error", "argument without command"));
                socket->close();
                return;
            } else {
                // before 1.0 we simply skip the block
                if (data.length()<5) break;
                quint32 len = qFromBigEndian<quint32>(data.constData()+1);
                if ((unsigned)data.length()-5<len) break;
                data = data.mid(5+len);
            }
        }
        int p = data.indexOf('\n');
        if (p>=0) { // received a complete command line
            int q = data.indexOf(' ');
            QByteArray cmd;
            QByteArray args;
            if (q>=0 && q<p) {
                cmd = data.left(q);
                args = data.mid(q+1, p-q-1);
            } else {
                cmd = data.left(p);
            }

            // detect protocol version
            if (cmd.startsWith("EMU_")) client.version = Client::Version::Alpha;
            else if (cmd.startsWith("EMUL")) client.version = Client::Version::R1;
            else if (cmd.startsWith("b")) client.version = Client::Version::R1;

            // detect if binary block should follow
            bool binarg = false;
            quint32 binlen = 0;
            if (client.version == Client::Version::R1) {
                binarg = (cmd[0] == 'b');
                if (binarg) cmd = cmd.mid(1);
            } else {
                binarg = (cmd == "CORE_WRITE");
            }

            // receive binary block
            if (binarg) {
                if (data.length()-p-1 < 1) break; // did not receive binary start yet
                if (data[p+1] != '\0') { // not a binary block
                    printf("bCMD bad data\n");
                    socket->write(client.makeErrorReply("invalid_argument", "no data"));
                    data = data.mid(p+1); // remove command from buffer
                    continue;
                }
                if (data.length()-p-1 < 5) break; // did not receive binary header yet
                binlen = qFromBigEndian<quint32>(data.constData()+p+1+1);
                if ((unsigned)data.length()-p-1-5<binlen) break; // did not receive complete binary data yet
            }

            // handle command
            if (cmd == "EMULATOR_INFO" || cmd == "EMU_INFO")
            {
                socket->write(client.cmdEmulatorInfo());
            }
            else if (cmd == "EMULATION_STATUS" || cmd == "EMU_STATUS")
            {
                socket->write(client.cmdEmulationStatus());
            }
            else if (cmd == "CORES_LIST")
            {
                socket->write(client.cmdCoresList(QString::fromUtf8(args)));
            }
            else if (cmd == "CORE_INFO")
            {
                socket->write(client.cmdCoreInfo(QString::fromUtf8(args)));
            }
            else if (cmd == "CORE_CURRENT_INFO")
            {
                socket->write(client.cmdCoreInfo(""));
            }
            else if (cmd == "CORE_RESET")
            {
                socket->write(client.cmdCoreReset());
            }
            else if (cmd == "CORE_MEMORIES")
            {
                socket->write(client.cmdCoreMemories());
            }
            else if (cmd == "CORE_READ")
            {
                QStringList sargs = QString::fromUtf8(args).split(';');
                QList< QPair<int,int> > ranges;
                for (int i=1; i<sargs.length(); i+=2) {
                    int addr=toInt(sargs[i]);
                    int len=(i+1<sargs.length()) ? toInt(sargs[i+1],-1) : -1;
                    ranges.push_back({addr,len});
                }
                socket->write(client.cmdCoreRead(sargs[0], ranges));
            }
            else if (cmd == "CORE_WRITE" && binarg)
            {
                QByteArray wr = data.mid(p+1+5, binlen);
                QStringList sargs = QString::fromUtf8(args).split(';');
                QList< QPair<int,int> > ranges;
                for (int i=1; i<sargs.length(); i+=2) {
                    int addr=toInt(sargs[i]);
                    int len=(i+1<sargs.length()) ? toInt(sargs[i+1],-1) : -1;
                    ranges.push_back({addr,len});
                }
                socket->write(client.cmdCoreWrite(sargs[0], ranges, wr));
            }
            else if (cmd == "LOAD_CORE")
            {
                socket->write(client.cmdLoadCore(QString::fromUtf8(args)));
            }
            else if (cmd == "LOAD_GAME")
            {
                socket->write(client.cmdLoadGame(QString::fromUtf8(args)));
            }
            else if (cmd == "GAME_INFO")
            {
                socket->write(client.cmdGameInfo());
            }
            else if (cmd == "EMULATION_PAUSE" || cmd == "EMU_PAUSE")
            {
                socket->write(client.cmdEmulationPause());
            }
            else if (cmd == "EMULATION_RESUME" || cmd == "EMU_RESUME")
            {
                socket->write(client.cmdEmulationResume());
            }
            else if (cmd == "EMULATION_STOP" || cmd == "EMU_STOP")
            {
                socket->write(client.cmdEmulationStop());
            }
            else if (cmd == "EMULATION_RESET" || cmd == "EMU_RESET")
            {
                socket->write(client.cmdEmulationReset());
            }
            else if (cmd == "EMULATION_RELOAD" || cmd == "EMU_RELOAD")
            {
                socket->write(client.cmdEmulationReload());
            }
            else if (cmd == "MY_NAME_IS")
            {
                client.version = Client::Version::R1;
                socket->write(client.cmdMyNameIs(QString::fromUtf8(args)));
            }
#if defined(DEBUGGER)
            else if (cmd == "DEBUG_BREAK")
            {
                socket->write(client.cmdDebugBreak());
            }
            else if (cmd == "DEBUG_CONTINUE")
            {
                socket->write(client.cmdDebugContinue());
            }
#endif
            else if (cmd == "WASM_RESET")
            {
                socket->write(client.cmdWasmReset(args));
            }
            else if (cmd == "WASM_ZIP_UNLOAD")
            {
                socket->write(client.cmdWasmUnload(args));
            }
            else if (cmd == "WASM_ZIP_LOAD")
            {
                QByteArray wr = data.mid(p + 1 + 5, binlen);
                socket->write(client.cmdWasmLoad(args, wr));
            }
            else if (cmd == "WASM_MSG_ENQUEUE")
            {
                QByteArray wr = data.mid(p + 1 + 5, binlen);
                socket->write(client.cmdWasmMsgEnqueue(args, wr));
            }
            else
            {
                socket->write(client.makeErrorReply("invalid_command", "unsupported command"));
            }
            data = data.mid(p + 1 + (binarg ? (5+binlen) : 0)); // remove command from buffer
        } else {
            break; // incomplete command
        }
    }
    buffers[socket] = data;
    socket->flush();
}


QByteArray NWAccess::Client::makeHashReply(QString reply)
{
    if (reply.isEmpty()) return "\n\n";
    return "\n" + reply.toUtf8() + (reply.endsWith("\n") ? "\n" : "\n\n");
}


QByteArray NWAccess::Client::makeHashReply(QList<QPair<QString,QString>> reply)
{
    if (reply.isEmpty()) return "\n\n";
    QByteArray buf;
    for (auto& pair: reply)
        buf += "\n" + pair.first.toUtf8() + ":" + pair.second.toUtf8();
    return buf + "\n\n";
}

QByteArray NWAccess::Client::makeEmptyListReply()
{
    if (version == Version::Alpha)
        return "\nnone:none\n\n";
    return "\n\n";
}

QByteArray NWAccess::Client::makeErrorReply(QString type, QString msg)
{
    if (version == Version::Alpha)
        return "\nerror:" + msg.replace('\n',' ').toUtf8() + "\n\n";
    return "\nerror:" + type.replace('\n',' ').toUtf8() +
           "\nreason:" + msg.replace('\n',' ').toUtf8() + "\n\n";
}

QByteArray NWAccess::Client::makeOkReply()
{
    if (version == Version::Alpha)
        return "\nok\n\n";
    return "\n\n";
}

QByteArray NWAccess::Client::makeBinaryReply(const QByteArray &data)
{
    QByteArray reply(5,'\0');
    qToBigEndian((quint32)data.length(), reply.data()+1);
    reply += data;
    return reply;
}


QByteArray NWAccess::Client::cmdCoresList(QString platform)
{
    if (!platform.isEmpty() && platform.toLower() != "snes")
        return makeEmptyListReply();
    return makeHashReply("platform:snes\nname:"+coreName);
}

QByteArray NWAccess::Client::cmdCoreInfo(QString core)
{
    if (!core.isEmpty() && core != coreName) // only one core supported
        return makeErrorReply("invalid_argument", "no such core \"" + core + "\"");
    return makeHashReply("platform:snes\nname:"+coreName);
}

QByteArray NWAccess::Client::cmdLoadCore(QString core)
{    
    if (core != coreName) // can not change (load/unload) core
        return makeErrorReply("invalid_command", "not supported");
    return makeOkReply();
}

QByteArray NWAccess::Client::cmdCoreMemories()
{
#if defined(DEBUGGER)
    QString s;
    for (const QString& memory: {"CARTROM", "WRAM", "SRAM", "VRAM", "OAM", "CGRAM"}) {
        SNES::Debugger::MemorySource source;
        unsigned offset, size;
        if (mapDebuggerMemory(memory, source, offset, size))
            s += "name:" + memory + "\naccess:rw\nsize:" + QString::number(size) + "\n";
    }
    return makeHashReply(s);
#else
    // TODO: regular bsnes
    return makeEmptyListReply();
#endif
}

QByteArray NWAccess::Client::cmdEmulationStatus()
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
        QString name = notdir(SNES::cartridge.basename());
        game = name.replace("\n"," ");
    }
    return makeHashReply(QString("state:") + state + "\n" +
                         QString("game:") + game);
}

QByteArray NWAccess::Client::cmdEmulatorInfo()
{
    if (version == Version::Alpha)
        return makeHashReply(QString("name:") + SNES::Info::Name + "\n" +
                             QString("version:") + SNES::Info::Version + "\n");

    // FIXME: sometimes this crashes when running right after loading ROM ?!
    return makeHashReply({
        {"name", SNES::Info::Name},
        {"version", SNES::Info::Version},
        {"nwa_version", "1.0"},
        {"id", emulator_id},
        {"commands", commands},
    });
}

QByteArray NWAccess::Client::cmdCoreReset()
{
    // this is a hard reset
    utility.modifySystemState(Utility::PowerCycle);
    return makeOkReply();
}

QByteArray NWAccess::Client::cmdEmulationReset()
{
    // this is a soft reset
    utility.modifySystemState(Utility::Reset);
    return makeOkReply();
}

QByteArray NWAccess::Client::cmdEmulationStop()
{
    utility.modifySystemState(Utility::PowerOff);
    return makeOkReply();
}

QByteArray NWAccess::Client::cmdEmulationPause()
{
    application.pause = true;
    return makeOkReply();
}

QByteArray NWAccess::Client::cmdEmulationResume()
{
    if (!SNES::cartridge.loaded()) {
        return makeErrorReply("not_allowed", "no game loaded");
    }
#if defined(DEBUGGER)
    else if (application.debug) {
        // use DEBUG_CONTINUE to continue from breakpoint
        return makeErrorReply("not_allowed", "in breakpoint");
    }
#endif
    else {
        application.pause = false;
        if (!application.power)
            mainWindow->power();
        return makeOkReply();
    }
}

QByteArray NWAccess::Client::cmdEmulationReload()
{
    if (SNES::cartridge.loaded()) {
        application.reloadCartridge();
        return makeOkReply();
    } else {
        cmdEmulationStop();
        return cmdEmulationResume();
    }
}

QByteArray NWAccess::Client::cmdLoadGame(QString path)
{
    if (path.isEmpty()) {
        utility.modifySystemState(Utility::UnloadCartridge);
        return makeOkReply();
    }
    
    string f = path.toUtf8().constData();
    if (!file::exists(f)) return makeErrorReply("invalid_argument", "no such file");
    
    utility.modifySystemState(Utility::UnloadCartridge);
    cartridge.loadNormal(f); // TODO: make this depend on file ext
    utility.modifySystemState(Utility::LoadCartridge);
    if (SNES::cartridge.loaded()) return makeOkReply();
    return makeErrorReply("invalid_argument", "could not load game");
}

QByteArray NWAccess::Client::cmdGameInfo()
{
    if (!SNES::cartridge.loaded() || !cartridge.baseName || !SNES::cartridge.basename()) return makeHashReply("");
    
    QString file = cartridge.baseName;
    QString name = notdir(SNES::cartridge.basename());
    name = name.replace("\n"," ");
    QString region = SNES::cartridge.region()==SNES::Cartridge::Region::NTSC ? "NTSC" : "PAL";
    return makeHashReply("name:" + name + "\n" +
                         "file:" + file + "\n" +
                         "region:" + region);
}

QByteArray NWAccess::Client::cmdMyNameIs(QString)
{
    return makeHashReply("name:" + QString(SNES::Info::Name) + " " + emulator_id);
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

QByteArray NWAccess::Client::cmdCoreRead(QString memory, QList< QPair<int,int> > &regions)
{
    // verify args
    if (regions.length()>1)
        for (const auto& pair: regions)
            if (pair.second<1)
                return makeErrorReply("invalid_argument", "bad format");
    if (regions.isEmpty()) // no region = read all
        regions.push_back({0,-1});
    QByteArray data;
#if defined(DEBUGGER)
    SNES::Debugger::MemorySource source;
    unsigned offset;
    unsigned size;
    if (!mapDebuggerMemory(memory, source, offset, size))
        return makeErrorReply("invalid_argument", "unknown memory");
    bool loaded = SNES::cartridge.loaded();
    if (loaded || version == Version::R1) {
        // NOTE: this is probably not fully 1.0 compilant yet, but it passes the current tests
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
    return makeErrorReply("invalid_command", "not implemented");
#endif
    return makeBinaryReply(data);
}

QByteArray NWAccess::Client::cmdCoreWrite(QString memory, QList< QPair<int,int> >& regions, QByteArray data)
{
    // verify args
    int expectedLen=(regions.length()) ? 0 : -1;
    for (const auto& pair: regions) {
        if (regions.length()>1)
            if (pair.second<1)
                return makeErrorReply("invalid_argument", "bad format");
        expectedLen += pair.second;
    }
    if (expectedLen>=0 && data.length() != expectedLen) // data len != regions
        return makeErrorReply("invalid_argument", "bad data length");
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
        return makeErrorReply("invalid_argument", "unknown memory");
    bool loaded = SNES::cartridge.loaded();
    if (loaded) {
        // NOTE: this is probably not fully 1.0 compilant yet, but it passes the current tests
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
    return makeErrorReply("invalid_command", "not implemented");
#endif
    return makeOkReply();
}

#if defined(DEBUGGER)
QByteArray NWAccess::Client::cmdDebugBreak()
{
    if (!application.debug) debugger->toggleRunStatus();
    return makeOkReply();
}

QByteArray NWAccess::Client::cmdDebugContinue()
{
    if (application.debug) debugger->toggleRunStatus();
    return makeOkReply();
}
#endif

#include "wasm.cpp"
