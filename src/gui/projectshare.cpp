#include "projectshare.hpp"
#include "monocypher.h"
#include "qrcodegen.hpp"
#include "quirc.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QtEndian>
#include <zlib.h>
#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace {
[[noreturn]] void fail(const char *message) {
    throw std::runtime_error(message);
}
QByteArray rawCode(const QString &code) {
    if (code.size() > ProjectShare::MaximumCodeBytes)
        fail("Share-Code ist zu groß.");
    const auto value = code.trimmed();
    if (!value.startsWith("TC1:"))
        fail("Unbekanntes Share-Code-Format.");
    const auto result = QByteArray::fromBase64Encoding(
        value.mid(4).toLatin1(), QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    if (!result || result.decoded.size() < 9 || !result.decoded.startsWith("TCS1") ||
        (result.decoded[4] != 0 && result.decoded[4] != 1))
        fail("Share-Code ist beschädigt oder nicht unterstützt.");
    return result.decoded;
}
struct Secret {
    std::array<uint8_t, 32> bytes{};
    ~Secret() {
        crypto_wipe(bytes.data(), bytes.size());
    }
};
void derive(Secret &key, QByteArray &password, const QByteArray &salt) {
    if (password.isEmpty())
        fail("Dieser Share-Code benötigt ein Passwort.");
    if (password.size() > 1024)
        fail("Das Passwort ist zu lang.");
    // Fixed costs are format-versioned, never selected by untrusted input.
    std::vector<uint8_t> work(32 * 1024 * 1024);
    crypto_argon2_config config{CRYPTO_ARGON2_ID, 32768, 3, 1};
    crypto_argon2_inputs inputs{reinterpret_cast<const uint8_t *>(password.constData()),
                                reinterpret_cast<const uint8_t *>(salt.constData()),
                                uint32_t(password.size()), uint32_t(salt.size())};
    crypto_argon2(key.bytes.data(), 32, work.data(), config, inputs, crypto_argon2_no_extras);
    crypto_wipe(work.data(), work.size());
    crypto_wipe(password.data(), password.size());
}
QByteArray randomBytes(int count) {
    QByteArray out(count, Qt::Uninitialized);
    for (int i = 0; i < count; i += 4) {
        auto value = QRandomGenerator::system()->generate();
        std::memcpy(out.data() + i, &value, std::min(4, count - i));
    }
    return out;
}
} // namespace
QString ProjectShare::encode(SavedProject project, const QJsonArray &libraries, QByteArray password) {
    QJsonArray references = project.libraryReferences;
    for (const auto &value : libraries) {
        const auto name = value.toObject()["name"].toString();
        for (qsizetype i = references.size(); i > 0; --i)
            if (references[i - 1].toObject()["name"].toString() == name)
                references.removeAt(i - 1);
        references.append(value);
    }
    project.libraryReferences = references;
    project.bundledLibraries.clear(); // References only: no silent code/license disclosure.
    const auto projectJson = QJsonDocument::fromJson(ProjectIO::serialize(project)).object();
    const auto plain =
        QJsonDocument(
            QJsonObject{{"project", projectJson}, {"libraries", references}, {"format", "ThinkCad-Share-1"}})
            .toJson(QJsonDocument::Compact);
    if (plain.size() > MaximumProjectBytes)
        fail("Projekt ist zu groß für einen Share-Code.");
    QByteArray header("TCS1", 4);
    header.append(password.isEmpty() ? char(0) : char(1));
    std::array<char, 4> size{};
    qToBigEndian<quint32>(quint32(plain.size()), size.data());
    header.append(size.data(), 4);
    const auto compressed = qCompress(plain, 9).mid(4);
    QByteArray envelope;
    if (password.isEmpty()) {
        envelope = header + compressed;
        envelope += QCryptographicHash::hash(envelope, QCryptographicHash::Sha256);
    } else {
        const auto salt = randomBytes(16), nonce = randomBytes(24);
        header += salt;
        header += nonce;
        Secret key;
        derive(key, password, salt);
        QByteArray cipher(compressed.size(), Qt::Uninitialized);
        std::array<uint8_t, 16> tag{};
        crypto_aead_lock(reinterpret_cast<uint8_t *>(cipher.data()), tag.data(), key.bytes.data(),
                         reinterpret_cast<const uint8_t *>(nonce.constData()),
                         reinterpret_cast<const uint8_t *>(header.constData()), header.size(),
                         reinterpret_cast<const uint8_t *>(compressed.constData()), compressed.size());
        envelope = header + cipher + QByteArray(reinterpret_cast<const char *>(tag.data()), tag.size());
    }
    const auto code = "TC1:" + QString::fromLatin1(envelope.toBase64(QByteArray::Base64UrlEncoding |
                                                                     QByteArray::OmitTrailingEquals));
    if (code.size() > MaximumCodeBytes)
        fail("Share-Code ist zu groß.");
    return code;
}
DecodedShare ProjectShare::decode(const QString &code, QByteArray password) {
    const auto envelope = rawCode(code);
    const bool locked = envelope[4] == 1;
    const quint32 size = qFromBigEndian<quint32>(envelope.constData() + 5);
    if (!size || size > MaximumProjectBytes)
        fail("Ungültige Projektgröße im Share-Code.");
    QByteArray compressed;
    if (locked) {
        if (envelope.size() < 65)
            fail("Verschlüsselter Share-Code ist unvollständig.");
        const auto salt = envelope.mid(9, 16), nonce = envelope.mid(25, 24);
        Secret key;
        derive(key, password, salt);
        const auto cipher = envelope.mid(49, envelope.size() - 65);
        compressed.resize(cipher.size());
        if (crypto_aead_unlock(reinterpret_cast<uint8_t *>(compressed.data()),
                               reinterpret_cast<const uint8_t *>(envelope.constData() + envelope.size() - 16),
                               key.bytes.data(), reinterpret_cast<const uint8_t *>(nonce.constData()),
                               reinterpret_cast<const uint8_t *>(envelope.constData()), 49,
                               reinterpret_cast<const uint8_t *>(cipher.constData()), cipher.size()))
            fail("Passwort falsch oder Share-Code beschädigt. Es wurde nichts importiert.");
    } else {
        if (envelope.size() < 41 ||
            QCryptographicHash::hash(envelope.chopped(32), QCryptographicHash::Sha256) != envelope.last(32))
            fail("Share-Code-Prüfsumme stimmt nicht. Bitte den vollständigen Code erneut kopieren.");
        compressed = envelope.mid(9, envelope.size() - 41);
    }
    QByteArray plain(int(size), Qt::Uninitialized);
    uLongf length = size;
    if (uncompress(reinterpret_cast<Bytef *>(plain.data()), &length,
                   reinterpret_cast<const Bytef *>(compressed.constData()), compressed.size()) != Z_OK ||
        length != size)
        fail("Share-Code konnte nicht sicher entpackt werden.");
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(plain, &error);
    if (error.error != QJsonParseError::NoError || !json.isObject())
        fail("Ungültiges Projekt im Share-Code.");
    const auto object = json.object();
    if (object["format"].toString() != "ThinkCad-Share-1" || !object["project"].isObject() ||
        !object["libraries"].isArray() || object["libraries"].toArray().size() > 256)
        fail("Ungültige Share-Code-Metadaten.");
    for (const auto &value : object["libraries"].toArray()) {
        if (!value.isObject())
            fail("Ungültige Share-Code-Metadaten.");
        const auto library = value.toObject();
        const auto name = library["name"].toString(), version = library["version"].toString();
        if (!library["name"].isString() || !library["version"].isString() || name.isEmpty() ||
            name.size() > 128 || version.size() > 64 ||
            std::any_of(name.begin(), name.end(), [](QChar c) { return c.unicode() < 32; }) ||
            std::any_of(version.begin(), version.end(), [](QChar c) { return c.unicode() < 32; }))
            fail("Ungültige Share-Code-Metadaten.");
    }
    auto project = object["project"].toObject();
    if (project["format"].toString() != "ThinkCad" || project["version"].toInt() < 1 ||
        project["version"].toInt() > 8 || !project["parts"].isArray() || !project["wires"].isArray() ||
        !project["files"].isArray() || project["parts"].toArray().size() > 10000 ||
        project["wires"].toArray().size() > 50000 ||
        (!project["libraries"].isUndefined() && !project["libraries"].toObject().isEmpty()))
        fail("Projektformat oder Projektinhalt wird nicht unterstützt.");
    project["libraryReferences"] = object["libraries"].toArray();
    return {QJsonDocument(project).toJson(QJsonDocument::Compact), object["libraries"].toArray(), locked};
}
bool ProjectShare::encrypted(const QString &code) {
    return rawCode(code)[4] == 1;
}
QImage ProjectShare::qrImage(const QString &code, int scale) {
    if (code.toUtf8().size() > MaximumQrBytes)
        fail("Projekt ist zu groß für einen QR-Code. Bitte den Share-Code kopieren.");
    if (scale < 1 || scale > 16)
        fail("Ungültige QR-Bildgröße.");
    const auto qr = qrcodegen::QrCode::encodeText(code.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
    QImage image((qr.getSize() + 8) * scale, (qr.getSize() + 8) * scale, QImage::Format_Grayscale8);
    image.fill(255);
    for (int y = 0; y < qr.getSize(); ++y)
        for (int x = 0; x < qr.getSize(); ++x)
            if (qr.getModule(x, y))
                for (int dy = 0; dy < scale; ++dy)
                    std::memset(image.scanLine((y + 4) * scale + dy) + (x + 4) * scale, 0, scale);
    return image;
}
QString ProjectShare::readQr(const QImage &source) {
    if (source.isNull() || qint64(source.width()) * source.height() > 16000000)
        fail("QR-Bild fehlt oder ist zu groß.");
    const auto image = source.convertToFormat(QImage::Format_Grayscale8);
    std::unique_ptr<quirc, decltype(&quirc_destroy)> reader(quirc_new(), quirc_destroy);
    if (!reader || quirc_resize(reader.get(), image.width(), image.height()) < 0)
        fail("QR-Bild konnte nicht gelesen werden.");
    auto *buffer = quirc_begin(reader.get(), nullptr, nullptr);
    for (int y = 0; y < image.height(); ++y)
        std::memcpy(buffer + y * image.width(), image.constScanLine(y), image.width());
    quirc_end(reader.get());
    QString found;
    for (int i = 0; i < quirc_count(reader.get()); ++i) {
        auto code = std::make_unique<quirc_code>();
        auto data = std::make_unique<quirc_data>();
        quirc_extract(reader.get(), i, code.get());
        auto error = quirc_decode(code.get(), data.get());
        if (error) {
            quirc_flip(code.get());
            error = quirc_decode(code.get(), data.get());
        }
        if (error)
            continue;
        const auto value =
            QString::fromUtf8(reinterpret_cast<const char *>(data->payload), data->payload_len);
        if (!value.startsWith("TC1:"))
            continue;
        if (!found.isEmpty())
            fail("Mehrere ThinkCad-QR-Codes erkannt. Bitte nur ein Projekt importieren.");
        rawCode(value);
        found = value;
    }
    if (found.isEmpty())
        fail("Kein lesbarer ThinkCad-QR-Code im Bild gefunden.");
    return found;
}
