#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QFile>
#include <QElapsedTimer>
#include <QTimer>
#include <QCryptographicHash>
#include <QLockFile>
#include <memory>
class QNetworkReply;
// Installer bytes are never executable until both this hash and provenance pass.
class UpdateDownload final : public QObject {
    Q_OBJECT
  public:
    enum class State { Idle, Downloading, Paused, Retrying, Hashing, Ready, Cancelled, Error };
    Q_ENUM(State)
    explicit UpdateDownload(QObject *parent = nullptr, bool allowLoopbackTests = false);
    ~UpdateDownload() override;
    void start(const QUrl &, const QString &destination, qint64 size, const QByteArray &sha256);
    void pause();
    void resume();
    void cancel(bool keepPartial = false);
    State state() const {
        return state_;
    }
    QString destination() const {
        return destination_;
    }
  signals:
    void stateChanged(UpdateDownload::State);
    void progress(qint64 received, qint64 total, double bytesPerSecond, qint64 secondsRemaining);
    void failed(const QString &reason);
    void verified(const QString &path);

  private:
    QNetworkAccessManager network_;
    QPointer<QNetworkReply> reply_;
    QTimer retry_, hashTimer_;
    QElapsedTimer speedClock_;
    QFile part_, hashFile_;
    std::unique_ptr<QLockFile> lock_;
    std::unique_ptr<QCryptographicHash> hash_;
    State state_ = State::Idle;
    QUrl url_;
    QString destination_;
    QByteArray expected_, etag_;
    qint64 size_ = 0, offset_ = 0, speedBytes_ = 0;
    double currentSpeed_ = 0;
    int retries_ = 0;
    bool checkedHeaders_ = false, pauseWanted_ = false, cancelWanted_ = false, keep_ = false, tests_ = false;
    QString localError_;
    void setState(State);
    void request();
    bool headers();
    void consume();
    void finish();
    void hashFile();
    void fail(const QString &, bool discard = false);
    void journal();
    bool allowed(const QUrl &) const;
};
