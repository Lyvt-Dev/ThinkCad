#pragma once
#include "thinkercad/circuit.hpp"
#include <QAudioFormat>
#include <QIODevice>
#include <QObject>
#include <QTimer>
#include <memory>
#include <mutex>
class QAudioSink;
// Pull-based PCM source, also usable without an audio device in tests.
class SoundStream final:public QIODevice {
public:
    explicit SoundStream(QObject* parent=nullptr);
    void configure(QAudioFormat format);
    void setSignals(const std::vector<thinkercad::SoundSignal>& tones);
    bool isSequential()const override{return true;}
    bool reset()override{return true;}
    qint64 bytesAvailable()const override;
protected:
    qint64 readData(char* data,qint64 maxSize)override;
    qint64 writeData(const char*,qint64)override{return -1;}
private:
    struct Voice {thinkercad::SoundSignal signal;double phase=0;};
    mutable std::mutex mutex_;
    QAudioFormat format_;
    std::vector<Voice> voices_;
};
class SoundOutput final:public QObject {
    Q_OBJECT
public:
    explicit SoundOutput(QObject* parent=nullptr);
    ~SoundOutput()override;
    void setSignals(const std::vector<thinkercad::SoundSignal>& tones);
    void prepare();
    void setEnabled(bool enabled);
    void stop();
signals:
    void notice(const QString& message);
private:
    void pump();
    SoundStream stream_;
    QTimer pumpTimer_;
    QIODevice* device_=nullptr;
    QByteArray pending_;
    QAudioFormat format_;
    std::unique_ptr<QAudioSink> sink_;
    bool enabled_=true,warned_=false;
};
