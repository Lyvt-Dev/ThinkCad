#include "codeinterpreter.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <iostream>
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);QTemporaryDir library;
    const auto root=library.filePath("portable library/src");QDir().mkpath(root);
    auto write=[](const QString& path,const QByteArray& data){QFile f(path);return f.open(QIODevice::WriteOnly) && f.write(data)==data.size();};
    if(!write(root+"/Counter.h","#pragma once\nint doubled(int value);\n") || !write(root+"/Counter.cpp","#include \"Counter.h\"\n#ifndef THINKERCAD_SIMULATION\n#error Simulation define missing in library\n#endif\n\n#include <Scale.h>\nint doubled(int v){return scaled(v); }\n"))return 1;
    const auto scale=library.filePath("scale library/src"),unused=library.filePath("unused library/src");QDir().mkpath(scale);QDir().mkpath(unused);if(!write(scale+"/Scale.h","#pragma once\nint scaled(int);\n")||!write(scale+"/Scale.cpp","#include \"Scale.h\"\nint scaled(int value){return value*2;}\n")||!write(unused+"/Unused.cpp","#error Unused hardware library must not be compiled\n"))return 1;
    CodeInterpreter runner;bool wrote=false,read=false,adc=false,done=false;
    QObject::connect(&runner,&CodeInterpreter::log,&app,[](const QString& line){std::cerr<<line.toStdString()<<'\n';});
    QObject::connect(&runner,&CodeInterpreter::protocol,&app,[&](const QByteArray& line){
        if(done)return;
        if(line=="WRITE 14 1")wrote=true;
        if(line=="READ 12"){read=true;runner.answer(1);}
        if(line=="ADC"){adc=true;runner.answer(512);}
        if(line=="TIME 10000"){
            done=true;const bool ok=wrote&&read&&adc;runner.stop();std::cout<<(ok?"Portable library and GPIO/ADC round-trip passed\n":"IPC/library check failed\n");app.exit(ok?0:1);
        }
    });
    QObject::connect(&runner,&CodeInterpreter::activeChanged,&app,[&](bool active){if(!active&&!done){done=true;app.exit(1);}});
    QTimer::singleShot(20000,&app,[&]{if(!done){done=true;runner.stop();app.exit(1);}});
    QTimer::singleShot(0,&app,[&]{runner.start(
        "#include <Arduino.h>\n#include <Counter.h>\n#include <avr/pgmspace.h>\n#ifndef THINKERCAD_SIMULATION\n#include <HardwareOnlyDependency.h>\n#endif\nconst uint8_t bitmap[] PROGMEM={42};\nvoid setup(){pinMode(D5,OUTPUT);pinMode(D6,INPUT_PULLUP);digitalWrite(D5,doubled(21)==pgm_read_byte(bitmap)?HIGH:LOW);}\n"
        "void loop(){if(digitalRead(D6)!=HIGH || analogRead(A0)!=512) std::abort();delay(10);}\n",
        "g++",QCoreApplication::applicationDirPath()+"/sdk",{library.filePath("portable library"),library.filePath("scale library"),library.filePath("unused library")});});
    return app.exec();
}
