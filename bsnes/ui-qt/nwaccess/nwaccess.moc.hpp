#include <QTcpServer>
#include <QObject>
#include <QMap>

class NWAccess : public QObject
{
    Q_OBJECT
public:
    NWAccess(QObject *parent = nullptr);

protected:
    struct Client;

    QTcpServer *server;
    QMap<QObject*,QByteArray> buffers;
    QMap<QObject*,Client> clients;

#if defined(DEBUGGER)
    static bool mapDebuggerMemory(const QString &memory, SNES::Debugger::MemorySource &source, unsigned &offset, unsigned &size);
#endif

    struct Client {
        enum class Version {
            Unknown = 0,
            Alpha = 1,
            R100 = 100,
        };

        Version version = Version::Unknown;
        QString id;

        QByteArray makeHashReply(QString reply);
        QByteArray makeHashReply(const QList<QPair<QString,QString>>& reply);
        QByteArray makeEmptyListReply();
        QByteArray makeErrorReply(QString type, QString msg);
        QByteArray makeOkReply();
        QByteArray makeBinaryReply(const QByteArray &data);

        QByteArray cmdLoadCore(QString core);
        QByteArray cmdCoresList(QString platform);
        QByteArray cmdCoreInfo(QString core);
        QByteArray cmdCoreReset();
        QByteArray cmdCoreMemories();
        QByteArray cmdCoreRead(QString memory, QList< QPair<int,int> > &regions);
        QByteArray cmdCoreWrite(QString memory, QList< QPair<int,int> > &regions, QByteArray data);
        QByteArray cmdEmulatorInfo();
        QByteArray cmdEmulationStatus();
        QByteArray cmdEmulationReset();
        QByteArray cmdEmulationStop();
        QByteArray cmdEmulationPause();
        QByteArray cmdEmulationResume();
        QByteArray cmdEmulationReload();
        QByteArray cmdLoadGame(QString filename);
        QByteArray cmdGameInfo();
        QByteArray cmdMyNameIs(QString name);
#if defined(DEBUGGER)
        QByteArray cmdDebugBreak();
        QByteArray cmdDebugContinue();
#endif
        QByteArray cmdWasmReset(QByteArray args);
        QByteArray cmdWasmLoad(QByteArray args, QByteArray data);
        QByteArray cmdWasmUnload(QByteArray args);
        QByteArray cmdWasmMsgEnqueue(QByteArray args, QByteArray data);
    };

public slots:
    void newConnection();
    void clientDisconnected();
    void clientDataReady();
};

extern NWAccess *nwaccess;
