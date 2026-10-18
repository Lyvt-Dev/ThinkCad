#include "localcollaboration.hpp"
#include "projectmerge.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QtEndian>
#include <algorithm>
namespace {
QString randomIdentity(){return QUuid::createUuid().toString(QUuid::Id128).left(20)+'_';}
bool revisionNumber(const QJsonValue& value,quint64& revision){if(!value.isDouble())return false;const auto n=value.toDouble();if(n<0||n>9007199254740990.0)return false;revision=static_cast<quint64>(n);return double(revision)==n;}
}
LocalCollaboration::LocalCollaboration(QObject* parent):QObject(parent){
    connect(&server_,&QTcpServer::newConnection,this,[this]{while(server_.hasPendingConnections()){auto* socket=server_.nextPendingConnection();if(peers_.size()>=maximumPeers_+4){socket->abort();socket->deleteLater();continue;}attach(socket);}});
    limits_.setInterval(1000);connect(&limits_,&QTimer::timeout,this,[this]{const auto peers=peers_;for(const auto& peer:peers){if((!peer->authenticated&&peer->age.elapsed()>5000)||(!peer->buffer.isEmpty()&&peer->age.elapsed()>30000))peer->socket->abort();peer->messages=0;}});
}
LocalCollaboration::~LocalCollaboration(){stop();}
int LocalCollaboration::peerCount()const{int count=0;for(const auto& peer:peers_)if(peer->authenticated)++count;return count;}
QString LocalCollaboration::validate(const QJsonObject& object)const{if(QJsonDocument(object).toJson(QJsonDocument::Compact).size()>maximumDocumentBytes)return QStringLiteral("LAN document exceeds 4 MiB");return validator_?validator_(object):QString();}
bool LocalCollaboration::host(const QJsonObject& initial,const QHostAddress& address,quint16 requestedPort,int maximumPeers){
    stop();const auto failure=validate(initial);if(!failure.isEmpty()){emit error(failure);return false;}maximumPeers_=std::clamp(maximumPeers,1,8);current_=initial;revision_=0;identity_=randomIdentity();ticket_=QUuid::createUuid().toString(QUuid::Id128)+QUuid::createUuid().toString(QUuid::Id128);
    server_.setMaxPendingConnections(maximumPeers_+4);if(!server_.listen(address,requestedPort)){emit error(server_.errorString());return false;}retain();limits_.start();emit connectionChanged();return true;
}
void LocalCollaboration::join(const QString& address,quint16 port,const QString& ticket){
    stop();if(address.trimmed().isEmpty()||!port||ticket.size()!=64){emit error(QStringLiteral("Invalid LAN address, port or session ticket"));return;}ticket_=ticket;auto* socket=new QTcpSocket(this);upstream_=socket;attach(socket);
    connect(socket,&QTcpSocket::connected,this,[this,socket]{send(socket,{{"type","hello"},{"protocol",1},{"ticket",ticket_}});});socket->connectToHost(address,port);limits_.start();
}
void LocalCollaboration::stop(){
    stopping_=true;limits_.stop();server_.close();const auto peers=peers_;peers_.clear();for(const auto& peer:peers){disconnect(peer->socket,nullptr,this,nullptr);peer->socket->abort();peer->socket->deleteLater();}upstream_=nullptr;joined_=false;history_.clear();historyBytes_.clear();retainedBytes_=0;current_={};revision_=0;ticket_.clear();identity_.clear();stopping_=false;emit connectionChanged();
}
std::shared_ptr<LocalCollaboration::Peer> LocalCollaboration::attach(QTcpSocket* socket){
    auto peer=std::make_shared<Peer>();peer->socket=socket;peer->age.start();socket->setReadBufferSize(maximumDocumentBytes+8192);peers_[socket]=peer;
    connect(socket,&QTcpSocket::readyRead,this,[this,peer]{read(peer);});
    connect(socket,&QTcpSocket::errorOccurred,this,[this,socket](QAbstractSocket::SocketError){if(socket==upstream_&&!stopping_)emit error(socket->errorString());});
    connect(socket,&QTcpSocket::disconnected,this,[this,socket]{peers_.remove(socket);if(upstream_==socket){upstream_=nullptr;joined_=false;limits_.stop();}socket->deleteLater();if(!stopping_)emit connectionChanged();});return peer;
}
bool LocalCollaboration::send(QTcpSocket* socket,const QJsonObject& object){
    const auto data=QJsonDocument(object).toJson(QJsonDocument::Compact);if(data.size()>maximumDocumentBytes+4096||socket->bytesToWrite()>2*maximumDocumentBytes){socket->abort();return false;}QByteArray frame(4,'\0');qToBigEndian<quint32>(quint32(data.size()),frame.data());frame+=data;return socket->write(frame)==frame.size();
}
void LocalCollaboration::read(const std::shared_ptr<Peer>& peer){
    const auto received=peer->socket->readAll();if(peer->authenticated&&peer->buffer.isEmpty()&&!received.isEmpty())peer->age.restart();peer->buffer+=received;if(peer->buffer.size()>maximumDocumentBytes+8192){peer->socket->abort();return;}
    int dispatched=0;
    while(peer->buffer.size()>=4){const auto size=qFromBigEndian<quint32>(peer->buffer.constData());if(size>quint32(maximumDocumentBytes+4096)||(hosting()&&!peer->authenticated&&size>4096)||!size){peer->socket->abort();return;}if(peer->buffer.size()<qsizetype(size)+4)return;
        if(++peer->messages>60){peer->socket->abort();return;}const auto data=peer->buffer.mid(4,size);peer->buffer.remove(0,size+4);peer->age.restart();QJsonParseError parse;const auto json=QJsonDocument::fromJson(data,&parse);if(parse.error!=QJsonParseError::NoError||!json.isObject()){peer->socket->abort();return;}message(peer,json.object());if(!peers_.contains(peer->socket))return;
        if(++dispatched>=16){QTimer::singleShot(0,this,[this,peer]{if(peers_.contains(peer->socket))read(peer);});return;}
    }
}
void LocalCollaboration::message(const std::shared_ptr<Peer>& peer,const QJsonObject& object){
    const auto type=object["type"].toString();
    if(hosting()){
        if(!peer->authenticated){if(type!="hello"||object["protocol"].toInt()!=1||object["ticket"].toString()!=ticket_||peerCount()>=maximumPeers_){peer->socket->abort();return;}peer->authenticated=true;peer->identity=randomIdentity();send(peer->socket,{{"type","welcome"},{"protocol",1},{"identity",peer->identity},{"revision",double(revision_)},{"document",current_}});emit connectionChanged();return;}
        quint64 base=0;if(type!="change"||!revisionNumber(object["base"],base)||!object["document"].isObject()){peer->socket->abort();return;}apply(base,object["document"].toObject(),peer->socket);
    }else if(peer->socket==upstream_){
        quint64 version=0;if(!revisionNumber(object["revision"],version)||!object["document"].isObject()){peer->socket->abort();return;}
        if(!peer->authenticated){const auto id=object["identity"].toString();if(type!="welcome"||object["protocol"].toInt()!=1||id.size()!=21||!id.endsWith('_')){peer->socket->abort();return;}identity_=id;peer->authenticated=true;joined_=true;emit connectionChanged();}
        else if(type!="state"&&type!="conflict"){peer->socket->abort();return;}
        if(version<revision_){peer->socket->abort();return;}const auto doc=object["document"].toObject();const auto failure=validate(doc);if(!failure.isEmpty()){emit error(failure);peer->socket->abort();return;}current_=doc;revision_=version;
        if(type=="conflict"){QStringList paths;for(const auto& value:object["paths"].toArray()){if(paths.size()>=256)break;paths.append(value.toString().left(512));}emit conflict(current_,revision_,paths);}else emit stateReceived(current_,revision_,object["origin"].toString());
    }
}
void LocalCollaboration::submit(quint64 base,const QJsonObject& proposal){if(hosting())apply(base,proposal,nullptr);else if(joined_&&upstream_)send(upstream_,{{"type","change"},{"base",double(base)},{"document",proposal}});}
void LocalCollaboration::apply(quint64 base,const QJsonObject& proposal,QTcpSocket* source){
    QStringList conflicts;QJsonObject merged;
    if(!history_.contains(base))conflicts.append(QStringLiteral("/revision: base is no longer retained"));
    else {const auto result=ProjectMerge::merge(history_.value(base),current_,proposal);merged=result.document;conflicts=result.conflicts;if(conflicts.isEmpty()){const auto failure=validate(merged);if(!failure.isEmpty())conflicts.append(failure);}}
    if(!conflicts.isEmpty()){if(source){QJsonArray paths;for(const auto& path:conflicts)paths.append(path);send(source,{{"type","conflict"},{"revision",double(revision_)},{"document",current_},{"paths",paths}});}else emit conflict(current_,revision_,conflicts);return;}
    if(merged==current_){if(source)send(source,{{"type","state"},{"revision",double(revision_)},{"document",current_},{"origin",peers_.value(source)->identity}});else emit stateReceived(current_,revision_,identity_);return;}
    if(revision_>=9007199254740990ULL){emit error(QStringLiteral("LAN revision limit reached; restart the session"));return;}current_=merged;++revision_;retain();const auto origin=source?peers_.value(source)->identity:identity_;broadcast(origin);emit stateReceived(current_,revision_,origin);
}
void LocalCollaboration::retain(){
    const int size=QJsonDocument(current_).toJson(QJsonDocument::Compact).size();history_[revision_]=current_;historyBytes_[revision_]=size;retainedBytes_+=size;
    while(history_.size()>32||(retainedBytes_>16*1024*1024&&history_.size()>1)){const auto first=history_.firstKey();retainedBytes_-=historyBytes_.take(first);history_.remove(first);}
}
void LocalCollaboration::broadcast(const QString& origin){const auto peers=peers_;for(const auto& peer:peers)if(peer->authenticated)send(peer->socket,{{"type","state"},{"revision",double(revision_)},{"document",current_},{"origin",origin}});}
