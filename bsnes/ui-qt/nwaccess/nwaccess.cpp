#include "nwaccess.moc"
#include <QTcpSocket>
#include <QMap>


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
    auto it = buffers.find(QObject::sender());
    if (it != buffers.end()) buffers.erase(it);
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
    while (data.length()>0) {
        if (data.front() == '\0') { // dangling binary data (from previous command that was not supported)
            if (data.length()<5) break;
            quint32 len = qFromBigEndian<quint32>(data.constData()+1);
            if ((unsigned)data.length()-5<len) break;
            data = data.mid(5+len); // skip over binary block
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
            if (cmd == "EMU_INFO")
            {
                socket->write(cmdEmuInfo());
            }
            else if (cmd == "EMU_STATUS")
            {
                socket->write(cmdEmuStatus());
            }
            else if (cmd == "CORES_LIST")
            {
                socket->write(cmdCoresList(QString::fromUtf8(args)));
            }
            else if (cmd == "CORE_INFO")
            {
                socket->write(cmdCoreInfo(QString::fromUtf8(args)));
            }
            else if (cmd == "CORE_CURRENT_INFO")
            {
                socket->write(cmdCoreInfo(""));
            }
            else if (cmd == "CORE_RESET")
            {
                socket->write(cmdCoreReset());
            }
            else if (cmd == "CORE_MEMORIES")
            {
                socket->write(cmdCoreMemories());
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
                socket->write(cmdCoreRead(sargs[0], ranges));
            }
            else if (cmd == "CORE_WRITE")
            {
                if (data.length()-p-1 < 1) break; // did not receive binary start
                if (data[p+1] != '\0') { // no binary data
                    socket->write(makeErrorReply("no data"));
                } else {
                    if (data.length()-p-1 < 5) break; // did not receive binary header yet
                    quint32 len = qFromBigEndian<quint32>(data.constData()+p+1+1);
                    if ((unsigned)data.length()-p-1-5<len) break; // did not receive complete binary data yet
                    QByteArray wr = data.mid(p+1+5, len);
                    QStringList sargs = QString::fromUtf8(args).split(';');
                    QList< QPair<int,int> > ranges;
                    for (int i=1; i<sargs.length(); i+=2) {
                        int addr=toInt(sargs[i]);
                        int len=(i+1<sargs.length()) ? toInt(sargs[i+1],-1) : -1;
                        ranges.push_back({addr,len});
                    }
                    socket->write(cmdCoreWrite(sargs[0], ranges, wr));
                    data = data.mid(p+1+5+len); // remove wr data from buffer
                    continue;
                }
            }
            else if (cmd == "LOAD_CORE")
            {
                socket->write(cmdLoadCore(QString::fromUtf8(args)));
            }
            else if (cmd == "LOAD_GAME")
            {
                socket->write(cmdLoadGame(QString::fromUtf8(args)));
            }
            else if (cmd == "GAME_INFO")
            {
                socket->write(cmdGameInfo());
            }
            else if (cmd == "EMU_PAUSE")
            {
                socket->write(cmdEmuPause());
            }
            else if (cmd == "EMU_RESUME")
            {
                socket->write(cmdEmuResume());
            }
            else if (cmd == "EMU_STOP")
            {
                socket->write(cmdEmuStop());
            }
            else if (cmd == "EMU_RESET")
            {
                socket->write(cmdEmuReset());
            }
            else if (cmd == "EMU_RELOAD")
            {
                socket->write(cmdEmuReload());
            }
#if defined(DEBUGGER)
            else if (cmd == "DEBUG_BREAK")
            {
                socket->write(cmdDebugBreak());
            }
            else if (cmd == "DEBUG_CONTINUE")
            {
                socket->write(cmdDebugContinue());
            }
#endif
            else if (cmd == "WASM_RESET")
            {
                socket->write(cmdWasmReset(args));
            }
            else if (cmd == "WASM_ADD")
            {
                if (data.length()-p-1 < 1) break; // did not receive binary start
                if (data[p+1] != '\0') { // no binary data
                    socket->write(makeErrorReply("no data"));
                } else {
                    if (data.length() - p - 1 < 5) break; // did not receive binary header yet
                    quint32 len = qFromBigEndian<quint32>(data.constData() + p + 1 + 1);
                    if ((unsigned) data.length() - p - 1 - 5 < len) break; // did not receive complete binary data yet

                    QByteArray wr = data.mid(p + 1 + 5, len);
                    socket->write(cmdWasmAdd(args, wr));

                    data = data.mid(p+1+5+len); // remove wr data from buffer
                    continue;
                }
            }
            else if (cmd == "WASM_INVOKE")
            {
                socket->write(cmdWasmInvoke(args));
            }
            else if (cmd == "PPUX_RESET")
            {
                socket->write(cmdPpuxReset(args));
            }
            else if (cmd == "PPUX_SPR_WRITE")
            {
                socket->write(cmdPpuxSpriteWrite(args));
            }
            else if (cmd == "PPUX_RAM_READ")
            {
                socket->write(cmdPpuxRamRead(args));
            }
            else if (cmd == "PPUX_RAM_WRITE")
            {
                if (data.length()-p-1 < 1) break; // did not receive binary start
                if (data[p+1] != '\0') { // no binary data
                    socket->write(makeErrorReply("no data"));
                } else {
                    if (data.length()-p-1 < 5) break; // did not receive binary header yet
                    quint32 len = qFromBigEndian<quint32>(data.constData()+p+1+1);
                    if ((unsigned)data.length()-p-1-5 < len) break; // did not receive complete binary data yet

                    QByteArray wr = data.mid(p+1+5, len);
                    socket->write(cmdPpuxRamWrite(args, wr));

                    data = data.mid(p+1+5+len); // remove wr data from buffer
                    continue;
                }
            }
            else
            {
                socket->write("\nerror:unsupported command\n\n");
            }
            data = data.mid(p+1); // remove command from buffer
        } else {
            break; // incomplete command
        }
    }
    buffers[socket] = data;
    socket->flush();
}

#include "commands.cpp"
#include "ppux.cpp"
#include "wasm.cpp"
