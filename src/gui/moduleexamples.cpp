#include "moduleexamples.hpp"
#include "paths.hpp"
#include <QFile>
using namespace thinkercad;
bool configureModuleExample(const QString& id,SavedProject& project){
    if(!QStringList{"pico_bme280","s3_mpu6050","ultrasonic_distance","neopixel_strip","uart_pair"}.contains(id))return false;
    auto& doc=project.circuit;project.mode=ExecutionMode::Emulation;
    auto module=[&](const char* model,double x=420,double y=80){const auto part=doc.addPart(PartKind::Module,x,y);doc.part(part).model=model;return part;};
    auto power=[&](const std::string& part,const char* rail){doc.connect({"board",rail},{part,"VCC"},"#dc3545");doc.connect({"board","GND"},{part,"GND"},"#64748b");};
    if(id=="pico_bme280"||id=="s3_mpu6050"){
        const bool pico=id=="pico_bme280";doc.part("board").model=pico?"rp2040":"esp32-s3";const auto sensor=module(pico?"bme280":"mpu6050");power(sensor,"3V3");const auto profile=BoardProfile::get(doc.part("board").model);doc.connect({"board",profile.pinName(pico?0:8)},{sensor,"SDA"});doc.connect({"board",profile.pinName(pico?1:9)},{sensor,"SCL"});
    }else if(id=="ultrasonic_distance"){
        doc.part("board").model="arduino-uno";const auto sensor=module("hcsr04");power(sensor,"5V");doc.connect({"board","D2"},{sensor,"TRIG"});doc.connect({"board","D3"},{sensor,"ECHO"});
    }else if(id=="neopixel_strip"){
        doc.part("board").model="arduino-uno";const auto strip=module("ws2812");doc.part(strip).parameters["pixels"]=8;power(strip,"5V");doc.connect({"board","D6"},{strip,"DIN"});
    }else{
        doc.part("board").model="esp32";const auto receiver=doc.addPart(PartKind::Esp8266,550,80);doc.part(receiver).model="esp32";doc.connect({"board","GPIO17"},{receiver,"GPIO16"});doc.connect({receiver,"GPIO17"},{"board","GPIO16"});doc.connect({"board","GND"},{receiver,"GND"});
        QFile file(AppPaths::examples()+"/uart_receiver.ino");if(!file.open(QIODevice::ReadOnly))throw std::runtime_error("UART receiver example missing");SketchProject sketch;sketch.name="uart_receiver";sketch.order={"uart_receiver.ino"};sketch.files[sketch.order.front()]=QString::fromUtf8(file.readAll());project.boardSketches["board"]=project.sketch;project.boardSketches[QString::fromStdString(receiver)]=sketch;
    }
    return true;
}
