#include "language.hpp"
#include "librarymanager.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkReply>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QVersionNumber>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include "ziparchive.hpp"
#include <QSet>
namespace {
QVersionNumber version(const QString& text){return QVersionNumber::fromString(text);}
bool matches(const QString& actual,QString constraint){
    constraint=constraint.trimmed();if(constraint.isEmpty()||constraint=="*")return true;
    constraint.remove('(');constraint.remove(')');
    for(const auto& alternative:constraint.split("||")){bool ok=true;for(const auto& term:alternative.split(QRegularExpression("&&|,"),Qt::SkipEmptyParts)){auto match=QRegularExpression("^\\s*(>=|<=|==|!=|>|<|=)?\\s*([0-9]+(?:\\.[0-9]+){0,2})\\s*$").match(term);if(!match.hasMatch())throw std::runtime_error(Language::text("Nicht unterstützte Versionsbedingung: ").toStdString()+constraint.toStdString());const int c=QVersionNumber::compare(version(actual).normalized(),version(match.captured(2)).normalized());auto op=match.captured(1);ok&=op==">="?c>=0:op=="<="?c<=0:op==">"?c>0:op=="<"?c<0:op=="!="?c!=0:c==0;}if(ok)return true;}return false;
}
QString slug(const QString& name){return QString::fromLatin1(QCryptographicHash::hash(name.toUtf8(),QCryptographicHash::Sha256).toHex().left(20));}
QNetworkRequest request(const QUrl& url){QNetworkRequest r(url);r.setHeader(QNetworkRequest::UserAgentHeader,"ThinkCad Arduino Library Manager");r.setTransferTimeout(60000);r.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);return r;}
bool official(QNetworkReply* reply){return reply->url().scheme()=="https"&&reply->url().host()=="downloads.arduino.cc";}
}
QList<LibraryPackage> ArduinoLibraries::parse(const QByteArray& bytes){
    if(bytes.size()>100*1024*1024)throw std::runtime_error(Language::text("Bibliotheksindex zu groß.").toStdString());QJsonParseError error;const auto json=QJsonDocument::fromJson(bytes,&error);if(error.error!=QJsonParseError::NoError||!json.object()["libraries"].isArray())throw std::runtime_error(Language::text("Ungültiger Arduino-Bibliotheksindex.").toStdString());QList<LibraryPackage> result;
    for(const auto& value:json.object()["libraries"].toArray()){auto o=value.toObject();const auto checksum=o["checksum"].toString();QUrl url(o["url"].toString());if(!QRegularExpression("^SHA-256:[0-9a-fA-F]{64}$").match(checksum).hasMatch()||url.scheme()!="https"||url.host()!="downloads.arduino.cc")continue;const auto name=o["name"].toString(),v=o["version"].toString();if(name.isEmpty()||name.size()>160||!QRegularExpression("^[0-9]+\\.[0-9]+\\.[0-9]+$").match(v).hasMatch())continue;LibraryPackage p{name,v,o["sentence"].toString()+" "+o["paragraph"].toString(),url.toString(),checksum.mid(8).toLower(),{},o["dependencies"].toArray()};for(const auto& arch:o["architectures"].toArray())p.architectures<<arch.toString();result<<p;}
    if(result.isEmpty())throw std::runtime_error(Language::text("Kein gültiges Bibliothekspaket im Index.").toStdString());return result;
}
QList<LibraryPackage> ArduinoLibraries::resolve(const QList<LibraryPackage>& index,const QString& name,const QString& wanted){
    QMap<QString,LibraryPackage> selected;QList<LibraryPackage> order;
    std::function<void(QString,QString)> visit=[&](QString n,QString condition){if(selected.contains(n)){if(!matches(selected[n].version,condition))throw std::runtime_error(Language::text("Widersprüchliche Bibliotheksabhängigkeiten: ").toStdString()+n.toStdString());return;}if(selected.size()>=100)throw std::runtime_error(Language::text("Zu viele Bibliotheksabhängigkeiten.").toStdString());const LibraryPackage* best=nullptr;for(const auto& p:index)if(p.name==n&&matches(p.version,condition)&&(!best||version(p.version)>version(best->version)))best=&p;if(!best)throw std::runtime_error(Language::text("Abhängigkeit nicht im Index gefunden: ").toStdString()+n.toStdString());selected[n]=*best;for(const auto& dep:best->dependencies){auto d=dep.toObject();if(d["name"].toString().isEmpty())throw std::runtime_error(Language::text("Ungültige Abhängigkeit.").toStdString());visit(d["name"].toString(),d["version"].toString());}order<<*best;if(order.size()>100)throw std::runtime_error(Language::text("Zu viele Bibliotheksabhängigkeiten.").toStdString());};visit(name,"="+wanted);return order;
}
void ArduinoLibraries::extract(const QByteArray& bytes,const QString& destination){
    ZipArchive::extractArduinoLibrary(bytes,destination);
}
LibraryManagerDialog::LibraryManagerDialog(QWidget* parent):QDialog(parent),network_(this){
    setWindowTitle(Language::text("Arduino-Bibliotheken"));resize(860,620);root_=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/libraries";QDir().mkpath(root_);auto* layout=new QVBoxLayout(this);auto* info=new QLabel(Language::text("Offizieller Arduino-Katalog. Installation umfasst Abhängigkeiten. Hardwarebibliotheken benötigen zusätzlich ein unterstütztes Simulationsmodell."));info->setWordWrap(true);layout->addWidget(info);search_=new QLineEdit;search_->setPlaceholderText(Language::text("Bibliotheken suchen…"));layout->addWidget(search_);list_=new QTreeWidget;list_->setHeaderLabels({Language::text("Bibliothek"),"Version",Language::text("Architekturen")});list_->setColumnWidth(0,300);list_->setColumnWidth(1,90);layout->addWidget(list_);status_=new QLabel;status_->setWordWrap(true);layout->addWidget(status_);install_=new QPushButton(Language::text("Ausgewählte Bibliothek installieren"));layout->addWidget(install_);refresh_=new QPushButton(Language::text("Katalog aktualisieren"));layout->addWidget(refresh_);auto* local=new QPushButton(Language::text("Lokalen Bibliotheksordner hinzufügen…"));layout->addWidget(local);connect(local,&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getExistingDirectory(this,Language::text("Lokale Bibliothek"));if(path.isEmpty())return;auto paths=QSettings().value("libraries").toStringList();if(!paths.contains(path))paths<<path;QSettings().setValue("libraries",paths);status_->setText(Language::text("Lokale Bibliothek hinzugefügt: ")+path);});connect(search_,&QLineEdit::textChanged,this,[this](const QString& text){for(int i=0;i<list_->topLevelItemCount();++i){auto* item=list_->topLevelItem(i);item->setHidden(!(item->text(0)+" "+item->toolTip(0)).contains(text,Qt::CaseInsensitive));}});connect(install_,&QPushButton::clicked,this,&LibraryManagerDialog::startInstall);connect(refresh_,&QPushButton::clicked,this,&LibraryManagerDialog::fetchIndex);
    QFile cached(root_+"/library_index.json");if(cached.open(QIODevice::ReadOnly))try{index_=ArduinoLibraries::parse(cached.readAll());populate();}catch(...){}if(index_.isEmpty())fetchIndex();
}
void LibraryManagerDialog::fail(const QString& message){busy_=false;install_->setEnabled(true);refresh_->setEnabled(true);status_->setText(message);staging_.reset();staged_.clear();plan_.clear();}
void LibraryManagerDialog::fetchIndex(){if(busy_)return;busy_=true;install_->setEnabled(false);refresh_->setEnabled(false);status_->setText(Language::text("Arduino-Katalog wird geladen…"));auto* reply=network_.get(request(QUrl("https://downloads.arduino.cc/libraries/library_index.json")));connect(reply,&QNetworkReply::readyRead,reply,[reply]{if(reply->bytesAvailable()>100*1024*1024)reply->abort();});connect(reply,&QNetworkReply::finished,this,[this,reply]{reply->deleteLater();if(reply->error()!=QNetworkReply::NoError||!official(reply)){fail(Language::text("Katalog konnte nicht geladen werden: ")+reply->errorString());return;}try{auto bytes=reply->readAll();index_=ArduinoLibraries::parse(bytes);QSaveFile cache(root_+"/library_index.json");if(cache.open(QIODevice::WriteOnly)&&cache.write(bytes)==bytes.size())cache.commit();busy_=false;install_->setEnabled(true);refresh_->setEnabled(true);populate();}catch(const std::exception& e){fail(QString::fromUtf8(e.what()));}});}
void LibraryManagerDialog::populate(){list_->clear();QMap<QString,LibraryPackage> latest;for(const auto& p:index_)if(!latest.contains(p.name)||version(p.version)>version(latest[p.name].version))latest[p.name]=p;for(const auto& p:latest){auto* item=new QTreeWidgetItem(list_,{p.name,p.version,p.architectures.join(", ")});item->setToolTip(0,p.description);}status_->setText(QString(Language::text("%1 Bibliotheken · %2 Paketversionen")).arg(latest.size()).arg(index_.size()));}
void LibraryManagerDialog::startInstall(){if(busy_||!list_->currentItem())return;try{plan_=ArduinoLibraries::resolve(index_,list_->currentItem()->text(0),list_->currentItem()->text(1));}catch(const std::exception& e){fail(QString::fromUtf8(e.what()));return;}busy_=true;install_->setEnabled(false);refresh_->setEnabled(false);staged_.clear();nextPackage();}
void LibraryManagerDialog::nextPackage(){
    if(plan_.isEmpty()){auto paths=QSettings().value("libraries").toStringList();for(const auto& added:staged_){const auto prefix=QFileInfo(added).absolutePath()+"/";for(int i=paths.size();i-->0;)if(paths[i].startsWith(prefix))paths.removeAt(i);paths<<added;}paths.removeDuplicates();QSettings().setValue("libraries",paths);busy_=false;install_->setEnabled(true);refresh_->setEnabled(true);status_->setText(Language::text("Bibliothek und Abhängigkeiten installiert. Beim nächsten Sketch-Build verfügbar."));return;}
    const auto package=plan_.takeFirst();const auto target=root_+"/"+slug(package.name)+"/"+package.version;if(QFileInfo::exists(target+"/library.properties")){staged_<<target;nextPackage();return;}status_->setText(Language::text("Lade ")+package.name+" "+package.version+"…");auto* reply=network_.get(request(QUrl(package.url)));connect(reply,&QNetworkReply::readyRead,reply,[reply]{if(reply->bytesAvailable()>32*1024*1024)reply->abort();});connect(reply,&QNetworkReply::finished,this,[this,reply,package,target]{reply->deleteLater();if(reply->error()!=QNetworkReply::NoError||!official(reply)){fail(Language::text("Bibliotheksdownload fehlgeschlagen: ")+reply->errorString());return;}try{auto bytes=reply->readAll();if(bytes.size()>32*1024*1024||QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())!=package.sha256)throw std::runtime_error(Language::text("Bibliotheks-Prüfsumme stimmt nicht.").toStdString());staging_=std::make_unique<QTemporaryDir>(root_+"/.install-XXXXXX");if(!staging_->isValid())throw std::runtime_error(Language::text("Temporärer Bibliotheksordner fehlt.").toStdString());ArduinoLibraries::extract(bytes,staging_->path());QDir().mkpath(QFileInfo(target).absolutePath());if(!QDir().rename(staging_->path(),target))throw std::runtime_error(Language::text("Bibliothek konnte nicht übernommen werden.").toStdString());staging_.reset();staged_<<target;nextPackage();}catch(const std::exception& e){fail(QString::fromUtf8(e.what()));}});
}
