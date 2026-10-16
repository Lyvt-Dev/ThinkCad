#pragma once
#include <QByteArray>
#include <QImage>
#include <QVector>
#include <QString>
#include <QtGlobal>
struct VideoFrameOptions {
    int width=128,height=64,fps=10,threshold=128;
    bool invert=false;
    qint64 startUs=0,durationUs=10000000;
};
class VideoFrames {
public:
    static constexpr qsizetype maximumBytes=256*1024;
    explicit VideoFrames(VideoFrameOptions options);
    static QByteArray bitmap(const QImage& image,const VideoFrameOptions& options);
    void append(qint64 timestampUs,const QImage& image);
    void finish();
    bool complete()const{return frames_.size()==count_;}
    int count()const{return count_;}
    int collected()const{return frames_.size();}
    QByteArray header()const;
private:
    VideoFrameOptions options_;
    int count_=0;
    QVector<QByteArray> frames_;
    QByteArray previous_;
    qint64 previousTime_=-1;
    qint64 nextTime()const;
};
