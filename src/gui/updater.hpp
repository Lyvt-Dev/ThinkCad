#pragma once
#include <QObject>
#include <QNetworkAccessManager>
class Updater final : public QObject {
    Q_OBJECT
public:
    explicit Updater(QObject* parent=nullptr);
    void check();
    bool ready() const { return !installer_.isEmpty(); }
    bool restartAndInstall();
signals:
    void message(const QString& message);
    void updateReady(const QString& version);
private:
    QNetworkAccessManager network_;
    QString installer_,expectedCommit_;
    bool busy_=false;
    void fetchChecksum(const QString& url,const QString& installerUrl,const QString& filename,const QString& version);
    void download(const QString& url,const QString& filename,const QString& version,const QByteArray& hash);
    void verifyDownload(const QString& path,const QString& version);
    void fetchAttestation(const QString& url,const QString& installerUrl,const QString& filename,const QString& version,const QByteArray& expected);
};
