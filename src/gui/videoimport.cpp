#include "videoimport.hpp"
#include "language.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <algorithm>
#include <exception>
#include <stdexcept>
VideoImportDialog::VideoImportDialog(const QString& source,QWidget* parent):QDialog(parent),source_(source){
#if defined(Q_OS_LINUX) && QT_VERSION < QT_VERSION_CHECK(6,5,0)
    // Qt 6.4 probes graphics capabilities lazily even with raster widgets.
    // Do this before playback: first-frame EGL initialization can exceed
    // GStreamer's render deadline on cold/headless Wayland systems.
    static const bool readbackReady=[] {
        QVideoFrame frame(QVideoFrameFormat(QSize(2,2),QVideoFrameFormat::Format_YUV420P));
        if(frame.map(QVideoFrame::WriteOnly)){
            for(int plane=0;plane<frame.planeCount();++plane)std::fill_n(frame.bits(plane),frame.mappedBytes(plane),uchar(0));
            frame.unmap();(void)frame.toImage();
        }
        return true;
    }();
    Q_UNUSED(readbackReady);
#endif
    setObjectName("videoImportDialog");setWindowTitle(Language::text("Video in frames.h umwandeln"));resize(540,520);
    auto* layout=new QVBoxLayout(this);layout->setContentsMargins(20,20,20,20);layout->setSpacing(14);
    auto* file=new QLabel(QFileInfo(source).fileName());file->setTextFormat(Qt::PlainText);file->setWordWrap(true);file->setToolTip(source);layout->addWidget(file);
    auto* description=new QLabel(Language::text("Monochrome OLED-Frames für Adafruit GFX. Seitenverhältnis bleibt erhalten; Audio wird nicht exportiert."));description->setWordWrap(true);layout->addWidget(description);
    auto* form=new QFormLayout;resolution_=new QComboBox;resolution_->setObjectName("videoResolution");resolution_->addItem("128 × 64",QSize(128,64));resolution_->addItem("128 × 32",QSize(128,32));form->addRow(Language::text("Auflösung"),resolution_);
    fps_=new QSpinBox;fps_->setObjectName("videoFps");fps_->setRange(1,30);fps_->setValue(10);form->addRow(Language::text("Bilder pro Sekunde"),fps_);
    start_=new QDoubleSpinBox;start_->setObjectName("videoStart");start_->setDecimals(2);start_->setRange(0,86400);form->addRow(Language::text("Startzeit (Sekunden)"),start_);
    duration_=new QDoubleSpinBox;duration_->setObjectName("videoDuration");duration_->setDecimals(2);duration_->setRange(.01,300);duration_->setValue(10);form->addRow(Language::text("Clipdauer (Sekunden)"),duration_);
    threshold_=new QSpinBox;threshold_->setRange(0,255);threshold_->setValue(128);form->addRow(Language::text("Schwarz-Weiß-Schwelle"),threshold_);
    invert_=new QCheckBox(Language::text("Farben umkehren"));form->addRow(invert_);layout->addLayout(form);
    preview_=new QLabel;preview_->setObjectName("videoPreview");preview_->setFixedSize(256,128);preview_->setAlignment(Qt::AlignCenter);preview_->setFrameShape(QFrame::StyledPanel);layout->addWidget(preview_,0,Qt::AlignHCenter);
    status_=new QLabel(Language::text("Video wird geladen…"));status_->setObjectName("videoStatus");status_->setTextFormat(Qt::PlainText);status_->setWordWrap(true);layout->addWidget(status_);
    progress_=new QProgressBar;progress_->setRange(0,100);layout->addWidget(progress_);
    auto* actions=new QHBoxLayout;actions->addStretch();generate_=new QPushButton(Language::text("frames.h exportieren…"));generate_->setObjectName("exportVideoFrames");generate_->setEnabled(false);actions->addWidget(generate_);auto* cancel=new QPushButton(Language::text("Schließen"));actions->addWidget(cancel);layout->addLayout(actions);
    connect(cancel,&QPushButton::clicked,this,&VideoImportDialog::reject);connect(generate_,&QPushButton::clicked,this,&VideoImportDialog::begin);
    for(auto* spin:{fps_,threshold_})connect(spin,qOverload<int>(&QSpinBox::valueChanged),this,[this]{updateOptions();});
    for(auto* spin:{start_,duration_})connect(spin,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{updateOptions();});
    connect(resolution_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{updateOptions();});
    player_.setVideoSink(&sink_); // No audio output: conversion is silent.
    connect(&player_,&QMediaPlayer::hasVideoChanged,this,[this](bool){updateOptions();});
    connect(&player_,&QMediaPlayer::positionChanged,this,[this](qint64){if(converting_&&!preview_->pixmap().isNull())watchdog_.start();});
    connect(&player_,&QMediaPlayer::durationChanged,this,[this](qint64){updateOptions();});
    connect(&player_,&QMediaPlayer::mediaStatusChanged,this,[this](QMediaPlayer::MediaStatus state){if(state==QMediaPlayer::LoadedMedia)updateOptions();if(state==QMediaPlayer::EndOfMedia&&converting_)complete();});
    connect(&player_,&QMediaPlayer::errorOccurred,this,[this](QMediaPlayer::Error,const QString& message){fail(Language::text("Video konnte nicht gelesen werden: ")+message);});
    connect(&sink_,&QVideoSink::videoFrameChanged,this,[this](const QVideoFrame& frame){
        if(!converting_||!frame.isValid())return;
        const auto image=frame.toImage();if(image.isNull())return;
        watchdog_.start();const auto timestamp=frame.startTime()>=0?frame.startTime():player_.position()*1000;
        try{frames_->append(timestamp,image);const auto bitmap=VideoFrames::bitmap(image,options());QImage mono(options().width,options().height,QImage::Format_RGB32);const int stride=(mono.width()+7)/8;for(int y=0;y<mono.height();++y)for(int x=0;x<mono.width();++x)mono.setPixel(x,y,(static_cast<unsigned char>(bitmap[y*stride+x/8])&(0x80u>>(x%8)))?qRgb(255,255,255):qRgb(0,0,0));preview_->setPixmap(QPixmap::fromImage(mono).scaled(preview_->size(),Qt::KeepAspectRatio));progress_->setValue(frames_->collected()*100/frames_->count());if(frames_->complete()||timestamp>=endUs_)complete();}
        catch(const std::exception& error){fail(Language::text(QString::fromUtf8(error.what())));}
    });
    watchdog_.setSingleShot(true);watchdog_.setInterval(15000);connect(&watchdog_,&QTimer::timeout,this,[this]{fail(Language::text("Video-Decoder antwortet nicht."));});
    player_.setSource(QUrl::fromLocalFile(source));
}
VideoFrameOptions VideoImportDialog::options()const{
    VideoFrameOptions value;const auto size=resolution_->currentData().toSize();value.width=size.width();value.height=size.height();value.fps=fps_->value();value.threshold=threshold_->value();value.invert=invert_->isChecked();value.startUs=qRound64(start_->value()*1000000);value.durationUs=qRound64(duration_->value()*1000000);return value;
}
void VideoImportDialog::updateOptions(){
    if(converting_)return;
    const double available=player_.duration()/1000.;
    if(available<=0){generate_->setEnabled(false);return;}
    if(!player_.hasVideo()){status_->setText(Language::text("Die ausgewählte Datei enthält keine Videospur."));generate_->setEnabled(false);return;}
    start_->setMaximum(std::max(0.,available-.01));duration_->setMaximum(std::min(300.,std::max(.01,available-start_->value())));
    try{VideoFrames check(options());status_->setText(Language::text("%1 Frames · %2 KiB Bitmapdaten · maximal 256 KiB").arg(check.count()).arg(check.count()*((options().width+7)/8)*options().height/1024.0,0,'f',1));generate_->setEnabled(true);}
    catch(const std::exception& error){status_->setText(Language::text(QString::fromUtf8(error.what())));generate_->setEnabled(false);}
}
void VideoImportDialog::begin(){
    const auto output=QFileDialog::getSaveFileName(this,Language::text("frames.h exportieren"),QFileInfo(source_).absolutePath()+"/frames.h","C++ (*.h)");if(output.isEmpty())return;
    exportTo(output);
}
bool VideoImportDialog::exportTo(const QString& output){
    if(converting_||output.isEmpty()||player_.duration()<=0||!player_.hasVideo())return false;
    try{frames_=std::make_unique<VideoFrames>(options());}catch(const std::exception& error){fail(Language::text(QString::fromUtf8(error.what())));return false;}
    output_=output;endUs_=options().startUs+options().durationUs;converting_=true;
    for(auto* widget:findChildren<QWidget*>())if(qobject_cast<QSpinBox*>(widget)||qobject_cast<QDoubleSpinBox*>(widget)||qobject_cast<QComboBox*>(widget)||qobject_cast<QCheckBox*>(widget))widget->setEnabled(false);
    generate_->setEnabled(false);progress_->setValue(0);status_->setText(Language::text("Video wird umgewandelt…"));
    player_.setPosition(options().startUs/1000);watchdog_.start();player_.play();return true;
}
void VideoImportDialog::complete(){
    if(!converting_)return;converting_=false;watchdog_.stop();player_.stop();
    try{frames_->finish();const auto data=frames_->header();QSaveFile file(output_);if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit())throw std::runtime_error("Could not save frames.h.");progress_->setValue(100);status_->setText(Language::text("frames.h wurde exportiert: ")+output_);for(auto* widget:findChildren<QWidget*>())if(qobject_cast<QSpinBox*>(widget)||qobject_cast<QDoubleSpinBox*>(widget)||qobject_cast<QComboBox*>(widget)||qobject_cast<QCheckBox*>(widget))widget->setEnabled(true);generate_->setEnabled(true);emit exported(output_);}
    catch(const std::exception& error){fail(Language::text(QString::fromUtf8(error.what())));}
}
void VideoImportDialog::fail(const QString& message){converting_=false;watchdog_.stop();player_.stop();status_->setText(message);for(auto* widget:findChildren<QWidget*>())if(qobject_cast<QSpinBox*>(widget)||qobject_cast<QDoubleSpinBox*>(widget)||qobject_cast<QComboBox*>(widget)||qobject_cast<QCheckBox*>(widget))widget->setEnabled(true);generate_->setEnabled(false);}
void VideoImportDialog::reject(){converting_=false;watchdog_.stop();player_.stop();QDialog::reject();}

#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
bool videoImportSmokeTest(const QString& source,QString* error){
    QTemporaryDir directory;VideoImportDialog dialog(source);QEventLoop loop;QTimer poll,timeout;bool done=false;
    const auto target=directory.filePath("frames.h");
    QObject::connect(&dialog,&VideoImportDialog::exported,&loop,[&]{done=true;loop.quit();});
    poll.setInterval(20);QObject::connect(&poll,&QTimer::timeout,&loop,[&]{if(dialog.exportTo(target))poll.stop();});
    timeout.setSingleShot(true);QObject::connect(&timeout,&QTimer::timeout,&loop,&QEventLoop::quit);
    poll.start();timeout.start(10000);loop.exec();
    QFile result(target);const bool valid=done&&result.open(QIODevice::ReadOnly)&&result.readAll().contains("FRAME_COUNT = 10;");
    if(!valid&&error)*error=dialog.findChild<QLabel*>("videoStatus")->text();return valid;
}
