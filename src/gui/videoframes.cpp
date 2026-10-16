#include "videoframes.hpp"
#include <QPainter>
#include <algorithm>
#include <stdexcept>
VideoFrames::VideoFrames(VideoFrameOptions options):options_(options){
    if(options.width<1||options.width>320||options.height<1||options.height>240||options.fps<1||options.fps>30||options.threshold<0||options.threshold>255||options.startUs<0||options.startUs>86400000000LL||options.durationUs<=0||options.durationUs>300000000LL)throw std::invalid_argument("Invalid video frame settings");
    count_=int((options.durationUs*options.fps+999999)/1000000);
    const qint64 bytes=qint64((options.width+7)/8)*options.height*count_;
    if(bytes>maximumBytes)throw std::length_error("Video exceeds the 256 KiB bitmap limit. Reduce frame rate, clip length or resolution.");
    frames_.reserve(count_);
}
QByteArray VideoFrames::bitmap(const QImage& source,const VideoFrameOptions& options){
    if(source.isNull()||options.width<1||options.width>320||options.height<1||options.height>240)throw std::invalid_argument("Invalid video frame");
    QImage image(options.width,options.height,QImage::Format_RGB32);image.fill(Qt::black);
    const auto scaled=source.scaled(image.size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    {QPainter painter(&image);painter.drawImage((image.width()-scaled.width())/2,(image.height()-scaled.height())/2,scaled);}
    const int stride=(options.width+7)/8;QByteArray result(stride*options.height,'\0');
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){
        const bool on=(qGray(image.pixel(x,y))>=options.threshold)!=options.invert;
        if(on)result[y*stride+x/8]=char(static_cast<unsigned char>(result[y*stride+x/8])|(0x80u>>(x%8)));
    }
    return result;
}
qint64 VideoFrames::nextTime()const{return options_.startUs+qint64(frames_.size())*1000000/options_.fps;}
void VideoFrames::append(qint64 timestamp,const QImage& image){
    if(complete()||timestamp<0||timestamp<previousTime_||image.isNull())return;
    const auto current=bitmap(image,options_);
    if(previous_.isEmpty())previous_=current;
    while(!complete()&&nextTime()<timestamp)frames_.append(previous_);
    previous_=current;previousTime_=timestamp;
    if(!complete()&&nextTime()==timestamp)frames_.append(current);
}
void VideoFrames::finish(){
    if(previous_.isEmpty())throw std::runtime_error("The video did not produce any readable frames.");
    while(!complete())frames_.append(previous_);
}
QByteArray VideoFrames::header()const{
    if(!complete())throw std::logic_error("Video conversion is incomplete.");
    QByteArray result="#pragma once\n#include <Arduino.h>\n\n// Monochrome, row-major, MSB first. Adafruit_GFX::drawBitmap compatible.\n";
    result+="static constexpr uint16_t FRAME_WIDTH = "+QByteArray::number(options_.width)+";\n";
    result+="static constexpr uint16_t FRAME_HEIGHT = "+QByteArray::number(options_.height)+";\n";
    result+="static constexpr uint16_t FRAME_FPS = "+QByteArray::number(options_.fps)+";\n";
    result+="static constexpr uint32_t FRAME_COUNT = "+QByteArray::number(frames_.size())+";\n";
    result+="static constexpr uint32_t FRAME_BYTES = "+QByteArray::number(frames_.front().size())+";\n\n";
    result+="static const uint8_t frames[FRAME_COUNT][FRAME_BYTES] PROGMEM = {\n";
    for(const auto& frame:frames_){result+="    {\n        ";int column=0;for(unsigned char byte:frame){result+="0x";constexpr char hex[]="0123456789abcdef";result+=hex[byte>>4];result+=hex[byte&15];result+=',';if(++column%16==0)result+="\n        ";}result+="\n    },\n";}
    result+="};\n\n// #include \"frames.h\"\n// uint32_t index = (uint64_t(micros()) * FRAME_FPS / 1000000) % FRAME_COUNT;\n// display.clearDisplay();\n// display.drawBitmap(0, 0, frames[index], FRAME_WIDTH, FRAME_HEIGHT, SSD1306_WHITE);\n// display.display();\n";
    return result;
}
