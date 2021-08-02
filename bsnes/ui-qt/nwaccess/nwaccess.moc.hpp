#include <QTcpServer>
#include <QObject>
#include <QMap>

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
    QByteArray cmdWasmMsgEnqueue(QByteArray args, QByteArray data);

    QByteArray cmdPpuxReset(QByteArray args);
    QByteArray cmdPpuxSpriteReset(QByteArray args);
    QByteArray cmdPpuxSpriteWrite(QByteArray args);
    QByteArray cmdPpuxSpriteRead(QByteArray args);
    QByteArray cmdPpuxRamWrite(QByteArray args, QByteArray data);
    QByteArray cmdPpuxRamRead(QByteArray args);

public slots:
    void newConnection();
    void clientDisconnected();
    void clientDataReady();

};

extern NWAccess *nwaccess;
