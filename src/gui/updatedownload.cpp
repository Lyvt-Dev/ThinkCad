#include "updatedownload.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QRegularExpression>
UpdateDownload::UpdateDownload(QObject *p, bool tests) : QObject(p), network_(this), tests_(tests) {
    retry_.setSingleShot(true);
    connect(&retry_, &QTimer::timeout, this, &UpdateDownload::request);
    hashTimer_.setInterval(0);
    connect(&hashTimer_, &QTimer::timeout, this, [this] {
        for (int i = 0; i < 16 && !hashFile_.atEnd(); ++i) {
            auto bytes = hashFile_.read(65536);
            if (bytes.isEmpty() && hashFile_.error() != QFileDevice::NoError) {
                fail(hashFile_.errorString());
                return;
            }
            hash_->addData(bytes);
        }
        if (!hashFile_.atEnd())
            return;
        hashTimer_.stop();
        hashFile_.close();
        if (hash_->result() != expected_) {
            fail("SHA-256", true);
            return;
        }
        if (!QFileInfo::exists(destination_) && !QFile::rename(destination_ + ".part", destination_)) {
            fail("rename: " + destination_);
            return;
        }
        QFile::remove(destination_ + ".json");
        setState(State::Ready);
        emit verified(destination_);
    });
}
UpdateDownload::~UpdateDownload() {
    if (reply_) {
        disconnect(reply_, nullptr, this, nullptr);
        reply_->abort();
    }
    retry_.stop();
    hashTimer_.stop();
    part_.close();
    hashFile_.close();
}
bool UpdateDownload::allowed(const QUrl &u) const {
    if (tests_ && u.scheme() == "http" && (u.host() == "127.0.0.1" || u.host() == "localhost"))
        return true;
    return u.scheme() == "https" &&
           (u.host() == "github.com" || u.host() == "release-assets.githubusercontent.com" ||
            u.host() == "objects.githubusercontent.com");
}
void UpdateDownload::setState(State s) {
    state_ = s;
    emit stateChanged(s);
}
void UpdateDownload::start(const QUrl &url, const QString &destination, qint64 size,
                           const QByteArray &expected) {
    if (reply_ || state_ == State::Hashing)
        return;
    retry_.stop();
    part_.close();
    lock_.reset();
    url_ = url;
    destination_ = destination;
    size_ = size;
    expected_ = expected;
    retries_ = 0;
    etag_.clear();
    pauseWanted_ = cancelWanted_ = false;
    localError_.clear();
    if (!allowed(url) || size <= 0 || size > 512ll * 1024 * 1024 || expected.size() != 32) {
        fail("metadata");
        return;
    }
    if (QFileInfo(destination).isSymLink() || QFileInfo(destination + ".part").isSymLink() ||
        !QDir().mkpath(QFileInfo(destination).absolutePath())) {
        fail("directory: " + QFileInfo(destination).absolutePath());
        return;
    }
    lock_ = std::make_unique<QLockFile>(destination + ".lock");
    lock_->setStaleLockTime(0);
    if (!lock_->tryLock(0)) {
        fail("locked: " + destination);
        return;
    }
    if (QFileInfo(destination).isFile() && QFileInfo(destination).size() == size_) {
        hashFile();
        return;
    }
    QFile meta(destination + ".json");
    QJsonObject record;
    if (meta.open(QIODevice::ReadOnly) && meta.size() < 8192)
        record = QJsonDocument::fromJson(meta.readAll()).object();
    meta.close();
    if (record["url"].toString() != url.toString() ||
        record["sha256"].toString() != QString::fromLatin1(expected.toHex()) ||
        record["size"].toDouble() != size) {
        if (QFileInfo::exists(destination + ".part") && !QFile::remove(destination + ".part")) {
            fail("remove: " + destination + ".part");
            return;
        }
    } else
        etag_ = record["etag"].toString().toLatin1();
    request();
}
void UpdateDownload::journal() {
    QSaveFile f(destination_ + ".json");
    QJsonObject o{{"url", url_.toString()},
                  {"sha256", QString::fromLatin1(expected_.toHex())},
                  {"size", double(size_)},
                  {"etag", QString::fromLatin1(etag_)}};
    const auto bytes = QJsonDocument(o).toJson(QJsonDocument::Compact);
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit())
        localError_ = "journal: " + f.errorString();
}
void UpdateDownload::request() {
    if (reply_)
        return;
    pauseWanted_ = cancelWanted_ = false;
    checkedHeaders_ = false;
    localError_.clear();
    part_.setFileName(destination_ + ".part");
    if (!part_.open(QIODevice::ReadWrite)) {
        fail(part_.errorString());
        return;
    }
    offset_ = part_.size();
    if (offset_ > size_) {
        part_.close();
        fail("oversize", true);
        return;
    }
    if (offset_ == size_) {
        part_.close();
        hashFile();
        return;
    }
    if (!part_.seek(offset_)) {
        fail(part_.errorString());
        return;
    }
    journal();
    if (!localError_.isEmpty()) {
        fail(localError_);
        return;
    }
    QNetworkRequest r(url_);
    r.setTransferTimeout(30000);
    r.setRawHeader("Accept-Encoding", "identity");
    r.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    if (offset_) {
        r.setRawHeader("Range", "bytes=" + QByteArray::number(offset_) + "-");
        if (!etag_.isEmpty())
            r.setRawHeader("If-Range", etag_);
    }
    speedClock_.restart();
    speedBytes_ = 0;
    currentSpeed_ = 0;
    setState(State::Downloading);
    reply_ = network_.get(r);
    reply_->setReadBufferSize(256 * 1024);
    connect(reply_, &QNetworkReply::readyRead, this, &UpdateDownload::consume);
    connect(reply_, &QNetworkReply::finished, this, &UpdateDownload::finish);
}
bool UpdateDownload::headers() {
    if (checkedHeaders_)
        return localError_.isEmpty();
    const int status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (!status)
        return false;
    if (!allowed(reply_->url())) {
        localError_ = "redirect";
        return false;
    }
    if (status == 200) {
        if (offset_) {
            if (!part_.resize(0) || !part_.seek(0)) {
                localError_ = part_.errorString();
                return false;
            }
            offset_ = 0;
        }
    } else if (status == 206) {
        const auto m = QRegularExpression("^bytes ([0-9]+)-([0-9]+)/([0-9]+)$")
                           .match(QString::fromLatin1(reply_->rawHeader("Content-Range")));
        if (!m.hasMatch() || m.captured(1).toLongLong() != offset_ ||
            m.captured(2).toLongLong() != size_ - 1 || m.captured(3).toLongLong() != size_) {
            localError_ = "Content-Range";
            return false;
        }
    } else
        return false;
    const auto encoding = reply_->rawHeader("Content-Encoding");
    if (!encoding.isEmpty() && encoding != "identity") {
        localError_ = "Content-Encoding";
        return false;
    }
    if (reply_->hasRawHeader("Content-Length") &&
        reply_->rawHeader("Content-Length").toLongLong() != size_ - offset_) {
        localError_ = "Content-Length";
        return false;
    }
    const auto tag = reply_->rawHeader("ETag");
    etag_ = (!tag.startsWith("W/") && tag.startsWith('"') && tag.endsWith('"')) ? tag : QByteArray();
    journal();
    checkedHeaders_ = localError_.isEmpty();
    return checkedHeaders_;
}
void UpdateDownload::consume() {
    if (!reply_ || pauseWanted_ || cancelWanted_)
        return;
    if (!headers()) {
        if (!localError_.isEmpty())
            reply_->abort();
        return;
    }
    while (reply_->bytesAvailable()) {
        auto chunk = reply_->read(65536);
        if (chunk.isEmpty())
            break;
        if (part_.pos() + chunk.size() > size_ || part_.write(chunk) != chunk.size()) {
            localError_ = "write: " + part_.errorString();
            reply_->abort();
            return;
        }
        speedBytes_ += chunk.size();
    }
    const auto elapsed = speedClock_.elapsed();
    if (elapsed >= 500) {
        currentSpeed_ = speedBytes_ * 1000.0 / elapsed;
        speedBytes_ = 0;
        speedClock_.restart();
    }
    const double speed =
        currentSpeed_ > 0 ? currentSpeed_ : (elapsed > 0 ? speedBytes_ * 1000.0 / elapsed : 0);
    emit progress(part_.pos(), size_, speed, speed > 0 ? qint64((size_ - part_.pos()) / speed) : -1);
}
void UpdateDownload::finish() {
    consume();
    const auto error = reply_->error();
    const auto status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto detail = reply_->errorString();
    reply_->deleteLater();
    reply_ = nullptr;
    const auto bytes = part_.pos();
    const bool flushed = part_.flush();
    part_.close();
    if (cancelWanted_) {
        if (!keep_) {
            QFile::remove(destination_ + ".part");
            QFile::remove(destination_ + ".json");
        }
        lock_.reset();
        setState(State::Cancelled);
        return;
    }
    if (pauseWanted_) {
        if (!flushed) {
            fail("flush");
            return;
        }
        setState(State::Paused);
        return;
    }
    if (!localError_.isEmpty()) {
        fail(localError_, localError_.startsWith("Content-"));
        return;
    }
    if (!flushed) {
        fail("flush");
        return;
    }
    if (error == QNetworkReply::NoError && checkedHeaders_ && bytes == size_) {
        hashFile();
        return;
    }
    if ((status == 0 || status == 408 || status == 429 || status >= 500 ||
         error == QNetworkReply::RemoteHostClosedError || error == QNetworkReply::TimeoutError) &&
        retries_ < 3) {
        setState(State::Retrying);
        retry_.start(1000 * (1 << retries_++));
        return;
    }
    fail(QString("HTTP %1: %2").arg(status).arg(detail));
}
void UpdateDownload::hashFile() {
    part_.close();
    hashFile_.setFileName(QFileInfo::exists(destination_) ? destination_ : destination_ + ".part");
    if (!hashFile_.open(QIODevice::ReadOnly) || hashFile_.size() != size_) {
        fail("size/read");
        return;
    }
    hash_ = std::make_unique<QCryptographicHash>(QCryptographicHash::Sha256);
    setState(State::Hashing);
    hashTimer_.start();
}
void UpdateDownload::pause() {
    if (state_ == State::Retrying) {
        retry_.stop();
        setState(State::Paused);
        return;
    }
    if (state_ != State::Downloading || !reply_)
        return;
    pauseWanted_ = true;
    reply_->abort();
}
void UpdateDownload::resume() {
    if ((state_ == State::Paused || state_ == State::Cancelled || state_ == State::Error) &&
        !destination_.isEmpty()) {
        if (!lock_) {
            lock_ = std::make_unique<QLockFile>(destination_ + ".lock");
            lock_->setStaleLockTime(0);
            if (!lock_->tryLock(0)) {
                fail("locked: " + destination_);
                return;
            }
        }
        retries_ = 0;
        request();
    }
}
void UpdateDownload::cancel(bool keep) {
    if (destination_.isEmpty()) {
        setState(State::Cancelled);
        return;
    }
    retry_.stop();
    if (state_ == State::Hashing) {
        hashTimer_.stop();
        hashFile_.close();
    }
    keep_ = keep;
    cancelWanted_ = true;
    if (reply_) {
        reply_->abort();
        return;
    }
    part_.close();
    if (!keep) {
        QFile::remove(destination_ + ".part");
        QFile::remove(destination_ + ".json");
    }
    lock_.reset();
    setState(State::Cancelled);
}
void UpdateDownload::fail(const QString &reason, bool discard) {
    retry_.stop();
    hashTimer_.stop();
    hashFile_.close();
    part_.close();
    if (discard) {
        QFile::remove(destination_);
        QFile::remove(destination_ + ".part");
        QFile::remove(destination_ + ".json");
    }
    lock_.reset();
    setState(State::Error);
    emit failed(reason);
}
