#include "thinkercad/circuit.hpp"
#include "soundoutput.hpp"
#include <QCoreApplication>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
namespace {int checks=0;void check(bool condition,const char* message){++checks;if(!condition)throw std::runtime_error(message);}}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CircuitDocument doc;doc.part("board").model="arduino-uno";
    Esp8266Board board;board.configure("arduino-uno");
    std::map<std::string,const Esp8266Board*> sources{{"board",&board}};
    const auto buzzer=doc.addPart(PartKind::PiezoBuzzer,0,0);
    board.tone(3,1000);check(doc.soundSignals(sources).empty(),"Disconnected buzzer is silent");
    doc.connect({buzzer,"+"},{"board","D3"});doc.connect({buzzer,"-"},{"board","GND"});
    auto tones=doc.soundSignals(sources);check(tones.size()==1&&tones[0].frequency==1000&&tones[0].amplitude>0.99,"Wired piezo carries programmed pitch");
    const auto switchId=doc.addPart(PartKind::ToggleSwitch,0,0);doc.clearWires();doc.connect({buzzer,"+"},{"board","D3"});doc.connect({buzzer,"-"},{switchId,"1"});doc.connect({switchId,"2"},{"board","GND"});
    check(doc.soundSignals(sources).empty(),"Open return path is silent");doc.part(switchId).pressed=true;check(doc.soundSignals(sources).size()==1,"Closing SPST restores sound");
    board.tone(3,0);check(doc.soundSignals(sources).empty(),"noTone silences piezo");board.digitalWrite(3,true);check(doc.soundSignals(sources).empty(),"DC is silent");
    doc.clearWires();const auto speaker=doc.addPart(PartKind::Speaker,0,0);doc.connect({speaker,"+"},{"board","D3"});doc.connect({speaker,"-"},{"board","GND"});board.tone(3,440);
    check(!doc.solve(sources).errors.empty(),"8-ohm speaker directly on GPIO reports overcurrent");check(doc.soundSignals(sources).empty(),"Invalid circuit is silent");
    doc.clearWires();const auto r=doc.addPart(PartKind::Resistor,0,0);doc.part(r).value=470;doc.connect({speaker,"+"},{r,"1"});doc.connect({r,"2"},{"board","D3"});doc.connect({speaker,"-"},{"board","GND"});
    check(doc.solve(sources).errors.empty(),"Speaker with 470-ohm resistor stays within GPIO limit");tones=doc.soundSignals(sources);check(tones.size()==1&&tones[0].amplitude>0&&tones[0].amplitude<1,"Series resistor attenuates speaker signal");
    const auto level=tones[0].amplitude;doc.part(r).value=1000;check(doc.soundSignals(sources)[0].amplitude<level,"Higher series resistance reduces volume");
    doc.clearWires();doc.connect({speaker,"+"},{"board","D3"});doc.connect({speaker,"-"},{"board","D3"});check(doc.soundSignals(sources).empty(),"Both terminals on same signal produce no sound");
    doc.clearWires();doc.connect({buzzer,"-"},{"board","D3"});doc.connect({buzzer,"+"},{"board","GND"});check(doc.soundSignals(sources)[0].amplitude<0,"Reversing passive piezo reverses phase");
    for(const auto& model:{"esp8266","esp32","esp32-s3","arduino-nano"}){
        CircuitDocument circuit;circuit.part("board").model=model;Esp8266Board device;device.configure(model);
        const auto sound=circuit.addPart(PartKind::Speaker,0,0),resistor=circuit.addPart(PartKind::Resistor,0,0);circuit.part(resistor).value=470;
        device.tone(3,660);circuit.connect({"board",device.profile().pinName(3)},{resistor,"1"});circuit.connect({resistor,"2"},{sound,"+"});circuit.connect({sound,"-"},{"board","GND"});
        const std::map<std::string,const Esp8266Board*> inputs{{"board",&device}};
        check(circuit.solve(inputs).errors.empty()&&circuit.soundSignals(inputs).size()==1,"Speaker with current limiting works on every board profile");
        device.pinMode(3,PinMode::Input);check(circuit.soundSignals(inputs).empty(),"Input mode cancels tone");
    }
    CircuitDocument duet;duet.part("board").model="arduino-uno";const auto second=duet.addPart(PartKind::Esp8266,0,0);duet.part(second).model="arduino-nano";
    Esp8266Board firstBoard,secondBoard;firstBoard.configure("arduino-uno");secondBoard.configure("arduino-nano");firstBoard.tone(3,440);secondBoard.tone(3,880);
    for(const auto& boardId:{std::string("board"),second}){const auto sound=duet.addPart(PartKind::PiezoBuzzer,0,0);duet.connect({boardId,"D3"},{sound,"+"});duet.connect({sound,"-"},{boardId,"GND"});}
    const auto duetSignals=duet.soundSignals({{"board",&firstBoard},{second,&secondBoard}});
    check(duetSignals.size()==2&&duetSignals[0].frequency!=duetSignals[1].frequency,"Separate board sketches retain independent simultaneous pitches");
    SoundStream stream;stream.setSignals({{"test",1000,1}});auto data=stream.read(48000*2);check(data.size()==96000,"One second of PCM produced");
    int crossings=0;double energy=0,mean=0;qint16 previous=0;
    for(int i=0;i<data.size()/2;++i){qint16 sample;std::memcpy(&sample,data.constData()+i*2,2);if(previous<=0&&sample>0)++crossings;previous=sample;energy+=double(sample)*sample;mean+=sample;}
    check(std::abs(crossings-1000)<=1,"Synthesized PCM has actual 1-kHz pitch");check(energy/data.size()>1000000,"PCM contains audible signal energy");check(std::abs(mean/48000)<2,"Tone has no DC bias");
    stream.setSignals({});const auto silence=stream.read(4096);check(std::all_of(silence.begin(),silence.end(),[](char c){return c==0;}),"Stop generates silence");
    for(auto sampleFormat:{QAudioFormat::Int16,QAudioFormat::Int32,QAudioFormat::Float,QAudioFormat::UInt8}){
        QAudioFormat format;format.setSampleRate(44100);format.setChannelCount(2);format.setSampleFormat(sampleFormat);stream.configure(format);stream.setSignals({{"stereo",440,1}});const auto stereo=stream.read(100*format.bytesPerFrame());
        check(stereo.size()==100*format.bytesPerFrame(),"Preferred device sample format is supported");for(int i=0;i<100;++i)check(std::memcmp(stereo.constData()+i*format.bytesPerFrame(),stereo.constData()+i*format.bytesPerFrame()+format.bytesPerSample(),format.bytesPerSample())==0,"Mono mix duplicated across stereo channels");
    }
    std::cout<<checks<<" electrical audio and PCM checks passed\n";
    if(app.arguments().contains("--backend")){
        SoundOutput sound;QObject::connect(&sound,&SoundOutput::notice,[](const QString& message){std::cerr<<message.toStdString()<<"\n";});sound.setEnabled(true);sound.prepare();
        QTimer::singleShot(2500,&app,[&]{
            sound.setSignals({{"backend",440,1}});
            QTimer::singleShot(500,&app,[&]{sound.setSignals({});QTimer::singleShot(2500,&app,[&]{sound.stop();app.quit();});});
        });
        return app.exec();
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
