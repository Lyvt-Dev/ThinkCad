#pragma once
#include <QDialog>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QTimer>
#include <memory>
#include "videoframes.hpp"
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QProgressBar;
class VideoImportDialog final:public QDialog {
    Q_OBJECT
public:
    explicit VideoImportDialog(const QString& source,QWidget* parent=nullptr);
    void reject()override;
    bool exportTo(const QString& output);
signals:
    void exported(const QString& path);
private:
    QMediaPlayer player_;
    QVideoSink sink_;
    QTimer watchdog_;
    QComboBox* resolution_;
    QSpinBox* fps_;
    QSpinBox* threshold_;
    QDoubleSpinBox* start_;
    QDoubleSpinBox* duration_;
    QCheckBox* invert_;
    QLabel* status_;
    QLabel* preview_;
    QPushButton* generate_;
    QProgressBar* progress_;
    std::unique_ptr<VideoFrames> frames_;
    QString output_,source_;
    bool converting_=false;
    qint64 endUs_=0;
    void updateOptions();
    VideoFrameOptions options()const;
    void begin();
    void complete();
    void fail(const QString& message);
};
// Exercises the deployed video backend without showing a window.
bool videoImportSmokeTest(const QString& source,QString* error=nullptr);
