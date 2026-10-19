#pragma once
#include "circuit.hpp"
#include "modules.hpp"
#include "tft.hpp"
#include <array>
#include <deque>
namespace thinkercad {
// Transaction-level device state, shared by every master using a physical part.
// Registers and conversion clocks are independent of the GUI and native SDK.
class ModuleDevices {
public:
    void reset(){states_.clear();cards_.clear();analog_.clear();}
    void setEmulation(bool value){emulation_=value;}
    void synchronizePower(const CircuitDocument&,const CircuitState&);
    std::vector<std::uint8_t> i2c(const CircuitPart&,const std::vector<std::uint8_t>& data,int count,Microseconds now);
    void select(const CircuitPart&,bool active);
    unsigned maximumSpiClock(const CircuitPart&)const;
    void receiveMidi(const CircuitPart&,const std::vector<std::uint8_t>&,Microseconds);
    void applyDisplayFrames(const CircuitDocument&,CircuitState&,Microseconds);
    void setDisplayDataMode(const std::string& id,bool value){states_[id].dataMode=value;}
    void setCodecDataMode(const std::string& id,bool mode){states_[id].dataMode=mode;}
    struct CodecChunk {std::string id;std::vector<std::uint8_t> bytes;std::uint64_t generation;};
    std::vector<CodecChunk> takeCodecChunks();
    void setDecodedAudio(const CircuitPart&,std::shared_ptr<const std::vector<float>>,unsigned rate,Microseconds start,std::uint64_t generation);
    void captureI2sInputs(const CircuitDocument&,const CircuitState&,Microseconds);
    std::vector<float> i2sRead(const CircuitDocument&,const CircuitState&,const std::string& board,int bclk,int lrck,int din,unsigned rate,std::size_t frames);
    unsigned i2sWrite(const CircuitDocument&,const CircuitState&,const std::string& board,int bclk,int lrck,int dout,unsigned rate,const std::vector<float>& pcm,Microseconds start);
    std::vector<PeripheralSignal> outputSignals(const CircuitPart&,const CircuitState&,Microseconds now=0);
    std::uint8_t spi(const CircuitPart&,std::uint8_t value,Microseconds now);
    void setCard(const std::string& id,std::vector<std::uint8_t> uid){cards_[id]=std::move(uid);}
    void setAnalog(const std::string& id,const std::array<double,8>& volts,double reference);
    std::array<std::uint8_t,256> registers(const CircuitPart&,Microseconds now);
    std::optional<std::array<std::uint8_t,256>> peekRegisters(const std::string& id)const;
    static std::uint8_t crc8(const std::vector<std::uint8_t>& bytes);
private:
    struct State {
        TftController tft;
        std::array<std::uint8_t,256> reg{};
        std::array<std::uint32_t,8> words{};
        std::uint16_t serialWord=0,control=0;
        std::shared_ptr<const std::vector<float>> pcm;Microseconds pcmStart=0;unsigned pcmRate=48000,adcRate=48000;
        std::deque<std::array<float,2>> adcPcm;double adcNext=0;
        bool upperWord=false;
        std::string model;
        std::deque<std::uint8_t> fifo;
        bool initialized=false,selected=false,command=false,reading=false,pending=false,halted=false,dataMode=false;
        struct MidiVoice {unsigned velocity=0,program=0,channel=0;Microseconds start=0,release=0;bool releasing=false,held=false;double phaseOffset=0;};
        std::map<int,MidiVoice> midiVoices;
        std::array<unsigned,16> midiProgram{},midiVolume{},midiPan{},midiMod{};
        std::array<double,16> midiBend{};std::array<bool,16> midiSustain{};
        std::vector<std::uint8_t> midiMessage;int midiStatus=-1;bool midiPadding=false,midiDirty=false,midiPlugin=false;
        std::map<std::uint16_t,std::uint16_t> wram;std::uint16_t wramAddress=0;
        std::vector<std::uint8_t> codecBytes;std::uint64_t generation=0;
        std::uint8_t pointer=0;
        int stage=0,adc=0;
        Microseconds ready=0,resetUntil=0,nextSample=0;
    };
    struct Analog{std::array<double,8> volts{};double reference=3.3;};
    std::map<std::string,State> states_;
    std::map<std::string,std::vector<std::uint8_t>> cards_;
    std::map<std::string,Analog> analog_;
    bool emulation_=false;std::uint64_t generationCounter_=0;
    State& prepare(const CircuitPart&,Microseconds now);
    void initialize(State&,const ModuleDefinition&);
    void write(State&,const CircuitPart&,std::uint8_t address,std::uint8_t value,Microseconds now);
    std::uint8_t read(State&,const ModuleDefinition&,std::uint8_t address);
    void radio(State&,const CircuitPart&);
    static void sampleBme(State&,const CircuitPart&);
    static void sampleMpu(State&,const CircuitPart&);
};
}
