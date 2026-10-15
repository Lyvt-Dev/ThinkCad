#include "librarymanager.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTemporaryDir>
#include <QTimer>
#include <QVersionNumber>
#include <iostream>
#include <stdexcept>
static int checks=0;void check(bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);}
QByteArray file(const QString& path){QFile f(path);if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("Fixture missing");return f.readAll();}
QByteArray get(const QString& url,int cap){QNetworkAccessManager network;QNetworkRequest request{QUrl(url)};request.setTransferTimeout(60000);request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);auto* reply=network.get(request);QEventLoop loop;QObject::connect(reply,&QNetworkReply::readyRead,reply,[&]{if(reply->bytesAvailable()>cap)reply->abort();});QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);QTimer::singleShot(65000,reply,&QNetworkReply::abort);loop.exec();if(reply->error()!=QNetworkReply::NoError||reply->url().scheme()!="https"||reply->url().host()!="downloads.arduino.cc")throw std::runtime_error(reply->errorString().toStdString());auto bytes=reply->readAll();if(bytes.size()>cap)throw std::runtime_error("Network response too large");return bytes;}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    if(app.arguments().contains("--live")){auto packages=ArduinoLibraries::parse(get("https://downloads.arduino.cc/libraries/library_index.json",100*1024*1024));const LibraryPackage* selected=nullptr;for(const auto& p:packages)if(p.name=="RunningAverage"&&(!selected||QVersionNumber::fromString(p.version)>QVersionNumber::fromString(selected->version)))selected=&p;check(selected!=nullptr,"live package unavailable");auto zip=get(selected->url,32*1024*1024);check(QString::fromLatin1(QCryptographicHash::hash(zip,QCryptographicHash::Sha256).toHex())==selected->sha256,"live ZIP checksum");QTemporaryDir directory;ArduinoLibraries::extract(zip,directory.path());check(QFileInfo::exists(directory.filePath("library.properties")),"live library metadata");std::cout<<"Official Arduino index: "<<packages.size()<<" package versions; verified and extracted "<<selected->name.toStdString()<<" "<<selected->version.toStdString()<<'\n';return 0;}
    const auto fixture=QString(THINKCAD_TEST_FIXTURES);for(const auto& name:{"library.zip","stored.zip"}){QTemporaryDir dir;ArduinoLibraries::extract(file(fixture+"/"+name),dir.path());check(QFileInfo::exists(dir.filePath("src/Demo.cpp")),"valid library source extraction");check(QFileInfo(dir.filePath("empty.txt")).size()==0,"empty file extraction");}
    for(const auto& name:{"traversal.zip","symlink.zip","collision.zip","oversized.zip"}){QTemporaryDir dir;bool rejected=false;try{ArduinoLibraries::extract(file(fixture+"/"+name),dir.path());}catch(const std::exception&){rejected=true;}check(rejected,"unsafe ZIP accepted");}
    auto damaged=file(fixture+"/stored.zip");int offset=damaged.indexOf("int answer(){return 42;}");check(offset>=0,"stored fixture content");damaged[offset]='X';QTemporaryDir dir;bool rejected=false;try{ArduinoLibraries::extract(damaged,dir.path());}catch(const std::exception&){rejected=true;}check(rejected,"bad CRC accepted");
    auto package=[](QString name,QString version,QJsonArray deps={}){return QJsonObject{{"name",name},{"version",version},{"url","https://downloads.arduino.cc/libraries/test.zip"},{"checksum","SHA-256:"+QString(64,'a')},{"dependencies",deps},{"architectures",QJsonArray{"*"}}};};
    QJsonArray data{package("A","1.0.0",QJsonArray{QJsonObject{{"name","Missing"}}}),package("A","2.0.0",QJsonArray{QJsonObject{{"name","B"},{"version",">=2.0.0 && <3.0.0"}}}),package("B","2.5.0"),package("B","3.0.0")};auto index=ArduinoLibraries::parse(QJsonDocument(QJsonObject{{"libraries",data}}).toJson());auto plan=ArduinoLibraries::resolve(index,"A","2.0.0");check(plan.size()==2&&plan[0].name=="B"&&plan[0].version=="2.5.0"&&plan[1].name=="A","version-constrained dependency order");rejected=false;try{ArduinoLibraries::resolve(index,"A","1.0.0");}catch(...){rejected=true;}check(rejected,"missing dependency accepted");std::cout<<checks<<" catalog, dependency and archive checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
