#include <stdexcept>
#include "peripheralhub.hpp"
#include <cmath>
#include <QList>
#include <set>
#include <algorithm>
#include <QString>
using namespace thinkercad;
namespace {
bool supply(const CircuitState& s,const std::string& id,double low,double high){auto v=s.voltage.find({id,"VCC"}),g=s.voltage.find({id,"GND"});return v!=s.voltage.end()&&g!=s.voltage.end()&&v->second&&g->second&&*v->second-*g->second>=low-1e-6&&*v->second-*g->second<=high+1e-6;}
bool near(const CircuitPart& a,const CircuitPart& b,double range){return parameter(a,"channel",0)==parameter(b,"channel",0)&&std::hypot(a.x-b.x,a.y-b.y)<=range;}
QByteArray tagMemory(const CircuitPart& card,const QByteArray* overrideText=nullptr){QByteArray memory(160,0);const int length=int(parameter(card,"uidLength",7));const int defaults[]={4,161,178,195,212,229,246};unsigned char uid[7]{};for(int i=0;i<length;++i)uid[i]=static_cast<unsigned char>(parameter(card,"uid"+std::to_string(i),defaults[i]));for(int i=0;i<3;++i)memory[i]=char(uid[i]);memory[3]=char(0x88^uid[0]^uid[1]^uid[2]);for(int i=3;i<7;++i)memory[i+1]=char(uid[i]);memory[8]=char(uid[3]^uid[4]^uid[5]^uid[6]);memory[9]=0x48;memory[12]=char(0xe1);memory[13]=0x10;memory[14]=0x12;auto text=overrideText?*overrideText:card.model.empty()?QByteArray("ThinkCad NFC test card"):QByteArray::fromStdString(card.model);QByteArray record;record.append(char(0xd1));record.append(char(1));record.append(char(text.size()+3));record.append('T');record.append(char(2));record.append("en");record.append(text);QByteArray tlv;tlv.append(char(3));tlv.append(char(record.size()));tlv.append(record);tlv.append(char(0xfe));memory.replace(16,tlv.size(),tlv);return memory;}
}
void PeripheralHub::transmit(const CircuitDocument& doc,const CircuitState& state,const std::vector<std::pair<std::string,double>>& emitters,const thinkir::Wave& wave,Microseconds now){
 bool traced=false;
 for(auto& [key,d]:devices_)if(d.type=="IR_RX"){
  auto it=std::find_if(doc.parts().begin(),doc.parts().end(),[&](const auto& p){return p.id==d.id;});if(it==doc.parts().end())continue;const auto& rx=*it;
  thinkir::Optics o;o.range=parameter(rx,"irRange",1400);o.hz=unsigned(parameter(rx,"irFrequency",38000));o.tolerance=parameter(rx,"irTolerance",.12);o.ambient=parameter(rx,"irAmbient",0);double total=0;std::string source;
  for(const auto& [id,current]:emitters){const auto& tx=doc.part(id);if(parameter(tx,"channel",0)!=parameter(rx,"channel",0))continue;thinkir::Optics contribution=o;contribution.distance=std::hypot(tx.x-rx.x,tx.y-rx.y);contribution.current=current;contribution.intensity=parameter(tx,"irIntensity",1);contribution.sensitivity=parameter(rx,"irSensitivity",1);contribution.ambient=0;contribution.lineOfSight=!parameter(tx,"irBlocked",0)&&!parameter(rx,"irBlocked",0);if(contribution.lineOfSight&&contribution.distance<=contribution.range){total+=thinkir::strength(contribution);if(!source.empty())source+=", ";source+=id;}}
  o.current=total*.0095;if(parameter(rx,"irFidelity",0)){o.minimumCycles=10;o.minimumStrength=.25;o.tolerance=std::min(o.tolerance,.1);}
  auto capture=thinkir::demodulate(wave,o);bool powered=supply(state,rx.id,2.7,5.5);const bool readable=!d.pins.empty()&&state.level({d.board,BoardProfile::get(doc.part(d.board).model).pinName(d.pins[0])})==Level::High;auto decoded=capture&&powered&&readable?thinkir::decode(*capture):std::nullopt;
  std::string status=!powered?"Receiver unpowered":!capture?"Optical/carrier rejection":!readable?"Receiver output cannot reach GPIO HIGH":!decoded?"Invalid pulse frame":"Decoded";
  if(capture&&powered)for(const auto& trace:traces_)if(!trace.received.pulses.empty()&&trace.receiver==d.id&&now<trace.start+trace.emitted.duration()&&trace.start<now+wave.duration()){decoded.reset();d.lastIr.reset();status="Overlapping optical transmissions";std::erase_if(d.infrared,[&](const auto& frame){return frame.ready>now;});break;}
  if(decoded&&decoded->repeat){if(!d.lastIr||d.lastIr->protocol!=thinkir::Nec||now<d.lastIrAt||now-d.lastIrAt>150000){decoded.reset();status="Repeat without recent full NEC frame";}else{auto previous=*d.lastIr;previous.repeat=true;previous.bits=0;previous.raw=0;decoded=previous;}}
  if(decoded&&!decoded->repeat&&decoded->protocol!=thinkir::Nec&&d.lastIr&&decoded->protocol==d.lastIr->protocol&&decoded->raw==d.lastIr->raw&&now>=d.lastIrAt&&now-d.lastIrAt<200000)decoded->repeat=true;
  if(decoded&&d.listening&&d.infrared.empty()){d.lastIr=*decoded;d.lastIrAt=now;d.infrared.push_back({decoded->address,decoded->command,(decoded->repeat?1:0)|((decoded->protocol==thinkir::Rc5&&(decoded->raw&(1ull<<11)))||(decoded->protocol==thinkir::Rc6&&(decoded->raw&(1ull<<16)))?8:0),*decoded,*capture,now+wave.duration()});}
  traced=true;if(traces_.size()==64)traces_.pop_front();traces_.push_back({source,rx.id,status,now,wave,capture&&powered?*capture:thinkir::Wave{},thinkir::strength(o),decoded});
 }
 if(!traced){std::string source;for(const auto& [id,current]:emitters){(void)current;if(!source.empty())source+=", ";source+=id;}if(traces_.size()==64)traces_.pop_front();traces_.push_back({source,"","No initialized receiver",now,wave,{},0});}

}
void PeripheralHub::infrared(const CircuitDocument& doc,const std::string& source,int address,int command,int flags){
 // The test remote is an optical waveform source, not a direct decoder message.
 auto wave=thinkir::encode(thinkir::Nec,std::uint16_t(address),std::uint16_t(command),flags!=0);
 Esp8266Board board;board.configure(doc.part(doc.boardId()).model);auto state=doc.solve(board);transmit(doc,state,{{source,.0095}},wave,now_);
}
std::map<std::string,DigitalWaveform> PeripheralHub::irSignals()const{
 std::map<std::string,DigitalWaveform> waveforms;
 for(const auto& [key,d]:devices_)if(d.type=="IR_RX"){
  auto captures=std::make_shared<std::vector<std::pair<Microseconds,thinkir::Wave>>>();for(const auto& trace:traces_)if(trace.receiver==d.id&&!trace.received.pulses.empty())captures->push_back({trace.start,trace.received});
  DigitalWaveform signal;signal.high=[captures](Microseconds now){for(const auto& [start,wave]:*captures)if(now>=start){auto elapsed=now-start;for(auto p:wave.pulses){if(elapsed<p.us){if(p.mark)return false;break;}elapsed-=p.us;}}return true;};
  signal.nextEdge=[captures](Microseconds now){Microseconds next=0;for(const auto& [start,wave]:*captures){auto at=start;if(at>now&&(!next||at<next))next=at;for(auto pulse:wave.pulses){at+=pulse.us;if(at>now&&(!next||at<next))next=at;}}return next;};waveforms[d.id]=std::move(signal);
 }return waveforms;
}
std::optional<bool> PeripheralHub::irLevel(const std::string& board,int pin,Microseconds now)const{
 for(const auto& [key,d]:devices_)if(d.board==board&&d.type=="IR_RX"&&!d.pins.empty()&&d.pins[0]==pin){
  for(auto t=traces_.rbegin();t!=traces_.rend();++t)if(t->receiver==d.id&&now>=t->start&&now<t->start+t->received.duration()){auto elapsed=now-t->start;for(auto p:t->received.pulses){if(elapsed<p.us)return !p.mark;elapsed-=p.us;}}
  return true;
 }return {};
}
void PeripheralHub::inputs(const CircuitDocument& doc,std::optional<Microseconds> now){if(now)now_=*now;for(const auto& p:doc.parts())if(p.kind==PartKind::InfraredRemote){if(p.pressed&&!pressed_[p.id]){infrared(doc,p.id,int(parameter(p,"address",0)),int(p.value));remoteNext_[p.id]=now_+110000;}else if(p.pressed&&now_>=remoteNext_[p.id]){infrared(doc,p.id,int(parameter(p,"address",0)),int(p.value),1);remoteNext_[p.id]=now_+110000;}pressed_[p.id]=p.pressed;}}
bool PeripheralHub::handle(const std::string& boardId,Esp8266Board& board,const CircuitDocument& doc,const CircuitState& state,Microseconds now,const QByteArray& line,const Answer& answer){
    now_=now;
    const auto fields=line.split(' ');const auto op=fields[0];if(!op.startsWith("IR_")&&!op.startsWith("NFC_")&&!op.startsWith("RF_")&&!op.startsWith("ASK_"))return false;
    auto number=[&](int index){bool ok=false;int n=index<fields.size()?fields[index].toInt(&ok):0;if(!ok)throw std::runtime_error("Invalid peripheral command: "+line.toStdString());return n;};
    const int token=number(1);auto key=std::make_pair(boardId,token);

    auto ready=[&](const Device& d){if(std::none_of(doc.parts().begin(),doc.parts().end(),[&](const auto& p){return p.id==d.id;})||std::none_of(doc.parts().begin(),doc.parts().end(),[&](const auto& p){return p.id==d.board;}))return false;auto kind=doc.part(d.id).kind;if(d.type=="NFC"&&kind!=PartKind::NfcReader&&!(kind==PartKind::Module&&doc.part(d.id).model=="pn532-transmitter"))return false;if(doc.part(d.board).kind!=PartKind::Esp8266)return false;const auto profile=BoardProfile::get(doc.part(d.board).model);for(int pin:d.pins)if(std::find(profile.gpio.begin(),profile.gpio.end(),pin)==profile.gpio.end())return false;double min=kind==PartKind::RadioTransceiver?1.9:2.7,max=kind==PartKind::RadioTransceiver?3.6:5.5;if(kind==PartKind::InfraredLed)return true;if(!supply(state,d.id,min,max))return false;std::vector<std::string> names=d.type=="NFC"?std::vector<std::string>{"SDA","SCL","IRQ","RST"}:d.type=="RF"?std::vector<std::string>{"SCK","MISO","MOSI","CSN","CE"}:std::vector<std::string>{d.type=="IR_RX"?"OUT":"DATA"};for(std::size_t i=0;i<names.size();++i)if(!doc.connected({d.id,names[i]},{d.board,BoardProfile::get(doc.part(d.board).model).pinName(d.pins[i])}))return false;if(d.type=="NFC"){auto r=state.voltage.find({d.id,"RST"});auto v=state.voltage.find({d.id,"VCC"});auto g=state.voltage.find({d.id,"GND"});if(r==state.voltage.end()||!r->second||*r->second-*g->second<.7*(*v->second-*g->second))return false;}return true;};
    // A lost supply, reset, disconnected bus or deleted owner ends target mode.
    // Reapplying power cannot silently resurrect a reader's old selection.
    std::erase_if(targets_,[&](const auto& entry){auto owner=devices_.find(entry.second.owner);return owner==devices_.end()||owner->second.id!=entry.first||!ready(owner->second);});
    if(op=="IR_RX_BEGIN"||op=="NFC_BEGIN"||op=="RF_BEGIN"||op=="ASK_TX_BEGIN"||op=="ASK_RX_BEGIN"){
        Device d;d.board=boardId;d.type=op=="IR_RX_BEGIN"?"IR_RX":op=="NFC_BEGIN"?"NFC":op=="RF_BEGIN"?"RF":op=="ASK_TX_BEGIN"?"ASK_TX":"ASK_RX";
        int count=d.type=="NFC"?4:d.type=="RF"?5:1;if(fields.size()!=count+2)throw std::runtime_error("Invalid peripheral pin count");std::set<int> unique;for(int i=0;i<count;++i){int pin=number(i+2);(void)board.pin(pin);unique.insert(pin);d.pins.push_back(pin);}if(int(unique.size())!=count)throw std::runtime_error("Peripheral pins must be distinct");
        PartKind kind=d.type=="NFC"?PartKind::NfcReader:d.type=="RF"?PartKind::RadioTransceiver:d.type=="IR_RX"?PartKind::InfraredReceiver:d.type=="ASK_TX"?PartKind::RadioTransmitter:PartKind::RadioReceiver;std::vector<std::string> matches;for(const auto& p:doc.parts())if(p.kind==kind||(d.type=="NFC"&&p.kind==PartKind::Module&&p.model=="pn532-transmitter")){d.id=p.id;if(ready(d))matches.push_back(p.id);}if(matches.size()!=1){answer(0);return true;}d.id=matches.front();d.channel=int(doc.part(d.id).value);d.listening=d.type=="ASK_RX"||d.type=="IR_RX";if(d.type=="NFC")targets_.erase(d.id);devices_[key]=d;answer(1);return true;
    }
    if(op=="IR_TX_BEGIN") {int pin=number(2);(void)board.pin(pin);Device d;d.board=boardId;d.type="IR_TX";d.pins={pin};auto probe=board;probe.pinMode(pin,PinMode::Output);probe.digitalWrite(pin,true);auto on=doc.solve({{boardId,&probe}});probe.digitalWrite(pin,false);auto off=doc.solve({{boardId,&probe}});for(const auto& p:doc.parts())if(p.kind==PartKind::InfraredLed){auto a=on.ledCurrent.find(p.id),b=off.ledCurrent.find(p.id);if(a!=on.ledCurrent.end()&&a->second>=.001&&a->second<=.05&&(b==off.ledCurrent.end()||b->second<.0001)&&on.errors.empty())d.emitters.push_back(p.id);}if(d.emitters.empty()){answer(0);return true;}d.id=d.emitters.front();devices_[key]=d;answer(1);return true;}
    auto found=devices_.find(key);if(found==devices_.end())throw std::runtime_error("Peripheral not initialized: check module, supply and bus wiring");auto& d=found->second;if(!ready(d)){if(op=="NFC_TARGET_ACTIVE"){answer(0);return true;}throw std::runtime_error(d.id+": peripheral supply or bus wiring invalid");}
    if(op.startsWith("NFC_")&&d.type!="NFC")throw std::runtime_error("NFC command requires a PN532 binding");
    auto selectedMemory=[&](bool writing)->QByteArray*{
        if(targets_.contains(d.id)||d.card.empty())return nullptr;
        auto part=std::find_if(doc.parts().begin(),doc.parts().end(),[&](const auto& p){return p.id==d.card;});
        if(part==doc.parts().end()||!near(*part,doc.part(d.id),320))return nullptr;
        auto target=targets_.find(d.card);
        if(target!=targets_.end()){
            if(target->second.generation!=d.cardGeneration||(writing&&!target->second.writable))return nullptr;
            return &target->second.memory;
        }
        if(d.cardGeneration||part->kind!=PartKind::NfcTestCard||!part->pressed)return nullptr;
        return &tagMemory_.try_emplace(d.card,tagMemory(*part)).first->second;
    };
    if(op=="IR_SEND"||op=="IR_WAVE"){
     if(d.type!="IR_TX")throw std::runtime_error("IR sender not initialized");if(board.pin(d.pins[0]).mode!=PinMode::Output){answer(0);return true;}thinkir::Wave wave;
     if(op=="IR_SEND")wave=thinkir::encode(thinkir::Nec,std::uint16_t(number(2)),std::uint16_t(number(3)),fields.size()>4&&number(4));
     else{const int hz=number(2),duty=number(3),count=number(4);if(hz<20000||hz>60000||duty<100||duty>600||count<1||count>256||fields.size()!=count+6)throw std::runtime_error("Invalid IR waveform header");if(number(5)!=0&&number(5)!=1)throw std::runtime_error("Invalid IR initial level");wave.hz=unsigned(hz);wave.duty=duty/1000.;for(int i=0;i<count;++i){int us=number(i+6);if(us<1||us>20000)throw std::runtime_error("Invalid IR duration");thinkir::pulse(wave,(i%2==0)==bool(number(5)),unsigned(us));}if(wave.duration()>250000)throw std::runtime_error("IR frame exceeds 250 ms capture");}
     auto probe=board;probe.pinMode(d.pins[0],PinMode::Output);probe.digitalWrite(d.pins[0],true);auto on=doc.solve({{boardId,&probe}});probe.digitalWrite(d.pins[0],false);auto off=doc.solve({{boardId,&probe}});std::vector<std::pair<std::string,double>> emitters;
     for(const auto& led:doc.parts())if(led.kind==PartKind::InfraredLed){auto current=on.ledCurrent.find(led.id),low=off.ledCurrent.find(led.id);if(current!=on.ledCurrent.end()&&current->second>=.001&&current->second<=.05&&(low==off.ledCurrent.end()||low->second<.0001)&&on.errors.empty()){emitters.emplace_back(led.id,current->second);}}
     if(!emitters.empty()){transmit(doc,on,emitters,wave,now);board.scheduleWaveform(d.pins[0],now+wave.duration(),wave.hz,wave.duty,[wave,now](Microseconds at){return at>=now&&wave.carrier(at-now);},[wave,now](Microseconds at){if(at<now)return now;auto edge=wave.nextEdge(at-now);return edge?now+edge:0;});}answer(!emitters.empty()?1:0);
    }
    else if(op=="IR_DECODE"||op=="IR_CAPTURE"){
     if(d.type!="IR_RX")throw std::runtime_error("IR receiver not initialized");
     if(d.held||d.infrared.empty()||d.infrared.front().ready>now)answer(0);else{d.held=true;const auto& f=d.infrared.front();answer(1);answer(f.address);answer(f.command);answer(f.flags);
      if(op=="IR_CAPTURE"){answer(int(f.decoded.protocol));answer(int(f.decoded.bits));for(int i=0;i<4;++i)answer(int((f.decoded.raw>>(i*16))&65535));answer(int(f.wave.pulses.size()+1));answer(0);for(auto p:f.wave.pulses)answer(int((p.us+25)/50));}
     }
    }
    else if(op=="IR_CONTROL"){if(d.type!="IR_RX"||(number(2)!=0&&number(2)!=1))throw std::runtime_error("Invalid IR receiver control");d.listening=number(2)!=0;d.infrared.clear();d.held=false;answer(1);}
    else if(op=="IR_IDLE"){bool busy=false;for(const auto& t:traces_)busy|=t.receiver==d.id&&now>=t.start&&now<t.start+t.received.duration();answer(busy?0:1);}
    else if(op=="IR_RESUME"){if(!d.infrared.empty()&&(d.held||d.infrared.front().ready<=now))d.infrared.pop_front();d.held=false;answer(1);}
    else if(op=="NFC_TARGET_BEGIN"){
        if(fields.size()!=5)throw std::runtime_error("Target emulation needs UID, NDEF text and a writable flag");
        const auto uidHex=fields[2],uid=QByteArray::fromHex(uidHex);
        const auto text=fields[3]=="-"?QByteArray{}:QByteArray::fromHex(fields[3]);int writable=number(4);
        if((uid.size()!=4&&uid.size()!=7)||uidHex.size()!=uid.size()*2||uid.toHex()!=uidHex.toLower()||text.size()>96||(fields[3]!="-"&&(fields[3].size()!=text.size()*2||text.toHex()!=fields[3].toLower()))||QString::fromUtf8(text).toUtf8()!=text||(writable!=0&&writable!=1))throw std::runtime_error("Invalid synthetic NFC target UID, UTF-8 text or writable flag");
        CircuitPart card;card.kind=PartKind::NfcTestCard;card.model=text.toStdString();card.parameters["uidLength"]=uid.size();for(int i=0;i<uid.size();++i)card.parameters["uid"+std::to_string(i)]=static_cast<unsigned char>(uid[i]);
        targets_[d.id]={key,uid,tagMemory(card,&text),bool(writable),++targetGeneration_};
        for(auto& [otherKey,other]:devices_)if(other.id==d.id){other.card.clear();other.cardGeneration=0;}
        answer(1);
    }
    else if(op=="NFC_TARGET_STOP"){targets_.erase(d.id);answer(1);}
    else if(op=="NFC_TARGET_ACTIVE"){answer(targets_.contains(d.id)?1:0);}
    else if(op=="NFC_POLL"){
        d.card.clear();d.cardGeneration=0;
        if(!targets_.contains(d.id)){
            std::vector<std::string> cards;
            for(const auto& part:doc.parts())if(part.kind==PartKind::NfcTestCard&&part.pressed&&near(part,doc.part(d.id),320))cards.push_back(part.id);
            for(const auto& [id,target]:targets_)if(id!=d.id&&near(doc.part(id),doc.part(d.id),320))cards.push_back(id);
            if(cards.size()==1)d.card=cards.front();
        }
        if(d.card.empty())answer(0);
        else if(auto target=targets_.find(d.card);target!=targets_.end()){d.cardGeneration=target->second.generation;answer(target->second.uid.size());for(unsigned char byte:target->second.uid)answer(byte);}
        else{const auto& card=doc.part(d.card);int len=int(parameter(card,"uidLength",7));const int defaults[]={4,161,178,195,212,229,246};answer(len);for(int i=0;i<len;++i)answer(int(parameter(card,"uid"+std::to_string(i),defaults[i])));}
    }
    else if(op=="NFC_PAGE"||op=="NFC_TARGET_PAGE"){
        int page=number(2);if(fields.size()!=3||page<0||page>39)throw std::runtime_error("Test NFC tag has pages 0..39");
        auto target=targets_.find(d.id);auto* memory=op=="NFC_PAGE"?selectedMemory(false):target!=targets_.end()?&target->second.memory:nullptr;
        if(!memory)answer(0);else{answer(4);for(int i=0;i<4;++i)answer(static_cast<unsigned char>((*memory)[page*4+i]));}
    }
    else if(op=="NFC_WRITE_PAGE"||op=="NFC_TARGET_WRITE"){
        const int page=number(2);const auto hex=fields.value(3),bytes=QByteArray::fromHex(hex);
        if(fields.size()!=4||page<4||page>39||hex.size()!=8||bytes.size()!=4||bytes.toHex()!=hex.toLower())throw std::runtime_error("NTAG writes require user page 4..39 and exactly four bytes");
        auto target=targets_.find(d.id);auto* memory=op=="NFC_WRITE_PAGE"?selectedMemory(true):target!=targets_.end()?&target->second.memory:nullptr;
        if(!memory)answer(0);else{memory->replace(page*4,4,bytes);answer(1);}
    }
    else if(op=="RF_CONFIG"){int channel=number(2),rate=number(3),crc=number(4),ack=number(5);if(channel<0||channel>125||(rate!=250&&rate!=1000&&rate!=2000)||crc<0||crc>2||(ack!=0&&ack!=1))throw std::runtime_error("Invalid nRF24 configuration");d.channel=channel;d.rate=rate;d.crc=crc;d.ack=ack;answer(1);}
    else if(op=="RF_PIPE"||op=="RF_DEST"){int i=op=="RF_PIPE"?number(2):-1;auto hex=fields.value(op=="RF_PIPE"?3:2);auto address=QByteArray::fromHex(hex);if(hex.size()!=10||address.size()!=5||address.toHex()!=hex.toLower()||i>5||i< -1)throw std::runtime_error("nRF24 uses a five-byte address and pipes 0..5");if(i<0)d.destination=address;else d.pipes[i]=address;answer(1);}
    else if(op=="RF_LISTEN"){d.listening=number(2)!=0;answer(1);}
    else if(op=="RF_SEND"){auto hex=fields.value(2),bytes=QByteArray::fromHex(hex);if(bytes.isEmpty()||bytes.size()>32||bytes.toHex()!=hex.toLower()||d.destination.size()!=5||d.listening)throw std::runtime_error("nRF24 transmit needs 1..32 bytes, a destination and stopListening()");bool delivered=false;for(auto& [otherKey,receiver]:devices_)if(otherKey!=key&&receiver.type=="RF"&&receiver.listening&&receiver.channel==d.channel&&receiver.rate==d.rate&&receiver.crc==d.crc&&ready(receiver)){for(const auto& [pipe,address]:receiver.pipes)if(address==d.destination){if(receiver.packets.size()<3){receiver.packets.push_back({bytes,now+Microseconds((bytes.size()+9)*8000/d.rate+130),pipe});delivered|=receiver.ack;}break;}}answer(!d.ack||delivered?1:0);}
    else if(op=="RF_AVAILABLE"){if(d.packets.empty()||d.packets.front().ready>now)answer(0);else{answer(d.packets.front().bytes.size());answer(d.packets.front().pipe);}}
    else if(op=="RF_READ"){if(d.packets.empty()||d.packets.front().ready>now)answer(0);else{auto bytes=d.packets.front().bytes;d.packets.pop_front();answer(bytes.size());for(unsigned char c:bytes)answer(c);}}
    else if(op=="ASK_SEND"){int bits=number(2),protocol=number(3);auto bytes=fields.value(4);if(bits<1||bits>32||protocol!=1||bytes.isEmpty())throw std::runtime_error("ASK adapter supports rc-switch protocol 1, 1..32 bits");for(auto& [otherKey,receiver]:devices_)if(otherKey!=key&&receiver.type=="ASK_RX"&&std::abs(doc.part(receiver.id).value-doc.part(d.id).value)<.01&&near(doc.part(receiver.id),doc.part(d.id),3000)&&ready(receiver)){if(receiver.packets.size()==8)receiver.packets.pop_front();receiver.packets.push_back({QByteArray::number(bits)+":"+bytes,now+10000,1});}answer(1);}
    else if(op=="ASK_READ"){if(d.packets.empty()||d.packets.front().ready>now)answer(0);else{auto split=d.packets.front().bytes.split(':');d.packets.pop_front();bool ok=false;auto value=split[1].toUInt(&ok,16);if(!ok)throw std::runtime_error("Invalid ASK frame");answer(split[0].toInt());answer(int(value&65535));answer(int(value>>16));}}
    else throw std::runtime_error("Unsupported peripheral command: "+line.toStdString());
    return true;
}
