#pragma once
#include <QAudioDecoder>
#include <QAudioBuffer>
#include <QBuffer>
#include <QTimer>
#include <QString>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>
// Decode an uploaded VS1053 stream with Qt's deployed multimedia backend.
// A new reset generation cancels pending input and prevents stale playback.
class AudioCodecDecoder:public QObject {
    struct Stream {
        QByteArray encoded;std::vector<float> pcm;std::uint64_t generation=0;
        unsigned rate=48000;std::size_t deliveredSamples=0;
        std::unique_ptr<QTimer> timer;
        std::unique_ptr<QBuffer> buffer;
        std::unique_ptr<QAudioDecoder> decoder;
    };
    std::map<std::string,std::unique_ptr<Stream>> streams_;
    void decode(const std::string& id);
public:
    explicit AudioCodecDecoder(QObject* parent=nullptr):QObject(parent){}
    std::function<void(const std::string&,std::shared_ptr<const std::vector<float>>,unsigned,std::uint64_t)> decoded;
    std::function<void(const QString&)> notice;
    void append(const std::string&,const std::vector<std::uint8_t>&,std::uint64_t generation);
    void clear(){streams_.clear();}
};
