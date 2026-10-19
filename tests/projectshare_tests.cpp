#include "projectshare.hpp"
#include "language.hpp"
#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QSettings>
#include <QTemporaryDir>
#include <QCryptographicHash>
#include <QtEndian>
class ProjectShareTests : public QObject {
    Q_OBJECT
    QTemporaryDir settings_;
    static SavedProject project() {
        SavedProject p;
        p.sketch.name = "Shared blink";
        p.sketch.order = {"main.ino"};
        p.sketch.files["main.ino"] = "void setup() {}\nvoid loop() {}\n";
        p.sketch.breakpoints["main.ino"] = {2};
        const auto led = p.circuit.addPart(thinkercad::PartKind::Led, 300, 100);
        p.circuit.connect({led, "K"}, {"board", "GND"});
        return p;
    }
  private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("ThinkCadShareTests");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings_.path());
        Language::set("en");
    }
    void roundTripKeepsCircuitSketchAndReferences() {
        auto original = project();
        auto second = original.circuit.addPart(thinkercad::PartKind::Esp8266, 700, 0);
        original.boardSketches[QString::fromStdString(second)] = original.sketch;
        original.bundledLibraries["private/password.txt"] = "must never be embedded";
        QJsonArray libraries{QJsonObject{{"name", "ExampleLibrary"}, {"version", "1.2.3"}}};
        const auto code = ProjectShare::encode(original, libraries);
        QVERIFY(code.startsWith("TC1:"));
        QVERIFY(!ProjectShare::encrypted(code));
        const auto decoded = ProjectShare::decode(code);
        QCOMPARE(decoded.libraries, libraries);
        QVERIFY(!decoded.encrypted);
        const auto loaded = ProjectIO::deserialize(decoded.project);
        QCOMPARE(loaded.circuit.parts().size(), original.circuit.parts().size());
        QCOMPARE(loaded.circuit.wires().size(), original.circuit.wires().size());
        QCOMPARE(loaded.sketch.files, original.sketch.files);
        QCOMPARE(loaded.sketch.breakpoints, original.sketch.breakpoints);
        QCOMPARE(loaded.boardSketches.size(), 1);
        QVERIFY(loaded.bundledLibraries.isEmpty());
        QCOMPARE(loaded.libraryReferences, libraries);
        const auto saved = ProjectIO::deserialize(ProjectIO::serialize(loaded));
        QCOMPARE(saved.libraryReferences, libraries);
        QCOMPARE(ProjectShare::decode(ProjectShare::encode(saved)).libraries, libraries);
        QVERIFY(!decoded.project.contains("must never be embedded"));
    }
    void encryptedRoundTripAndWrongPassword() {
        const auto first = ProjectShare::encode(project(), {}, "correct horse battery staple");
        const auto second = ProjectShare::encode(project(), {}, "correct horse battery staple");
        QVERIFY(first != second);
        QVERIFY(ProjectShare::encrypted(first));
        const auto decoded = ProjectShare::decode(first, "correct horse battery staple");
        QVERIFY(decoded.encrypted);
        QCOMPARE(ProjectIO::deserialize(decoded.project).sketch.name, QString("Shared blink"));
        QVERIFY_EXCEPTION_THROWN(ProjectShare::decode(first, "wrong password"), std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(ProjectShare::decode(first), std::runtime_error);
        auto data = QByteArray::fromBase64(first.mid(4).toLatin1(), QByteArray::Base64UrlEncoding);
        data[30] = data[30] ^ 1;
        const auto tampered = "TC1:" + QString::fromLatin1(data.toBase64(QByteArray::Base64UrlEncoding |
                                                                         QByteArray::OmitTrailingEquals));
        QVERIFY_EXCEPTION_THROWN(ProjectShare::decode(tampered, "correct horse battery staple"),
                                 std::runtime_error);
    }
    void rejectsMalformedPersistedLibraryReferences() {
        auto source = project();
        source.libraryReferences = QJsonArray{QJsonObject{{"name", ""}, {"version", "1"}}};
        QVERIFY_EXCEPTION_THROWN(ProjectIO::serialize(source), std::runtime_error);
        auto data = QJsonDocument::fromJson(ProjectIO::serialize(project())).object();
        data["libraryReferences"] = "invalid";
        QVERIFY_EXCEPTION_THROWN(ProjectIO::deserialize(QJsonDocument(data).toJson()), std::runtime_error);
    }
    void rejectsCorruptionAndOversizedOutput() {
        QVERIFY_EXCEPTION_THROWN(ProjectShare::decode("TC2:invalid"), std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(ProjectShare::decode("TC1:%%%"), std::runtime_error);
        const auto code = ProjectShare::encode(project());
        auto bytes = QByteArray::fromBase64(code.mid(4).toLatin1(), QByteArray::Base64UrlEncoding);
        bytes[10] = bytes[10] ^ 1;
        QVERIFY_EXCEPTION_THROWN(
            ProjectShare::decode("TC1:" + QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding))),
            std::runtime_error);
        bytes = QByteArray::fromBase64(code.mid(4).toLatin1(), QByteArray::Base64UrlEncoding);
        bytes.chop(32);
        qToBigEndian<quint32>(ProjectShare::MaximumProjectBytes + 1, bytes.data() + 5);
        bytes += QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
        QVERIFY_EXCEPTION_THROWN(
            ProjectShare::decode("TC1:" + QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding))),
            std::runtime_error);
    }
    void qrRoundTripAndCapacity() {
        const auto code = ProjectShare::encode(project());
        QVERIFY(code.toUtf8().size() <= ProjectShare::MaximumQrBytes);
        const auto image = ProjectShare::qrImage(code);
        QVERIFY(!image.isNull());
        QCOMPARE(ProjectShare::readQr(image), code);
        QVERIFY_EXCEPTION_THROWN(ProjectShare::qrImage(QString(ProjectShare::MaximumQrBytes + 1, 'x')),
                                 std::runtime_error);
        QImage empty(128, 128, QImage::Format_Grayscale8);
        empty.fill(255);
        QVERIFY_EXCEPTION_THROWN(ProjectShare::readQr(empty), std::runtime_error);
    }
    void rejectsMultipleProjectQrCodes() {
        const auto code = ProjectShare::encode(project());
        const auto qr = ProjectShare::qrImage(code);
        QImage pair(qr.width() * 2 + 40, qr.height(), QImage::Format_RGB32);
        pair.fill(Qt::white);
        {
            QPainter painter(&pair);
            painter.drawImage(0, 0, qr);
            painter.drawImage(qr.width() + 40, 0, qr);
        }
        QVERIFY_EXCEPTION_THROWN(ProjectShare::readQr(pair), std::runtime_error);
    }
};
QTEST_MAIN(ProjectShareTests)
#include "projectshare_tests.moc"
