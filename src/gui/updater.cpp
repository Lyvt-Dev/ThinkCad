#include "updater.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVersionNumber>
#include <QUuid>
#include <QTimer>
#include <memory>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#include "../../packaging/windows/verify_attestation.hpp"
#endif
#ifndef THINKERCAD_PUBLISHER_SHA256
#define THINKERCAD_PUBLISHER_SHA256 ""
#endif
namespace {
QNetworkRequest request(const QString& url) {
    QNetworkRequest r{QUrl(url)};
    r.setHeader(QNetworkRequest::UserAgentHeader,"ThinkCad/" THINKERCAD_VERSION);
    r.setTransferTimeout(30000);
    r.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);
    return r;
}
bool allowedResponse(QNetworkReply* reply) {
    const auto url=reply->url();
    return url.scheme()=="https" && (url.host()=="api.github.com" || url.host()=="github.com" || url.host()=="release-assets.githubusercontent.com" || url.host()=="objects.githubusercontent.com");
}
}
Updater::Updater(QObject* parent) : QObject(parent), network_(this) {}
void Updater::check() {
    if(busy_ || ready()) return;
#ifndef Q_OS_WIN
    if(busy_)return;
    busy_=true;auto* linuxReply=network_.get(request("https://api.github.com/repos/Lyvt-Dev/ThinkCad/releases/latest"));
    connect(linuxReply,&QNetworkReply::readyRead,linuxReply,[linuxReply]{if(linuxReply->bytesAvailable()>1048576)linuxReply->abort();});
    connect(linuxReply,&QNetworkReply::finished,this,[this,linuxReply]{busy_=false;linuxReply->deleteLater();const auto bytes=linuxReply->readAll();const auto release=QJsonDocument::fromJson(bytes).object();const auto version=QRegularExpression("^v([0-9]+\\.[0-9]+\\.[0-9]+)$").match(release["tag_name"].toString());if(linuxReply->error()!=QNetworkReply::NoError||!allowedResponse(linuxReply)||bytes.size()>1048576||!version.hasMatch()){emit message("Update-Abfrage fehlgeschlagen.");return;}if(release["draft"].toBool()||release["prerelease"].toBool())return;if(QVersionNumber::fromString(version.captured(1))>QVersionNumber::fromString(THINKERCAD_VERSION))emit message("ThinkCad "+version.captured(1)+" ist verfügbar. Linux: über die Paketverwaltung aktualisieren; Gentoo: Overlay aktualisieren und emerge --ask --oneshot media-gfx/thinkcad. Downloads: https://github.com/Lyvt-Dev/ThinkCad/releases/latest");else emit message("ThinkCad ist aktuell.");});return;
#endif
    busy_=true;
    auto* reply=network_.get(request("https://api.github.com/repos/Lyvt-Dev/ThinkCad/releases/latest"));
    connect(reply,&QNetworkReply::readyRead,reply,[reply]{ if(reply->bytesAvailable()>1048576) reply->abort(); });
    connect(reply,&QNetworkReply::finished,this,[this,reply]{
        reply->deleteLater();
        const auto bytes=reply->readAll();
        if(reply->error()!=QNetworkReply::NoError || !allowedResponse(reply) || bytes.size()>1048576) {busy_=false;emit message("Update-Abfrage fehlgeschlagen (Netzwerk oder noch kein Release).");return;}
        const auto release=QJsonDocument::fromJson(bytes).object();
        const auto match=QRegularExpression("^v?(\\d+\\.\\d+\\.\\d+)$").match(release["tag_name"].toString());
        if(!match.hasMatch() || release["prerelease"].toBool() || release["draft"].toBool()) {busy_=false;emit message("Kein gültiges stabiles Release gefunden.");return;}
        const auto version=match.captured(1);
        if(QVersionNumber::fromString(version)<=QVersionNumber::fromString(THINKERCAD_VERSION)) {busy_=false;emit message("ThinkCad ist aktuell.");return;}
        const auto filename="ThinkCad-Setup-"+version+".exe";
        QString installerUrl,checksumUrl,attestationUrl;
        for(const auto& value:release["assets"].toArray()) {
            auto asset=value.toObject(); const auto name=asset["name"].toString(); const auto url=asset["browser_download_url"].toString();
            if(!url.startsWith("https://github.com/Lyvt-Dev/ThinkCad/releases/download/")) continue;
            if(name==filename && asset["size"].toDouble()>0 && asset["size"].toDouble()<512*1024*1024) installerUrl=url;
            if(name==filename+".sha256")checksumUrl=url;if(name==filename+".sigstore.jsonl"&&asset["size"].toDouble()<5*1024*1024)attestationUrl=url;
        }
        if(installerUrl.isEmpty()||checksumUrl.isEmpty()||attestationUrl.isEmpty()) {busy_=false;emit message("Release enthält keinen Installer mit Prüfsumme und Buildnachweis.");return;}
        emit message("Neue Version "+version+" wird im Hintergrund heruntergeladen.");
        auto* commit=network_.get(request("https://api.github.com/repos/Lyvt-Dev/ThinkCad/commits/v"+version));
        connect(commit,&QNetworkReply::readyRead,commit,[commit]{if(commit->bytesAvailable()>1048576)commit->abort();});
        connect(commit,&QNetworkReply::finished,this,[this,commit,checksumUrl,installerUrl,filename,version,attestationUrl]{commit->deleteLater();const auto bytes=commit->readAll();expectedCommit_=QJsonDocument::fromJson(bytes).object()["sha"].toString();if(commit->error()!=QNetworkReply::NoError||!allowedResponse(commit)||bytes.size()>1048576||!QRegularExpression("^[a-f0-9]{40}$").match(expectedCommit_).hasMatch()){busy_=false;emit message("Release-Commit konnte nicht geprüft werden.");return;}Q_UNUSED(attestationUrl);fetchChecksum(checksumUrl,installerUrl,filename,version);});
    });
}
void Updater::fetchChecksum(const QString& url,const QString& installerUrl,const QString& filename,const QString& version) {
    auto* reply=network_.get(request(url));
    connect(reply,&QNetworkReply::readyRead,reply,[reply]{if(reply->bytesAvailable()>4096)reply->abort();});
    connect(reply,&QNetworkReply::finished,this,[this,reply,installerUrl,filename,version]{
        reply->deleteLater(); const auto content=reply->readAll();
        auto expression=QRegularExpression("^([a-fA-F0-9]{64})[ \\t]+\\*?"+QRegularExpression::escape(filename)+"(?:\\r?\\n)?$");
        auto match=expression.match(QString::fromLatin1(content));
        if(reply->error()!=QNetworkReply::NoError || !allowedResponse(reply) || content.size()>4096 || !match.hasMatch()) {busy_=false;emit message("Ungültige Update-Prüfsumme.");return;}
        fetchAttestation(installerUrl+".sigstore.jsonl",installerUrl,filename,version,QByteArray::fromHex(match.captured(1).toLatin1()));
    });
}
void Updater::fetchAttestation(const QString& url,const QString& installerUrl,const QString& filename,const QString& version,const QByteArray& expected){
    auto* reply=network_.get(request(url));connect(reply,&QNetworkReply::readyRead,reply,[reply]{if(reply->bytesAvailable()>5*1024*1024)reply->abort();});connect(reply,&QNetworkReply::finished,this,[this,reply,installerUrl,filename,version,expected]{reply->deleteLater();const auto bytes=reply->readAll();if(reply->error()!=QNetworkReply::NoError||!allowedResponse(reply)||bytes.isEmpty()||bytes.size()>5*1024*1024){busy_=false;emit message("Buildnachweis konnte nicht geladen werden.");return;}const auto dir=QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/updates";QDir().mkpath(dir);QSaveFile file(dir+"/"+filename+".sigstore.jsonl");if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){busy_=false;emit message("Buildnachweis konnte nicht gespeichert werden.");return;}download(installerUrl,filename,version,expected);});
}
void Updater::download(const QString& url,const QString& filename,const QString& version,const QByteArray& expected) {
    const auto dir=QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/updates";
    if(!QDir().mkpath(dir)){busy_=false;emit message("Update-Verzeichnis ist nicht beschreibbar.");return;}
    auto file=std::make_shared<QSaveFile>(dir+"/"+filename);
    if(!file->open(QIODevice::WriteOnly)){busy_=false;emit message("Update-Datei kann nicht angelegt werden.");return;}
    struct State {QCryptographicHash hash{QCryptographicHash::Sha256}; qint64 bytes=0; bool failed=false;};
    auto state=std::make_shared<State>(); auto* reply=network_.get(request(url));
    auto consume=[reply,file,state] {
        const auto chunk=reply->readAll(); state->bytes+=chunk.size();
        if(state->bytes>512*1024*1024 || file->write(chunk)!=chunk.size()) {state->failed=true;reply->abort();return;}
        state->hash.addData(chunk);
    };
    connect(reply,&QNetworkReply::readyRead,this,consume);
    connect(reply,&QNetworkReply::finished,this,[this,reply,file,state,consume,expected,version] {
        consume(); reply->deleteLater(); busy_=false;
        if(state->failed || !state->bytes || reply->error()!=QNetworkReply::NoError || !allowedResponse(reply) || state->hash.result()!=expected || !file->commit()) {
            file->cancelWriting(); emit message("Update verworfen: Download oder SHA-256-Prüfung fehlgeschlagen.");return;
        }
        verifyDownload(file->fileName(),version);
    });
}
void Updater::verifyDownload(const QString& path,const QString& version){
#ifdef Q_OS_WIN
    const auto verifier=QCoreApplication::applicationDirPath()+"/verifier/gh.exe",roots=QCoreApplication::applicationDirPath()+"/verifier/trusted_root.jsonl";if(!QFileInfo::exists(verifier)||!QFileInfo::exists(roots)){QFile::remove(path);emit message("Update-Verifizierer fehlt in dieser Installation.");return;}
    busy_=true;auto* check=new QProcess(this);auto finished=std::make_shared<bool>(false);auto complete=[this,check,path,version,finished](bool ok){if(*finished)return;*finished=true;busy_=false;check->deleteLater();if(!ok){QFile::remove(path);emit message("Update verworfen: GitHub-Buildnachweis oder Release-Commit ungültig.");return;}installer_=path;emit updateReady(version);};
    connect(check,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[complete](int code,QProcess::ExitStatus status){complete(code==0&&status==QProcess::NormalExit);});connect(check,&QProcess::errorOccurred,this,[complete](QProcess::ProcessError error){if(error==QProcess::FailedToStart)complete(false);});QTimer::singleShot(60000,check,[check]{if(check->state()!=QProcess::NotRunning)check->kill();});
    check->start(verifier,{"attestation","verify",path,"--bundle",path+".sigstore.jsonl","--custom-trusted-root",roots,"--repo","Lyvt-Dev/ThinkCad","--signer-workflow","Lyvt-Dev/ThinkCad/.github/workflows/windows.yml","--source-ref","refs/heads/main","--source-digest",expectedCommit_,"--deny-self-hosted-runners"});
#else
    Q_UNUSED(path);Q_UNUSED(version);
#endif
}
bool Updater::restartAndInstall() {
    if(!ready()||!QFileInfo::exists(installer_)) {emit message("Update ist nicht mehr gültig.");return false;}
#ifdef Q_OS_WIN
    const auto cache=QFileInfo(installer_).absolutePath();QDir().mkpath(cache+"/verifier");for(const auto& name:QStringList{"gh.exe","trusted_root.jsonl"}){QFile source(QCoreApplication::applicationDirPath()+"/verifier/"+name);QSaveFile target(cache+"/verifier/"+name);if(!source.open(QIODevice::ReadOnly)||!target.open(QIODevice::WriteOnly)){emit message("Update-Verifizierer fehlt.");return false;}const auto bytes=source.readAll();if(target.write(bytes)!=bytes.size()||!target.commit())return false;}
    const auto helper=cache+"/ThinkCadUpdater-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".exe";
    if(!QFile::copy(QCoreApplication::applicationDirPath()+"/ThinkCadUpdater.exe",helper)) {emit message("Update-Helfer konnte nicht bereitgestellt werden.");return false;}
    if(!QProcess::startDetached(helper,{QString::number(QCoreApplication::applicationPid()),installer_,QCoreApplication::applicationDirPath(),expectedCommit_})) {QFile::remove(helper);emit message("Update-Helfer konnte nicht gestartet werden.");return false;}
    return true;
#else
    return false;
#endif
}
