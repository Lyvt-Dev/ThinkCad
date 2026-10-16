#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QTimer>
#include <QElapsedTimer>
#include <functional>
#include <memory>
class LocalCollaboration final:public QObject {
    Q_OBJECT
public:
    using Validator=std::function<QString(const QJsonObject&)>;
    explicit LocalCollaboration(QObject* parent=nullptr);
    ~LocalCollaboration()override;
    void setValidator(Validator validator){validator_=std::move(validator);}
    bool host(const QJsonObject& initial,const QHostAddress& address=QHostAddress::AnyIPv4,quint16 port=0,int maximumPeers=4);
    void join(const QString& address,quint16 port,const QString& ticket);
    void stop();
    void submit(quint64 baseRevision,const QJsonObject& proposal);
    bool hosting()const{return server_.isListening();}
    bool connected()const{return hosting()||joined_;}
    quint16 port()const{return server_.serverPort();}
    QString ticket()const{return ticket_;}
    QString identityNamespace()const{return identity_;}
    quint64 revision()const{return revision_;}
    QJsonObject document()const{return current_;}
    int peerCount()const;
    static constexpr int maximumDocumentBytes=4*1024*1024;
signals:
    void stateReceived(const QJsonObject& document,quint64 revision,const QString& origin);
    void conflict(const QJsonObject& authoritative,quint64 revision,const QStringList& paths);
    void error(const QString& message);
    void connectionChanged();
private:
    struct Peer {QTcpSocket* socket=nullptr;QByteArray buffer;bool authenticated=false;QString identity;QElapsedTimer age;int messages=0;};
    QTcpServer server_;QTimer limits_;
    QMap<QTcpSocket*,std::shared_ptr<Peer>> peers_;
    QTcpSocket* upstream_=nullptr;
    Validator validator_;
    QString ticket_,identity_;
    QJsonObject current_;
    quint64 revision_=0;
    QMap<quint64,QJsonObject> history_;
    QMap<quint64,int> historyBytes_;
    int retainedBytes_=0,maximumPeers_=4;
    bool joined_=false,stopping_=false;
    std::shared_ptr<Peer> attach(QTcpSocket* socket);
    void read(const std::shared_ptr<Peer>& peer);
    void message(const std::shared_ptr<Peer>& peer,const QJsonObject& object);
    bool send(QTcpSocket* socket,const QJsonObject& object);
    QString validate(const QJsonObject& object)const;
    void apply(quint64 base,const QJsonObject& proposal,QTcpSocket* source);
    void retain();
    void broadcast(const QString& origin);
};
