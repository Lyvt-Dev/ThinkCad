#include "qrscanner.hpp"
#include "projectshare.hpp"
#include "language.hpp"
#include <QCamera>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QLabel>
#include <QPushButton>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QThread>
#include <QVideoFrame>
#include <QVideoSink>
#include <QVBoxLayout>
#include <QCoreApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QPermissions>
#endif
#include <memory>

QString scanProjectQr(QWidget *parent) {
    QDialog dialog(parent);
    dialog.setObjectName("projectQrScanner");
    dialog.setWindowTitle(Language::text("QR-Code mit Kamera scannen"));
    dialog.resize(600, 470);
    auto *layout = new QVBoxLayout(&dialog);
    auto *status = new QLabel(
        Language::text("Kamerabilder bleiben lokal und werden nicht gespeichert oder übertragen."));
    status->setTextFormat(Qt::PlainText);
    status->setWordWrap(true);
    layout->addWidget(status);
    auto *select = new QComboBox;
    select->setAccessibleName(Language::text("Kamera"));
    layout->addWidget(select);
    auto *preview = new QLabel;
    preview->setAlignment(Qt::AlignCenter);
    preview->setMinimumSize(320, 240);
    layout->addWidget(preview, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Cancel)->setText(Language::text("Abbrechen"));
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    const auto cameras = QMediaDevices::videoInputs();
    for (const auto &device : cameras)
        select->addItem(device.description());
    QCamera camera;
    QMediaCaptureSession session;
    QVideoSink sink;
    session.setCamera(&camera);
    session.setVideoSink(&sink);
    QElapsedTimer timer;
    timer.start();
    bool busy = false, permitted = false;
    QString result;
    auto start = [&] {
        if (!permitted || select->currentIndex() < 0)
            return;
        camera.stop();
        camera.setCameraDevice(cameras.at(select->currentIndex()));
        for (const auto &format : camera.cameraDevice().videoFormats())
            if (format.resolution().width() >= 640 && format.resolution().width() <= 1280 &&
                format.resolution().height() <= 720) {
                camera.setCameraFormat(format);
                break;
            }
        camera.start();
    };
    QObject::connect(select, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, [&] { start(); });
    QObject::connect(&camera, &QCamera::errorOccurred, &dialog, [&](QCamera::Error, const QString &error) {
        status->setText(Language::text("Kamera konnte nicht gestartet werden. Prüfe Zugriffsrechte oder "
                                       "öffne stattdessen ein QR-Bild.") +
                        "\n" + error);
    });
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &dialog, [&](const QVideoFrame &frame) {
        if (busy || timer.elapsed() < 250)
            return;
        timer.restart();
        const auto image = frame.toImage();
        if (image.isNull())
            return;
        preview->setPixmap(
            QPixmap::fromImage(image).scaled(480, 320, Qt::KeepAspectRatio, Qt::FastTransformation));
        struct Outcome {
            QString code, error;
        };
        auto outcome = std::make_shared<Outcome>();
        busy = true;
        auto *thread = QThread::create([outcome, image] {
            try {
                outcome->code = ProjectShare::readQr(image);
            } catch (const std::exception &e) {
                outcome->error = QString::fromUtf8(e.what());
            }
        });
        QObject::connect(thread, &QThread::finished, &dialog, [&, outcome] {
            busy = false;
            if (!outcome->code.isEmpty()) {
                result = outcome->code;
                camera.stop();
                dialog.accept();
            } else
                status->setText(outcome->error);
        });
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        thread->start();
    });
    if (cameras.isEmpty())
        status->setText(Language::text("Keine Kamera verfügbar. Öffne stattdessen ein QR-Bild."));
    else {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        QCameraPermission permission;
        const auto current = QCoreApplication::instance()->checkPermission(permission);
        if (current == Qt::PermissionStatus::Undetermined)
            QCoreApplication::instance()->requestPermission(
                permission, &dialog, [&](const QPermission &granted) {
                    if (granted.status() == Qt::PermissionStatus::Granted) {
                        permitted = true;
                        start();
                    } else
                        status->setText(Language::text(
                            "Kamerazugriff wurde nicht erlaubt. Öffne stattdessen ein QR-Bild."));
                });
        else if (current == Qt::PermissionStatus::Granted) {
            permitted = true;
            start();
        } else
            status->setText(
                Language::text("Kamerazugriff wurde nicht erlaubt. Öffne stattdessen ein QR-Bild."));
#else
        permitted = true;
        start();
#endif
    }
    dialog.exec();
    camera.stop();
    return result;
}
