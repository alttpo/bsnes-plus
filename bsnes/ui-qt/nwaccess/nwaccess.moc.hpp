#include <QTcpServer>
#include <QObject>
#include <QMap>
#include <wasm/host.hpp>

class NWAccess : public QObject
{
    Q_OBJECT
public:
    NWAccess(QObject *parent = nullptr);

protected:
    QTcpServer *server;
    QMap<QObject*,QByteArray> buffers;
    
    static QByteArray makeHashReply(QString reply);
    static QByteArray makeEmptyListReply();
    static QByteArray makeErrorReply(QString error);
    static QByteArray makeOkReply();
    static QByteArray makeBinaryReply(const QByteArray &data);
    
#if defined(DEBUGGER)
    bool mapDebuggerMemory(const QString &memory, SNES::Debugger::MemorySource &source, unsigned &offset, unsigned &size);
#endif

    QByteArray cmdLoadCore(QString core);
    QByteArray cmdCoresList(QString platform);
    QByteArray cmdCoreInfo(QString core);
    QByteArray cmdCoreReset();
    QByteArray cmdCoreMemories();
    QByteArray cmdCoreRead(QString memory, QList< QPair<int,int> > &regions);
    QByteArray cmdCoreWrite(QString memory, QList< QPair<int,int> > &regions, QByteArray data);
    QByteArray cmdEmuInfo();
    QByteArray cmdEmuStatus();
    QByteArray cmdEmuReset();
    QByteArray cmdEmuStop();
    QByteArray cmdEmuPause();
    QByteArray cmdEmuResume();
    QByteArray cmdEmuReload();
    QByteArray cmdLoadGame(QString filename);
    QByteArray cmdGameInfo();
#if defined(DEBUGGER)
    QByteArray cmdDebugBreak();
    QByteArray cmdDebugContinue();
#endif

    QByteArray cmdWasmReset(QByteArray args);
    QByteArray cmdWasmLoad(QByteArray args, QByteArray data);
    QByteArray cmdWasmUnload(QByteArray args);
    QByteArray cmdWasmInvoke(QByteArray args);

    QByteArray cmdPpuxReset(QByteArray args);
    QByteArray cmdPpuxSpriteWrite(QByteArray args);
    QByteArray cmdPpuxSpriteRead(QByteArray args);
    QByteArray cmdPpuxRamWrite(QByteArray args, QByteArray data);
    QByteArray cmdPpuxRamRead(QByteArray args);

    // link functions:
    void wasm_link(WASM::Module& module);

    // wasm bindings:
#define decl_binding(name) \
    static const char *wasmsig_##name; \
    m3ApiRawFunction(wasm_##name)

    decl_binding(ppux_reset);
    decl_binding(ppux_sprite_read);
    decl_binding(ppux_sprite_write);
    decl_binding(ppux_ram_write);
    decl_binding(ppux_ram_read);

    decl_binding(snes_bus_read);
    decl_binding(snes_bus_write);

#undef decl_binding

public slots:
    void newConnection();
    void clientDisconnected();
    void clientDataReady();

};

extern NWAccess *nwaccess;
