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
#include <memory>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#endif
#ifndef THINKERCAD_PUBLISHER_SHA256
#define THINKERCAD_PUBLISHER_SHA256 ""
#endif
namespace {
QNetworkRequest request(const QString& url) {
    QNetworkRequest r{QUrl(url)};
    r.setHeader(QNetworkRequest::UserAgentHeader,"ThinkerCad/" THINKERCAD_VERSION);
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
    emit message("Automatische Installation ist nur unter Windows verfügbar."); return;
#endif
    if(QString(THINKERCAD_PUBLISHER_SHA256).size()!=64) {
        emit message("Automatische Updates sind in diesem Entwicklungsbuild deaktiviert: kein Herausgeberzertifikat hinterlegt."); return;
    }
    busy_=true;
    auto* reply=network_.get(request("https://api.github.com/repos/Lyvt-Dev/ThinkerCad/releases/latest"));
    connect(reply,&QNetworkReply::readyRead,reply,[reply]{ if(reply->bytesAvailable()>1048576) reply->abort(); });
    connect(reply,&QNetworkReply::finished,this,[this,reply]{
        reply->deleteLater();
        const auto bytes=reply->readAll();
        if(reply->error()!=QNetworkReply::NoError || !allowedResponse(reply) || bytes.size()>1048576) {busy_=false;emit message("Update-Abfrage fehlgeschlagen (Netzwerk oder noch kein Release).");return;}
        const auto release=QJsonDocument::fromJson(bytes).object();
        const auto match=QRegularExpression("^v?(\\d+\\.\\d+\\.\\d+)$").match(release["tag_name"].toString());
        if(!match.hasMatch() || release["prerelease"].toBool() || release["draft"].toBool()) {busy_=false;emit message("Kein gültiges stabiles Release gefunden.");return;}
        const auto version=match.captured(1);
        if(QVersionNumber::fromString(version)<=QVersionNumber::fromString(THINKERCAD_VERSION)) {busy_=false;emit message("ThinkerCad ist aktuell.");return;}
        const auto filename="ThinkerCad-Setup-"+version+".exe";
        QString installerUrl, checksumUrl;
        for(const auto& value:release["assets"].toArray()) {
            auto asset=value.toObject(); const auto name=asset["name"].toString(); const auto url=asset["browser_download_url"].toString();
            if(!url.startsWith("https://github.com/Lyvt-Dev/ThinkerCad/releases/download/")) continue;
            if(name==filename && asset["size"].toDouble()>0 && asset["size"].toDouble()<512*1024*1024) installerUrl=url;
            if(name==filename+".sha256") checksumUrl=url;
        }
        if(installerUrl.isEmpty() || checksumUrl.isEmpty()) {busy_=false;emit message("Release enthält keinen passenden Windows-Installer mit Prüfsumme.");return;}
        emit message("Neue Version "+version+" wird im Hintergrund heruntergeladen.");
        fetchChecksum(checksumUrl,installerUrl,filename,version);
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
        download(installerUrl,filename,version,QByteArray::fromHex(match.captured(1).toLatin1()));
    });
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
        if(!verifyPublisher(file->fileName())) {QFile::remove(file->fileName());emit message("Update verworfen: Herausgebersignatur stimmt nicht oder ist nicht vertrauenswürdig.");return;}
        installer_=file->fileName(); emit updateReady(version);
    });
}
bool Updater::verifyPublisher(const QString& path) {
#ifdef Q_OS_WIN
    const auto native=QDir::toNativeSeparators(path).toStdWString();
    WINTRUST_FILE_INFO file{}; file.cbStruct=sizeof(file); file.pcwszFilePath=native.c_str();
    WINTRUST_DATA data{}; data.cbStruct=sizeof(data); data.dwUIChoice=WTD_UI_NONE;
    data.fdwRevocationChecks=WTD_REVOKE_WHOLECHAIN; data.dwUnionChoice=WTD_CHOICE_FILE; data.pFile=&file; data.dwStateAction=WTD_STATEACTION_VERIFY;
    GUID policy=WINTRUST_ACTION_GENERIC_VERIFY_V2;
    const LONG status=WinVerifyTrust(nullptr,&policy,&data);
    bool valid=false;
    if(status==ERROR_SUCCESS) {
        auto* provider=WTHelperProvDataFromStateData(data.hWVTStateData);
        auto* signer=provider?WTHelperGetProvSignerFromChain(provider,0,FALSE,0):nullptr;
        if(signer && signer->csCertChain>0) {
            BYTE hash[32]{}; DWORD size=sizeof(hash);
            if(CertGetCertificateContextProperty(signer->pasCertChain[0].pCert,CERT_SHA256_HASH_PROP_ID,hash,&size) && size==32)
                valid=QByteArray(reinterpret_cast<const char*>(hash),32).toHex()==QByteArray(THINKERCAD_PUBLISHER_SHA256).toLower();
        }
    }
    data.dwStateAction=WTD_STATEACTION_CLOSE; WinVerifyTrust(nullptr,&policy,&data);
    return valid;
#else
    Q_UNUSED(path); return false;
#endif
}
bool Updater::restartAndInstall() {
    if(!ready() || !verifyPublisher(installer_)) {emit message("Update ist nicht mehr gültig.");return false;}
#ifdef Q_OS_WIN
    const auto helper=QFileInfo(installer_).absolutePath()+"/ThinkerCadUpdater-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".exe";
    if(!QFile::copy(QCoreApplication::applicationDirPath()+"/ThinkerCadUpdater.exe",helper)) {emit message("Update-Helfer konnte nicht bereitgestellt werden.");return false;}
    if(!QProcess::startDetached(helper,{QString::number(QCoreApplication::applicationPid()),installer_,QCoreApplication::applicationDirPath()})) {QFile::remove(helper);emit message("Update-Helfer konnte nicht gestartet werden.");return false;}
    return true;
#else
    return false;
#endif
}
