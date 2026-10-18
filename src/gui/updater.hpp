#pragma once
#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QJsonObject>
#include <functional>
#include "updatedownload.hpp"
class UpdateDialog;
class QNetworkReply;
class Updater final : public QObject {
    Q_OBJECT
  public:
    explicit Updater(QObject *parent = nullptr);
    ~Updater() override;
    void check();
    void showDialog();
    void beginDownload();
    void skipVersion();
    void pauseDownload() {
        download_.pause();
    }
    void resumeDownload() {
        download_.resume();
    }
    void cancelDownload(bool keep);
    bool ready() const {
        return !installer_.isEmpty();
    }
    bool restartAndInstall();
    QString version() const {
        return release_["tag_name"].toString();
    }
    QString notes() const {
        return release_["body"].toString();
    }
    QString date() const {
        return release_["published_at"].toString();
    }
    QString status() const {
        return status_;
    }
    QString details() const {
        return details_;
    }
    QString logPath() const {
        return log_;
    }
    qint64 size() const {
        return size_;
    }
    bool available() const {
        return available_;
    }
    bool packageManaged() const {
        return packageManaged_;
    }
    bool busy() const {
        return busy_;
    }
    bool hashValid() const {
        return hashValid_;
    }
    bool hashFailed() const {
        return hashFailed_;
    }
    bool attestationFailed() const {
        return attestationFailed_;
    }
    bool attestationValid() const {
        return ready();
    }
    UpdateDownload::State downloadState() const {
        return download_.state();
    }
  signals:
    void message(const QString &message);
    void updateReady(const QString &version);
    void changed();
    void transferProgress(qint64, qint64, double, qint64);
    void installRequested();

  private:
    QNetworkAccessManager network_;
    UpdateDownload download_;
    QPointer<UpdateDialog> dialog_;
    QJsonObject release_;
    QString installer_, expectedCommit_, appImagePath_, url_, checksumUrl_, attestationUrl_, filename_,
        directory_, status_, details_, log_;
    QByteArray verifiedHash_;
    qint64 size_ = 0;
    bool hashFailed_ = false, attestationFailed_ = false;
    quint64 generation_ = 0;
    bool busy_ = false, packageManaged_ = false, hashValid_ = false, available_ = false;
    void performCheck(bool manual);
    void fetch(const QString &, qint64, std::function<void(QByteArray)>, bool sha = false, int retry = 0);
    void verifyDownload(const QString &path, const QString &version);
    void report(const QString &, const QString &details = {});
    void failure(const QString &, const QString &);
};
