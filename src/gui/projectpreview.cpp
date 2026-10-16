#include "projectpreview.hpp"
#include "circuitcanvas.hpp"
#include "projectio.hpp"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QImageReader>
#include <QPainter>
#include <QPalette>
#include <QSaveFile>
#include <QStandardPaths>
#include <exception>

namespace {
constexpr int width = 320, height = 192;
QString revision(const QFileInfo& file) {
    return QString::number(file.size()) + ":" + QString::number(file.lastModified().toMSecsSinceEpoch());
}
QString cachePath(const QFileInfo& file) {
    const auto root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (root.isEmpty()) return {};
    auto path = file.canonicalFilePath();
#ifdef Q_OS_WIN
    path = path.toCaseFolded();
#endif
    return root + "/project-previews/" +
        QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha256).toHex()) + ".png";
}
void store(const QFileInfo& source, QImage image) {
    const auto path = cachePath(source);
    if (path.isEmpty() || image.isNull() || !QDir().mkpath(QFileInfo(path).absolutePath())) return;
    image.setText("sourceRevision", revision(source));
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly) || !image.save(&output, "PNG") || !output.commit()) return;
    // Keep a bounded cache, including projects removed from the recent list.
    QDir directory(QFileInfo(path).absolutePath());
    const auto files = directory.entryInfoList({"*.png"}, QDir::Files, QDir::Time);
    for (qsizetype i = 64; i < files.size(); ++i) directory.remove(files[i].fileName());
}
}
QImage ProjectPreview::capture(CircuitCanvas& canvas) {
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(canvas.palette().color(QPalette::Window));
    auto bounds = canvas.scene()->itemsBoundingRect();
    if (bounds.isEmpty()) return image;
    bounds.adjust(-24, -24, 24, 24);
    const auto fitted = bounds.size().scaled(QSizeF(width, height), Qt::KeepAspectRatio);
    const QRectF target((width - fitted.width()) / 2, (height - fitted.height()) / 2, fitted.width(), fitted.height());
    QPainter painter(&image);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    canvas.scene()->render(&painter, target, bounds, Qt::KeepAspectRatio);
    return image;
}
void ProjectPreview::update(const QString& path, CircuitCanvas& canvas) {
    const QFileInfo file(path);
    if (file.isFile()) store(file, capture(canvas));
}
QImage ProjectPreview::load(const QString& path) {
    const QFileInfo file(path);
    if (!file.isFile()) return {};
    QImageReader reader(cachePath(file), "PNG");
    // Never allocate an image of an arbitrary size from a corrupt cache entry.
    if (reader.size() == QSize(width, height)) {
        auto image = reader.read();
        if (!image.isNull() && image.text("sourceRevision") == revision(file)) return image;
    }
    const auto suffix = file.suffix().toLower();
    if (suffix != "thinkcad" && suffix != "thinkercad") return {};
    try {
        auto project = ProjectIO::load(path);
        CircuitCanvas canvas(project.circuit);
        auto image = capture(canvas);
        store(file, image);
        return image;
    } catch (const std::exception&) {
        return {}; // A missing or invalid preview must not prevent using the menu.
    }
}
