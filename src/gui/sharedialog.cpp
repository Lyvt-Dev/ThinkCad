#include "sharedialog.hpp"
#include "qrscanner.hpp"
#include "language.hpp"
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QImageReader>
#include <QJsonObject>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>
#include <memory>

namespace {
QString t(const char *value) {
    return Language::text(value);
}
QLabel *label(QVBoxLayout *layout, const QString &value = {}) {
    auto *result = new QLabel(value);
    result->setWordWrap(true);
    result->setTextFormat(Qt::PlainText);
    result->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(result);
    return result;
}
template <class Result, class Work, class Done> void background(QDialog *dialog, Work work, Done done) {
    struct State {
        Result result{};
        QString error;
    };
    auto state = std::make_shared<State>();
    auto *thread = QThread::create([state, work = std::move(work)]() mutable {
        try {
            state->result = work();
        } catch (const std::exception &e) {
            state->error = QString::fromUtf8(e.what());
        }
    });
    QObject::connect(thread, &QThread::finished, dialog,
                     [state, done = std::move(done)] { done(state->result, state->error); });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}
QJsonArray manifest(const QStringList &directories) {
    QJsonArray result;
    QStringList seen;
    for (const auto &directory : directories) {
        QFile properties(directory + "/library.properties");
        QString name = QFileInfo(directory).fileName(), version;
        if (properties.open(QIODevice::ReadOnly))
            for (const auto &line : properties.read(65536).split('\n')) {
                if (line.startsWith("name="))
                    name = QString::fromUtf8(line.mid(5)).trimmed();
                if (line.startsWith("version="))
                    version = QString::fromUtf8(line.mid(8)).trimmed();
            }
        if (!seen.contains(name)) {
            seen << name;
            result.append(QJsonObject{{"name", name}, {"version", version}});
        }
    }
    return result;
}
} // namespace
void showProjectShare(QWidget *parent, SavedProject project, const QStringList &directories,
                      std::function<void(bool)> onCreated) {
    QDialog dialog(parent);
    dialog.setObjectName("projectShareDialog");
    dialog.setWindowTitle(t("Projekt teilen…"));
    dialog.resize(700, 650);
    auto *layout = new QVBoxLayout(&dialog);
    label(
        layout,
        t("Der Share-Code enthält deine Schaltung und deinen Code. Ohne Passwort ist er nicht vertraulich."));
    label(layout, t("Alles wird lokal erzeugt. Bibliotheken werden als benötigte Namen und Versionen "
                    "aufgelistet und nicht eingebettet."));
    auto *form = new QFormLayout;
    auto *password = new QLineEdit;
    password->setObjectName("sharePassword");
    password->setEchoMode(QLineEdit::Password);
    password->setMaxLength(256);
    form->addRow(t("Passwort (optional)"), password);
    layout->addLayout(form);
    auto *generate = new QPushButton(t("Share-Code erzeugen"));
    generate->setObjectName("generateShareCode");
    layout->addWidget(generate);
    auto *status = label(layout);
    status->setObjectName("shareStatus");
    auto *code = new QPlainTextEdit;
    code->setObjectName("shareCode");
    code->setReadOnly(true);
    code->setLineWrapMode(QPlainTextEdit::NoWrap);
    code->setAccessibleName(t("Share-Code"));
    layout->addWidget(code, 1);
    auto *qr = label(layout);
    qr->setObjectName("shareQrPreview");
    qr->setAlignment(Qt::AlignCenter);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons->button(QDialogButtonBox::Close)->setText(t("Schließen"));
    auto *copy = buttons->addButton(t("Share-Code kopieren"), QDialogButtonBox::ActionRole);
    auto *save = buttons->addButton(t("QR-Bild speichern…"), QDialogButtonBox::ActionRole);
    copy->setEnabled(false);
    save->setEnabled(false);
    layout->addWidget(buttons);
    QImage qrImage;
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(password, &QLineEdit::textChanged, &dialog, [&] {
        code->clear();
        qr->clear();
        qrImage = {};
        copy->setEnabled(false);
        save->setEnabled(false);
    });
    QObject::connect(copy, &QPushButton::clicked, &dialog,
                     [&] { QApplication::clipboard()->setText(code->toPlainText()); });
    QObject::connect(save, &QPushButton::clicked, &dialog, [&] {
        const auto path = QFileDialog::getSaveFileName(&dialog, t("QR-Bild speichern…"), "ThinkCad-Share.png",
                                                       "PNG (*.png)");
        if (!path.isEmpty() && !qrImage.save(path, "PNG"))
            status->setText(t("QR-Bild konnte nicht gespeichert werden."));
    });
    const auto libraries = manifest(directories);
    QObject::connect(generate, &QPushButton::clicked, &dialog, [&] {
        generate->setEnabled(false);
        password->setEnabled(false);
        copy->setEnabled(false);
        save->setEnabled(false);
        code->clear();
        qr->clear();
        status->setText(t("Share-Code wird lokal vorbereitet…"));
        const bool locked = !password->text().isEmpty();
        background<QString>(
            &dialog,
            [snapshot = project, libraries, secret = password->text().toUtf8()] {
                return ProjectShare::encode(snapshot, libraries, secret);
            },
            [&, locked](const QString &value, const QString &error) {
                generate->setEnabled(true);
                password->setEnabled(true);
                if (!error.isEmpty()) {
                    status->setText(Language::text(error));
                    return;
                }
                code->setPlainText(value);
                copy->setEnabled(true);
                if (onCreated)
                    onCreated(locked);
                status->setText(t(
                    locked ? "Share-Code ist passwortgeschützt. Teile das Passwort über einen getrennten Weg."
                           : "Ohne Passwort ist der Share-Code nicht vertraulich."));
                if (value.toUtf8().size() <= ProjectShare::MaximumQrBytes) {
                    try {
                        qrImage = ProjectShare::qrImage(value);
                        qr->setPixmap(QPixmap::fromImage(qrImage).scaled(240, 240, Qt::KeepAspectRatio,
                                                                         Qt::FastTransformation));
                        save->setEnabled(true);
                    } catch (const std::exception &e) {
                        status->setText(Language::text(QString::fromUtf8(e.what())));
                    }
                } else
                    qr->setText(t("Projekt ist zu groß für einen QR-Code. Bitte den Share-Code kopieren."));
            });
    });
    dialog.exec();
}
std::optional<DecodedShare> showShareImport(QWidget *parent) {
    QDialog dialog(parent);
    dialog.setObjectName("shareImportDialog");
    dialog.setWindowTitle(t("Aus Share-Code importieren…"));
    dialog.resize(680, 580);
    auto *layout = new QVBoxLayout(&dialog);
    label(layout,
          t("Code einfügen oder ein QR-Bild öffnen. Bestehende Projektdateien werden nicht überschrieben."));
    auto *code = new QPlainTextEdit;
    code->setObjectName("importShareCode");
    code->setLineWrapMode(QPlainTextEdit::NoWrap);
    code->setAccessibleName(t("Share-Code"));
    layout->addWidget(code, 1);
    auto *image = new QPushButton(t("QR-Bild öffnen…"));
    layout->addWidget(image);
    auto *scan = new QPushButton(t("QR-Code mit Kamera scannen"));
    layout->addWidget(scan);
    auto *password = new QLineEdit;
    password->setObjectName("importSharePassword");
    password->setEchoMode(QLineEdit::Password);
    password->setMaxLength(256);
    auto *form = new QFormLayout;
    form->addRow(t("Passwort (falls erforderlich)"), password);
    layout->addLayout(form);
    auto *preview = new QPushButton(t("Importvorschau prüfen"));
    preview->setObjectName("previewShareImport");
    layout->addWidget(preview);
    auto *status = label(layout);
    status->setObjectName("shareImportPreview");
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Cancel)->setText(t("Abbrechen"));
    auto *applyImport = buttons->addButton(t("Als neues Projekt importieren"), QDialogButtonBox::AcceptRole);
    applyImport->setObjectName("confirmShareImport");
    applyImport->setEnabled(false);
    layout->addWidget(buttons);
    std::optional<DecodedShare> chosen;
    int generation = 0;
    auto changed = [&] {
        ++generation;
        chosen.reset();
        applyImport->setEnabled(false);
        status->clear();
    };
    QObject::connect(code, &QPlainTextEdit::textChanged, &dialog, changed);
    QObject::connect(password, &QLineEdit::textChanged, &dialog, changed);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (chosen)
            dialog.accept();
    });
    QObject::connect(image, &QPushButton::clicked, &dialog, [&] {
        const auto path = QFileDialog::getOpenFileName(&dialog, t("QR-Bild öffnen…"), {},
                                                       "Images (*.png *.jpg *.jpeg *.bmp)");
        if (path.isEmpty())
            return;
        try {
            QImageReader reader(path);
            const auto size = reader.size();
            if (QFileInfo(path).size() > 32 * 1024 * 1024 || !size.isValid() ||
                qint64(size.width()) * size.height() > 16000000)
                throw std::runtime_error(t("QR-Bild fehlt oder ist zu groß.").toStdString());
            code->setPlainText(ProjectShare::readQr(reader.read()));
        } catch (const std::exception &e) {
            status->setText(Language::text(QString::fromUtf8(e.what())));
        }
    });
    QObject::connect(scan, &QPushButton::clicked, &dialog, [&] {
        const auto value = scanProjectQr(&dialog);
        if (!value.isEmpty())
            code->setPlainText(value);
    });
    QObject::connect(preview, &QPushButton::clicked, &dialog, [&] {
        if (code->toPlainText().size() > ProjectShare::MaximumCodeBytes) {
            status->setText(t("Share-Code ist zu groß."));
            return;
        }
        chosen.reset();
        applyImport->setEnabled(false);
        preview->setEnabled(false);
        code->setEnabled(false);
        password->setEnabled(false);
        image->setEnabled(false);
        scan->setEnabled(false);
        status->setText(t("Share-Code wird lokal geprüft…"));
        const auto requested = generation;
        background<DecodedShare>(
            &dialog,
            [value = code->toPlainText(), secret = password->text().toUtf8()] {
                return ProjectShare::decode(value, secret);
            },
            [&, requested](const DecodedShare &result, const QString &error) {
                preview->setEnabled(true);
                code->setEnabled(true);
                password->setEnabled(true);
                image->setEnabled(true);
                scan->setEnabled(true);
                if (requested != generation)
                    return;
                if (!error.isEmpty()) {
                    status->setText(Language::text(error));
                    return;
                }
                try {
                    const auto project = ProjectIO::deserialize(result.project, false);
                    QStringList boards, libraries;
                    for (const auto &part : project.circuit.parts())
                        if (part.kind == thinkercad::PartKind::Esp8266)
                            boards << QString::fromStdString(thinkercad::BoardProfile::get(part.model).label);
                    for (const auto &value : result.libraries) {
                        const auto item = value.toObject();
                        libraries << item["name"].toString() + " " + item["version"].toString();
                    }
                    status->setText(
                        t("Projekt: %1\nBoards: %2\nBauteile: %3 · Projektformat: %4\nBenötigte "
                          "Bibliotheken: %5\nBibliotheken werden nicht automatisch installiert. Prüfe die "
                          "lokale Verfügbarkeit vor dem Start.")
                            .arg(project.sketch.name, boards.join(", "))
                            .arg(project.circuit.parts().size())
                            .arg(QJsonDocument::fromJson(result.project).object()["version"].toInt())
                            .arg(libraries.isEmpty() ? t("Keine angegeben") : libraries.join(", ")));
                    chosen = result;
                    applyImport->setEnabled(true);
                } catch (const std::exception &e) {
                    status->setText(Language::text(QString::fromUtf8(e.what())));
                }
            });
    });
    return dialog.exec() == QDialog::Accepted ? chosen : std::nullopt;
}
