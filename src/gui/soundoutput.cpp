#include "soundoutput.hpp"
#include "language.hpp"
#include <QAudioDevice>
#include <QAudioSink>
#include <QMediaDevices>
#include <QSettings>
#include <algorithm>
#include <cmath>
#include <cstring>
SoundStream::SoundStream(QObject* parent):QIODevice(parent){
    format_.setSampleRate(48000);format_.setChannelCount(1);format_.setSampleFormat(QAudioFormat::Int16);
    open(QIODevice::ReadOnly|QIODevice::Unbuffered);
}
void SoundStream::configure(QAudioFormat format){std::lock_guard lock(mutex_);format_=format;voices_.clear();pcm_.clear();pcmPhase_=dc_=0;pcmMode_=false;}
void SoundStream::setSignals(const std::vector<thinkercad::SoundSignal>& tones){
    std::unique_lock lock(mutex_);std::vector<Voice> next;
    for(const auto& signal:tones){
        if(!std::isfinite(signal.frequency)||!std::isfinite(signal.amplitude)||signal.frequency<=0||signal.frequency>20000)continue;
        const auto old=std::find_if(voices_.begin(),voices_.end(),[&](const auto& v){return v.signal.id==signal.id;});
        next.push_back({signal,old==voices_.end()?0:old->phase});
    }
    pcmMode_=false;pcm_.clear();voices_=std::move(next);
    lock.unlock();emit readyRead();
}
void SoundStream::clearPcm(){std::lock_guard lock(mutex_);pcm_.clear();pcmMode_=false;pcmPhase_=dc_=0;}
void SoundStream::appendPcm(const std::vector<float>& samples){{std::lock_guard lock(mutex_);pcmMode_=true;voices_.clear();for(float value:samples)pcm_.push_back(std::isfinite(value)?std::clamp(value,-1.f,1.f):0);while(pcm_.size()>12000)pcm_.pop_front();}emit readyRead();}
qint64 SoundStream::bytesAvailable()const{std::lock_guard lock(mutex_);return format_.bytesForDuration(100000)+QIODevice::bytesAvailable();}
qint64 SoundStream::readData(char* data,qint64 maxSize){
    std::lock_guard lock(mutex_);const int stride=format_.bytesPerFrame();if(stride<=0)return 0;
    const qint64 frames=maxSize/stride;
    for(qint64 i=0;i<frames;++i){
        double sample=0;
        for(auto& voice:voices_){
            // Additive, band-limited square wave: preserve tone harmonics without aliasing.
            double wave=0;for(int harmonic=1;harmonic<=31&&harmonic*voice.signal.frequency<format_.sampleRate()*0.45;harmonic+=2)
                wave+=std::sin(6.283185307179586*voice.phase*harmonic)/harmonic;
            sample+=wave*(4/3.141592653589793)*voice.signal.amplitude*0.2;
            voice.phase=std::fmod(voice.phase+voice.signal.frequency/format_.sampleRate(),1.0);
        }
        if(pcmMode_){sample=pcm_.empty()?0:pcm_.front();dc_+=.001*(sample-dc_);sample-=dc_;pcmPhase_+=48000./format_.sampleRate();while(pcmPhase_>=1){if(!pcm_.empty())pcm_.pop_front();pcmPhase_-=1;}}
        sample=std::clamp(sample,-0.9,0.9);
        for(int channel=0;channel<format_.channelCount();++channel){
            char* target=data+i*stride+channel*format_.bytesPerSample();
            switch(format_.sampleFormat()){
            case QAudioFormat::Int16:{const qint16 value=static_cast<qint16>(sample*32767);std::memcpy(target,&value,sizeof(value));break;}
            case QAudioFormat::Int32:{const qint32 value=static_cast<qint32>(sample*2147483647);std::memcpy(target,&value,sizeof(value));break;}
            case QAudioFormat::Float:{const float value=static_cast<float>(sample);std::memcpy(target,&value,sizeof(value));break;}
            case QAudioFormat::UInt8:*target=static_cast<char>(std::clamp(int(128+sample*127),0,255));break;
            default:std::memset(target,0,format_.bytesPerSample());break;
            }
        }
    }
    return frames*stride;
}
SoundOutput::SoundOutput(QObject* parent):QObject(parent){
    enabled_=QSettings().value("soundEnabled",true).toBool();
    pumpTimer_.setInterval(10);pumpTimer_.setTimerType(Qt::PreciseTimer);
    connect(&pumpTimer_,&QTimer::timeout,this,&SoundOutput::pump);
}
SoundOutput::~SoundOutput(){stop();}
void SoundOutput::setEnabled(bool enabled){enabled_=enabled;QSettings().setValue("soundEnabled",enabled);if(!enabled)stop();}
// reset discards queued PCM immediately; stop can drain it and delay note endings.
void SoundOutput::stop(){pumpTimer_.stop();device_=nullptr;pending_.clear();if(sink_)sink_->reset();stream_.setSignals({});stream_.clearPcm();}
void SoundOutput::pump(){
    if(!sink_||!device_)return;
    if(pending_.isEmpty()){
        const qint64 queued=std::max<qint64>(0,sink_->bufferSize()-sink_->bytesFree());
        const qint64 needed=format_.bytesForDuration(80000)-queued;
        qint64 bytes=std::min<qint64>({needed,sink_->bytesFree(),format_.bytesForDuration(20000)});
        bytes-=bytes%format_.bytesPerFrame();if(bytes<=0)return;
        pending_=stream_.read(bytes);
    }
    const auto written=device_->write(pending_);if(written>0)pending_.remove(0,written);
}
void SoundOutput::prepare(){
    if(!enabled_)return;
    if(!sink_){
        const auto device=QMediaDevices::defaultAudioOutput();
        if(device.isNull()){
            if(!warned_){warned_=true;emit notice(Language::text("Kein Audioausgabegerät verfügbar. Simulation läuft ohne Ton."));}return;
        }
        QAudioFormat format;format.setSampleRate(48000);format.setChannelCount(1);format.setSampleFormat(QAudioFormat::Int16);
        if(!device.isFormatSupported(format))format=device.preferredFormat();
        format_=format;stream_.configure(format);sink_=std::make_unique<QAudioSink>(device,format);sink_->setVolume(0.5);
        sink_->setBufferSize(format.bytesForDuration(100000));
        connect(sink_.get(),&QAudioSink::stateChanged,this,[this](QAudio::State state){
            if(state==QAudio::StoppedState&&sink_->error()!=QAudio::NoError&&!warned_){warned_=true;emit notice(Language::text("Audioausgabe fehlgeschlagen. Audiogerät prüfen."));}
        });
    }
    if(sink_->state()==QAudio::StoppedState){device_=sink_->start();if(device_){pumpTimer_.start();pump();}}
}
void SoundOutput::setSignals(const std::vector<thinkercad::SoundSignal>& tones){
    if(!enabled_){stop();return;}
    // Keep the device clock running through rests, avoiding backend startup per note.
    if(tones.empty()){stream_.setSignals({});return;}
    prepare();if(!sink_)return;
    stream_.setSignals(tones);
}

void SoundOutput::appendPcm(const std::vector<float>& samples){if(!enabled_){stop();return;}prepare();if(sink_)stream_.appendPcm(samples);}
