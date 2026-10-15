#include "codeinterpreter.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTimer>
#include <iostream>
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);CodeInterpreter runner;QElapsedTimer elapsed;
    bool done=false,switched=false; qint64 slowMs=0;
    runner.setSimulationSpeed(0.1);
    QObject::connect(&runner,&CodeInterpreter::log,&app,[](const QString& line){std::cerr<<line.toStdString()<<'\n';});
    QObject::connect(&runner,&CodeInterpreter::activeChanged,&app,[&](bool active){if(!active&&!done){done=true;app.exit(1);}});
    QObject::connect(&runner,&CodeInterpreter::protocol,&app,[&](const QByteArray& line){
        if(done)return;
        if(line=="READY")elapsed.start();
        if(line=="TIME 200000"&&!switched){slowMs=elapsed.elapsed();switched=true;runner.setSimulationSpeed(10);elapsed.restart();}
        if(line=="TIME 1000000"){
            const auto fastMs=elapsed.elapsed();const bool ok=switched&&slowMs>=1400&&fastMs<slowMs/2&&runner.simulationSpeed()==10;
            done=true;runner.stop();std::cout<<"Live pacing: first 200ms virtual at 0.1x took "<<slowMs<<"ms; next 800ms at 10x took "<<fastMs<<"ms\n";app.exit(ok?0:1);
        }
    });
    QTimer::singleShot(25000,&app,[&]{if(!done){done=true;runner.stop();app.exit(1);}});
    QTimer::singleShot(0,&app,[&]{runner.start("void setup(){}\nvoid loop(){delay(1000);}\n","g++",QCoreApplication::applicationDirPath()+"/sdk",{});});
    return app.exec();
}
