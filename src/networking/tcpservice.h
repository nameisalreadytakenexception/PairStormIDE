#ifndef TCPSERVICE_H
#define TCPSERVICE_H

#include <memory>
#include <unordered_map>
#include <QString>
#include <QObject>
#include <QTcpServer>
#include <QtNetwork>
#include <QtCore>
#include <QNetworkSession>
#include "networkbase.h"

// ==========================================================================================
//                                                                                   SNGLETON
//                                                                       TCP service provider
// ==========================================================================================
class TcpService : public QObject
{
    Q_OBJECT // for signals and slots


    // Network session if needed
    // Composition of the TCP server
    // Set of the connected TCP
    //QNetworkSession*                     m_netSession = nullptr;
    std::unique_ptr<QNetworkSession>     m_netSession;
    std::unique_ptr<QTcpServer>          m_tcpServerPtr;
    QVector<std::shared_ptr<QTcpSocket>> m_clientSocketPtrs;




    QVector<std::shared_ptr<QTcpSocket>> m_serverSocketPtrs;




    // Standard port number for TCP communication
    // Received segments using the port m_portNumber
    const PortNumType m_portNumber = g_defaultTcpPortNumber;
    Segment m_pendingSegment;

    QString m_serverName;

    std::unordered_map<std::string, QString> m_ipToServerName;

    std::unordered_map<std::string, std::shared_ptr<QTcpSocket>> m_nameToSocket;

    explicit TcpService(QObject *qObject = nullptr);

public:

    TcpService(TcpService const&) = delete;
    TcpService& operator=(TcpService const&) = delete;

    // Service getter
    static std::shared_ptr<TcpService> getService();

    ServerData getServerData();

    void sendThroughSocket(const QString & data, std::shared_ptr<QTcpSocket> receiver);
    void disconnectSocket(std::shared_ptr<QTcpSocket> socket);

/*
    void addIpServerNameRelation(QHostAddress ip, QString serverName);
    QString getServerNameByIp(QHostAddress ip);
*/
    bool resolveSocketByUserName(std::shared_ptr<QTcpSocket> & userSocket, const QString & userName);


    void setServerName(const QString & name);

    void connectToTcpServer(const ServerData & serverData);//QHostAddress ip, PortNumType port);
    Segment getReceivedSegment();

    ~TcpService();

signals:
    void newSegmentSaved();

public slots:
    void configureServer();
    void processServerConnectionOnRequest();
    void processClientConnectionOnRequest();
    void informAboutStatusOnDisconnected();
    void saveSegmentOnReceival();

};

/*
#include <QDialog>
#include <QString>
#include <QVector>


QT_BEGIN_NAMESPACE
class QLabel;
class QTcpServer;
class QNetworkSession;
QT_END_NAMESPACE

class Server : public QDialog
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = nullptr);

private slots:
    void sessionOpened();
    void sendFortune();

private:
    QLabel *statusLabel = nullptr;
    QTcpServer *tcpServer = nullptr;
    QVector<QString> fortunes;
    QNetworkSession *networkSession = nullptr;
};
//! [0]
//!
*/
#endif // TCPSERVICE_H
