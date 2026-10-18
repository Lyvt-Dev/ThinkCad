#include "projectexport.hpp"
#include "circuitcanvas.hpp"
#include "language.hpp"
#include "thinkercad/modules.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QSvgGenerator>
#include <QSignalBlocker>
#include <QSettings>
#include <QCryptographicHash>
#include <zlib.h>
#include <cmath>
#include <stdexcept>
namespace {
void need(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void u16(QByteArray &bytes, quint16 value) {
    bytes += char(value);
    bytes += char(value >> 8);
}
void u32(QByteArray &bytes, quint32 value) {
    u16(bytes, quint16(value));
    u16(bytes, quint16(value >> 16));
}
QString quoted(QString value) {
    if (!value.isEmpty() && QString("=+-@").contains(value.front()))
        value.prepend('\'');
    value.replace('"', "\"\"");
    return '"' + value + '"';
}
QString label(const thinkercad::CircuitPart &p) {
    if (p.kind == thinkercad::PartKind::Module)
        return Language::text(QString::fromStdString(thinkercad::ModuleRegistry::get(p.model).label));
    if (p.kind == thinkercad::PartKind::Esp8266)
        return QString::fromStdString(thinkercad::BoardProfile::get(p.model).label);
    return Language::text(QString::fromStdString(thinkercad::componentSpec(p.kind).label));
}
} // namespace
QByteArray ZipWriter::archive(const QMap<QString, QByteArray> &files) {
    need(files.size() <= 2000, "Too many archive files");
    QByteArray output, central;
    QSet<QString> names;
    for (auto it = files.begin(); it != files.end(); ++it) {
        const auto path = it.key();
        const auto pieces = path.split('/');
        need(!path.isEmpty() && path.size() < 512 && !path.startsWith('/') && !path.contains('\\') &&
                 !path.contains(':') && !path.contains(QChar::Null) && QDir::cleanPath(path) == path &&
                 !pieces.contains("..") && !pieces.contains(".") && !pieces.contains(""),
             "Unsafe archive path");
        for (const auto &piece : pieces) {
            auto base = piece.section('.', 0, 0).toUpper();
            need(!piece.endsWith('.') && !piece.endsWith(' ') &&
                     !QStringList{"CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4",
                                  "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3",
                                  "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"}
                          .contains(base),
                 "Unsafe archive filename");
        }
        need(!names.contains(path.toCaseFolded()), "Archive filename collision");
        names.insert(path.toCaseFolded());
        const auto name = path.toUtf8(), data = it.value();
        need(data.size() <= 32 * 1024 * 1024 && output.size() + data.size() < 128 * 1024 * 1024,
             "Archive exceeds size limit");
        const auto crc = quint32(
                       crc32(0, reinterpret_cast<const Bytef *>(data.constData()), uInt(data.size()))),
                   offset = quint32(output.size());
        u32(output, 0x04034b50);
        u16(output, 20);
        u16(output, 0x800);
        u16(output, 0);
        u16(output, 0);
        u16(output, 33);
        u32(output, crc);
        u32(output, quint32(data.size()));
        u32(output, quint32(data.size()));
        u16(output, quint16(name.size()));
        u16(output, 0);
        output += name;
        output += data;
        u32(central, 0x02014b50);
        u16(central, 20);
        u16(central, 20);
        u16(central, 0x800);
        u16(central, 0);
        u16(central, 0);
        u16(central, 33);
        u32(central, crc);
        u32(central, quint32(data.size()));
        u32(central, quint32(data.size()));
        u16(central, quint16(name.size()));
        u16(central, 0);
        u16(central, 0);
        u16(central, 0);
        u16(central, 0);
        u32(central, 0);
        u32(central, offset);
        central += name;
    }
    auto offset = quint32(output.size());
    output += central;
    u32(output, 0x06054b50);
    u16(output, 0);
    u16(output, 0);
    u16(output, quint16(files.size()));
    u16(output, quint16(files.size()));
    u32(output, quint32(central.size()));
    u32(output, offset);
    u16(output, 0);
    return output;
}
QByteArray BomBuilder::csv(const thinkercad::CircuitDocument &doc) {
    using namespace thinkercad;
    struct Row {
        QString name, value, model;
        QStringList ids, pins;
    };
    QMap<QString, Row> rows;
    for (const auto &p : doc.parts()) {
        if (p.kind == PartKind::Note || p.kind == PartKind::Junction || p.kind == PartKind::NetLabel)
            continue;
        auto pins = CircuitDocument::pins(p);
        QStringList pinNames;
        for (const auto &pin : pins)
            pinNames << QString::fromStdString(pin);
        auto value = QString::number(p.value, 'g', 12) + QString::fromStdString(componentSpec(p.kind).unit);
        auto model = QString::fromStdString(p.model);
        const auto key = QString::number(int(p.kind)) + "/" + model + "/" + value + "/" + pinNames.join('|');
        auto &row = rows[key];
        row.name = label(p);
        row.value = value;
        row.model = model;
        row.ids << QString::fromStdString(p.id);
        row.pins = pinNames;
    }
    QString out;
    for (const auto &column : {"Name", "Modell", "Wert", "Anzahl", "Referenzen", "Pins"}) {
        if (!out.isEmpty())
            out += ',';
        out += quoted(Language::text(column));
    }
    out += '\n';
    for (const auto &row : rows)
        out += quoted(row.name) + ',' + quoted(row.model) + ',' + quoted(row.value) + ',' +
               QString::number(row.ids.size()) + ',' + quoted(row.ids.join(' ')) + ',' +
               quoted(row.pins.join(' ')) + '\n';
    return out.toUtf8();
}
QMap<QString, QByteArray> ProjectExporter::files(SavedProject project, const QStringList &libraries,
                                                 bool include) {
    QMap<QString, QByteArray> files;
    QJsonArray manifest;
    QStringList all = libraries;
    all.removeDuplicates();
    for (const auto &root : all) {
        QFile file(QDir(root).filePath("library.properties"));
        QString name = QFileInfo(root).fileName(), version;
        QByteArray properties;
        if (file.open(QIODevice::ReadOnly))
            properties = file.read(65536);
        for (const auto &line : properties.split('\n')) {
            if (line.startsWith("name="))
                name = QString::fromUtf8(line.mid(5)).trimmed();
            if (line.startsWith("version="))
                version = QString::fromUtf8(line.mid(8)).trimmed();
        }
        manifest.append(QJsonObject{{"name", name}, {"version", version}, {"origin", "local"}});
    }
    if (include) {
        auto bundled = ProjectIO::bundleLibraries(all);
        for (auto it = bundled.begin(); it != bundled.end(); ++it)
            project.bundledLibraries[it.key()] = it.value();
    } else
        project.bundledLibraries.clear();
    auto sketches = project.boardSketches;
    if (sketches.isEmpty())
        sketches["unassigned"] = project.sketch;
    for (auto it = sketches.begin(); it != sketches.end(); ++it) {
        const auto primary = it->entryPoint();
        const auto folder = "sketches/" + it.key() + "/" + QFileInfo(primary).completeBaseName() + "/";
        for (const auto &name : it->order) {
            need(ProjectIO::validFileName(name), "Unsafe sketch filename");
            files[folder + name] = it->files.value(name).toUtf8();
        }
    }
    for (auto it = project.bundledLibraries.begin(); it != project.bundledLibraries.end(); ++it)
        files["libraries/" + it.key()] = it.value();
    files["project.thinkcad"] = ProjectIO::serialize(project);
    files["bom.csv"] = BomBuilder::csv(project.circuit);
    files["libraries.json"] =
        QJsonDocument(
            QJsonObject{
                {"format", "ThinkCad.LibraryManifest"},
                {"version", 1},
                {"localLibraries", manifest},
                {"filesIncluded", include},
                {"note", "Local compiler library roots; bundled simulator SDK is provided by ThinkCad."}})
            .toJson();
    files["README.md"] =
        (Language::text("ThinkCad-Projektexport") + "\n\n" +
         Language::text(
             "project.thinkcad in ThinkCad öffnen. Sketches stehen getrennt pro Board im Ordner sketches. "
             "Bibliotheken sind in libraries.json aufgeführt. Es wurde nichts hochgeladen.") +
         "\n\n" +
         Language::text(
             include ? "Lokale Bibliotheksdateien sind enthalten."
                     : "Lokale Bibliotheksdateien sind nicht enthalten; bei Bedarf separat installieren.") +
         "\n")
            .toUtf8();
    return files;
}
void ProjectExporter::saveZip(const QString &path, SavedProject project, const QStringList &libraries,
                              bool include) {
    auto bytes = ZipWriter::archive(files(std::move(project), libraries, include));
    QSaveFile file(path);
    need(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit(),
         "Could not write project archive");
}
void CircuitExporter::save(CircuitCanvas &canvas, const QString &path, bool whole) {
    const bool svg = QFileInfo(path).suffix().compare("svg", Qt::CaseInsensitive) == 0;
    canvas.clearNetPreview();
    const auto source = whole ? canvas.scene()->itemsBoundingRect().adjusted(-20, -20, 20, 20)
                              : QRectF(canvas.viewport()->rect());
    need(source.isValid() && source.width() <= 16000 && source.height() <= 16000 &&
             source.width() * source.height() <= 32 * 1024 * 1024,
         "Image is too large");
    QSaveFile file(path);
    need(file.open(QIODevice::WriteOnly), "Could not open image export");
    QSignalBlocker sceneSignals(canvas.scene());
    const auto selection = canvas.scene()->selectedItems();
    canvas.scene()->clearSelection();
    std::vector<QGraphicsItem *> hidden;
    for (auto *item : canvas.scene()->items())
        if (item->isVisible() && (item->data(0) == "bend" || item->data(0) == "netPreview")) {
            item->hide();
            hidden.push_back(item);
        }
    auto render = [&](QPainter &p) {
        p.setRenderHint(QPainter::Antialiasing);
        if (whole)
            canvas.scene()->render(&p, QRectF(QPointF(0, 0), source.size()), source);
        else
            canvas.render(&p, QRectF(QPointF(0, 0), source.size()), source.toRect());
    };
    bool success = false;
    if (svg) {
        QSvgGenerator generator;
        generator.setOutputDevice(&file);
        generator.setSize(source.size().toSize());
        generator.setViewBox(QRectF(QPointF(0, 0), source.size()));
        generator.setTitle("ThinkCad");
        QPainter painter(&generator);
        render(painter);
        success = painter.end();
    } else {
        QImage image(source.size().toSize(), QImage::Format_ARGB32_Premultiplied);
        image.fill(canvas.palette().color(QPalette::Base));
        QPainter painter(&image);
        render(painter);
        painter.end();
        success = image.save(&file, "PNG");
    }
    for (auto *item : hidden)
        item->show();
    for (auto *item : selection)
        item->setSelected(true);
    need(success && file.commit(), "Could not save image export");
}
