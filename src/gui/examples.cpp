#include "language.hpp"
#include "examples.hpp"
#include "paths.hpp"
#include <QCoreApplication>
#include <QFile>
#include <stdexcept>
#include <algorithm>
using namespace thinkercad;
QList<CircuitExample> ExampleCatalog::entries(){return {
    {"keypad_matrix",Language::text("Keypad: 4×4-Matrix scannen"),Language::text("Uno D2–D5: Zeilen; D6–D9: Spalten. Tastenfenster öffnen und gedrückt halten.")},
    {"midi_keyboard",Language::text("MIDI: 25-Tasten-Keyboard"),Language::text("Elektrische 4×8-Matrix an D2–D13. MIDI-Noten aus dem Sketch im MIDI-Tab; keine direkte MIDI-Injektion nötig.")},
    {"dj_controller",Language::text("DJ: Encoder, Fader und MIDI-Pad"),Language::text("Uno mit Quadratur-Encoder, analogem Fader und Taster. MIDI-CC und Note On/Off im MIDI-Tab ansehen und übersetzen.")},
    {"ir_remote",Language::text("IR: TV-Fernbedienung testen"),Language::text("ESP32, 38-kHz-Empfänger und NEC-Testfernbedienung. Taste während der Simulation gedrückt halten.")},
    {"ir_loopback",Language::text("IR: Senden und Empfangen"),Language::text("GPIO2 mit 220 Ω und IR-LED; 38-kHz-Empfänger an GPIO4. NEC-Kommandos im seriellen Monitor.")},
    {"nfc_reader",Language::text("NFC: PN532 und Testkarte"),Language::text("ESP32 mit PN532 über I²C. Synthetische UID und NDEF lesen; Karte während der Simulation mit einem Klick entfernen oder auflegen.")},
    {"nrf24_pair",Language::text("Funk: nRF24L01 Sender und Empfänger"),Language::text("Zwei ESP32 mit nRF24L01, eigenen Sketches und ACK-Paketen auf Kanal 76. SPI18/19/23, CSN5, CE16.")},
    {"ask_pair",Language::text("Funk: 433-MHz-ASK-Paar"),Language::text("ESP32: Sender GPIO25, Empfänger GPIO26; rc-switch Protokoll 1 mit 16-Bit-Testnachrichten.")},
    {"lcd_parallel",Language::text("LCD 16×2: LiquidCrystal, 4 Bit"),Language::text("Uno: RS=D12, E=D11, D4…D7=D5,D4,D3,D2. VCC=5V, GND/RW/VO=GND.")},
    {"lcd_i2c",Language::text("LCD 20×4: I²C-Backpack"),Language::text("Uno: SDA=A4, SCL=A5, Adresse 0x27. LiquidCrystal_I2C mit Text und eigenem Zeichen.")},
    {"servo_sweep",Language::text("Servo: Winkel steuern"),Language::text("Servo.h steuert SIGNAL an Uno D9. Versorgung 5 V/GND; Winkel pendelt zwischen 15° und 165°.")},
    {"dc_motor",Language::text("Motor: Anlauf und Gegen-EMK"),Language::text("5-V-Quelle und DC-Motor: Strom sinkt mit steigender Drehzahl. Stromsonde in Serie.")},
    {"analog_rc",Language::text("Analog: RC-Tiefpass"),Language::text("Rechteckgenerator, 1 kΩ, 10 µF, zwei Scope-Kanäle und Stromsonde. Läuft ohne Mikrocontroller.")},
    {"analog_rl",Language::text("Analog: RL-Stromverlauf"),Language::text("Rechteckgenerator, 100 Ω und 100 mH: Spannung und verzögerter Spulenstrom.")},
    {"analog_opamp",Language::text("Analog: Spannungsfolger"),Language::text("Operationsverstärker mit 5-V-Versorgung und Gegenkopplung. Eingang und Ausgang im Oszilloskop vergleichen.")},
    {"analog_npn",Language::text("Analog: NPN-Schalter"),Language::text("Rechtecksignal über Basiswiderstand steuert einen NPN mit Kollektorlast. Das Ausgangssignal ist invertiert.")},
    {"pwm_filter",Language::text("Arduino: PWM und RC-Filter"),Language::text("analogWrite(D5, 128): 980 Hz, Scope, 8-Kanal-Logikanalysator und RC-Tiefpass.")},
    {"sound_melody",Language::text("Melodie mit Piezo und Lautsprecher"),Language::text("Arduino D3: Piezo direkt, 8-Ω-Lautsprecher über 470 Ω. tone() spielt Noten, noTone() erzeugt Pausen.")},
    {"tmp36_temperature",Language::text("Temperatur messen (TMP36)"),Language::text("TMP36 an Arduino 5V/GND/A0. Temperatur im Eigenschaftenfeld ändern; über 20 °C leuchtet die LED.")},
    {"animation",Language::text("OLED-Animation"),Language::text("ESP8266 und SSD1306: zwei Bitmap-Frames aus PROGMEM mit realistischer I²C-Transferzeit.")},
    {"led_blink",Language::text("LED blinken lassen"),Language::text("Externe LED an D5 mit 220-Ω-Vorwiderstand. Mit dem Temporegler die Blinkgeschwindigkeit vergleichen.")},
    {"button_led",Language::text("Taster schaltet LED"),Language::text("Taster an D6 gegen GND, interner Pullup, LED an D5. Taster während der Simulation gedrückt halten oder Auswahl-Checkbox verwenden.")},
    {"traffic_light",Language::text("Ampelschaltung"),Language::text("Drei LEDs an D5/D6/D7 mit eigenen Vorwiderständen: Rot, Rot/Gelb, Grün, Gelb.")},
    {"led_chaser",Language::text("Lauflicht"),Language::text("Vier LEDs an D1/D2/D5/D6 mit jeweils 220 Ω werden der Reihe nach eingeschaltet.")},
    {"analog_threshold",Language::text("Analoger Schwellwert"),Language::text("0–1-V-Quelle an A0. Ab ADC-Wert 512 leuchtet die LED. Simulation stoppen, Quelle auswählen und Spannung verändern.")},
    {"esp32_blink",Language::text("ESP32: LED blinken"),Language::text("ESP32-Boardprofil, externe LED an GPIO5 mit Vorwiderstand.")},
    {"esp32_s3_adc","ESP32-S3: ADC",Language::text("12-Bit-ADC an A0/GPIO1. Eine Quelle mit 1,65 V schaltet die LED an GPIO5 ein.")},
    {"oled_text_32",Language::text("OLED-Text 128×32"),Language::text("setCursor, setTextColor und Textgröße auf einem korrekt verkabelten 128×32-SSD1306.")},
    {"two_oleds",Language::text("Zwei OLEDs am I²C-Bus"),Language::text("Zwei Displays teilen D2/SDA und D1/SCL. Adressen 0x3C und 0x3D; Flash-Bitmaps bewegen sich gegenläufig.")}
};}
SavedProject ExampleCatalog::create(const QString& id){
    const auto entries=ExampleCatalog::entries();if(std::none_of(entries.begin(),entries.end(),[&](const auto& entry){return entry.id==id;}))throw std::invalid_argument("Unbekanntes Beispiel.");
    QFile file(AppPaths::examples()+"/"+id+".ino");if(!file.open(QIODevice::ReadOnly))throw std::runtime_error(Language::text("Beispieldatei fehlt. Installation überprüfen.").toStdString());
    SavedProject result;auto& doc=result.circuit;result.sketch.name=id;result.sketch.order={id+".ino"};result.sketch.files[id+".ino"]=QString::fromUtf8(file.readAll());
    auto led=[&](const std::string& pin,double y,const std::string& color){
        const auto resistor=doc.addPart(PartKind::Resistor,400,y),light=doc.addPart(PartKind::Led,650,y);doc.part(light).color=color;
        doc.connect({"board",pin},{resistor,"1"},color);doc.connect({resistor,"2"},{light,"A"},color);doc.connect({light,"K"},{"board","GND"},"#64748b");
    };
    auto oled=[&](double x,double y,int address){const auto display=doc.addPart(PartKind::Oled,x,y);doc.part(display).value=address;
        doc.connect({"board","3V3"},{display,"VCC"},"#ec5967");doc.connect({"board","GND"},{display,"GND"},"#64748b");doc.connect({"board","GPIO5"},{display,"SCL"},"#e9ad33");doc.connect({"board","GPIO4"},{display,"SDA"},"#378bcb");};
    auto power=[&](const std::string& board,const std::string& module){doc.connect({board,"3V3"},{module,"VCC"},"#ef4444");doc.connect({board,"GND"},{module,"GND"},"#64748b");};
    if(id=="keypad_matrix"||id=="midi_keyboard"){
        doc.part("board").model="arduino-uno";const auto input=doc.addPart(id=="keypad_matrix"?PartKind::Keypad:PartKind::Keyboard,650,100);for(int r=0;r<4;++r)doc.connect({input,"R"+std::to_string(r+1)},{"board","D"+std::to_string(2+r)});for(int c=0;c<(id=="keypad_matrix"?4:8);++c)doc.connect({input,"C"+std::to_string(c+1)},{"board","D"+std::to_string(6+c)});
    }else if(id=="dj_controller"){
        doc.part("board").model="arduino-uno";auto encoder=doc.addPart(PartKind::RotaryEncoder,650,0),fader=doc.addPart(PartKind::Potentiometer,650,220),pad=doc.addPart(PartKind::Button,650,420);for(const auto& item:std::vector<std::pair<std::string,std::string>>{{"A","D2"},{"B","D3"},{"SW","D4"},{"GND","GND"}})doc.connect({encoder,item.first},{"board",item.second});doc.connect({fader,"1"},{"board","GND"});doc.connect({fader,"2"},{"board","5V"});doc.connect({fader,"W"},{"board","A0"});doc.connect({pad,"1"},{"board","D5"});doc.connect({pad,"2"},{"board","GND"});
    }else if(id=="ir_remote"||id=="ir_loopback"){
        doc.part("board").model="esp32";auto receiver=doc.addPart(PartKind::InfraredReceiver,500,30);power("board",receiver);doc.connect({"board","GPIO4"},{receiver,"OUT"});
        if(id=="ir_remote")doc.addPart(PartKind::InfraredRemote,750,30);
        else{auto resistor=doc.addPart(PartKind::Resistor,400,270),ir=doc.addPart(PartKind::InfraredLed,640,270);doc.connect({"board","GPIO2"},{resistor,"1"});doc.connect({resistor,"2"},{ir,"A"});doc.connect({ir,"K"},{"board","GND"});}
    }else if(id=="nfc_reader"){
        doc.part("board").model="esp32";auto reader=doc.addPart(PartKind::NfcReader,480,20);power("board",reader);for(const auto& [gpio,pin]:std::vector<std::pair<std::string,std::string>>{{"GPIO21","SDA"},{"GPIO22","SCL"},{"GPIO27","IRQ"},{"GPIO26","RST"}})doc.connect({"board",gpio},{reader,pin});doc.addPart(PartKind::NfcTestCard,680,120);
    }else if(id=="nrf24_pair"){
        doc.part("board").model="esp32";auto second=doc.addPart(PartKind::Esp8266,850,0);doc.part(second).model="esp32";
        for(auto b:std::vector<std::string>{"board",second}){auto radio=doc.addPart(PartKind::RadioTransceiver,b=="board"?420:1270,10);power(b,radio);for(const auto& [gpio,pin]:std::vector<std::pair<std::string,std::string>>{{"GPIO18","SCK"},{"GPIO19","MISO"},{"GPIO23","MOSI"},{"GPIO5","CSN"},{"GPIO16","CE"}})doc.connect({b,gpio},{radio,pin});}
        QFile rx(AppPaths::examples()+"/nrf24_receiver.ino");if(!rx.open(QIODevice::ReadOnly))throw std::runtime_error("Receiver example missing");auto receiver=result.sketch;receiver.name="nrf24_receiver";receiver.order={"nrf24_receiver.ino"};receiver.files.clear();receiver.files["nrf24_receiver.ino"]=QString::fromUtf8(rx.readAll());result.boardSketches[QString::fromStdString(second)]=receiver;result.boardSketches["board"]=result.sketch;
    }else if(id=="ask_pair"){
        doc.part("board").model="esp32";auto tx=doc.addPart(PartKind::RadioTransmitter,430,20),rx=doc.addPart(PartKind::RadioReceiver,700,250);power("board",tx);power("board",rx);doc.connect({"board","GPIO25"},{tx,"DATA"});doc.connect({"board","GPIO26"},{rx,"DATA"});
    }else if(id=="lcd_parallel"||id=="lcd_i2c"){
        doc.part("board").model="arduino-uno";bool i2c=id=="lcd_i2c";const auto lcd=doc.addPart(i2c?PartKind::I2cCharacterLcd:PartKind::CharacterLcd,560,100);doc.part(lcd).model=i2c?"lcd-20x4":"lcd-16x2";doc.connect({"board","5V"},{lcd,"VCC"});doc.connect({"board","GND"},{lcd,"GND"});
        if(i2c){doc.connect({"board","A4"},{lcd,"SDA"});doc.connect({"board","A5"},{lcd,"SCL"});}else{doc.connect({"board","GND"},{lcd,"RW"});doc.connect({"board","GND"},{lcd,"VO"});const std::vector<std::pair<std::string,std::string>> pins{{"D12","RS"},{"D11","E"},{"D5","D4"},{"D4","D5"},{"D3","D6"},{"D2","D7"}};for(const auto& [board,pin]:pins)doc.connect({"board",board},{lcd,pin});}
    }else if(id=="servo_sweep"){
        doc.part("board").model="arduino-uno";const auto servo=doc.addPart(PartKind::ServoMotor,560,100);doc.connect({"board","5V"},{servo,"VCC"});doc.connect({"board","GND"},{servo,"GND"});doc.connect({"board","D9"},{servo,"SIGNAL"});
    }else if(id=="dc_motor"){
        doc.removePart("board");const auto ground=doc.addPart(PartKind::Ground,100,350),source=doc.addPart(PartKind::PowerSupply,100,60),motor=doc.addPart(PartKind::DcMotor,550,60),probe=doc.addPart(PartKind::CurrentProbe,330,60);doc.connect({source,"-"},{ground,"GND"});doc.connect({source,"+"},{probe,"+"});doc.connect({probe,"-"},{motor,"+"});doc.connect({motor,"-"},{ground,"GND"});
    }else if(id.startsWith("analog_")||id=="pwm_filter"){
        const bool pwm=id=="pwm_filter";if(!pwm)doc.removePart("board");else doc.part("board").model="arduino-uno";
        const auto ground=doc.addPart(PartKind::Ground,100,380),scope=doc.addPart(PartKind::Oscilloscope,760,60);
        auto g=[&](const std::string& part,const std::string& pin){doc.connect({part,pin},{ground,"GND"},"#64748b");};g(scope,"GND");
        std::string source=pwm?"board":doc.addPart(PartKind::FunctionGenerator,80,60),out=pwm?"D5":"OUT";
        if(pwm)g("board","GND");else{g(source,"GND");doc.part(source).value=id=="analog_rl"?20:10;doc.part(source).parameters={{"amplitude",2.5},{"offset",2.5},{"waveform",1}};}
        doc.connect({source,out},{scope,"A"},"#facc15");
        if(id=="analog_opamp"){
            doc.part(source).parameters={{"amplitude",1},{"offset",2.5},{"waveform",0}};
            const auto op=doc.addPart(PartKind::OpAmp,430,100),supply=doc.addPart(PartKind::PowerSupply,430,320);g(supply,"-");g(op,"V-");doc.connect({supply,"+"},{op,"V+"});doc.connect({source,out},{op,"IN+"});doc.connect({op,"OUT"},{op,"IN-"});doc.connect({op,"OUT"},{scope,"B"});
        }else if(id=="analog_npn"){
            const auto t=doc.addPart(PartKind::NpnTransistor,450,150),base=doc.addPart(PartKind::Resistor,300,80),load=doc.addPart(PartKind::Resistor,470,-100),supply=doc.addPart(PartKind::PowerSupply,100,-100);doc.part(base).value=10000;doc.part(load).value=1000;g(supply,"-");g(t,"E");doc.connect({source,out},{base,"1"});doc.connect({base,"2"},{t,"B"});doc.connect({supply,"+"},{load,"1"});doc.connect({load,"2"},{t,"C"});doc.connect({t,"C"},{scope,"B"});
        }else{
            const auto r=doc.addPart(PartKind::Resistor,330,80),reactive=doc.addPart(id=="analog_rl"?PartKind::Inductor:PartKind::Capacitor,540,200),probe=doc.addPart(PartKind::CurrentProbe,540,60);doc.part(r).value=id=="analog_rl"?100:1000;doc.part(reactive).value=id=="analog_rl"?100:10;
            doc.connect({source,out},{r,"1"});doc.connect({r,"2"},{probe,"+"});doc.connect({probe,"-"},{reactive,"1"});g(reactive,"2");doc.connect({reactive,"1"},{scope,"B"});
            if(pwm){const auto logic=doc.addPart(PartKind::LogicAnalyzer,760,320);g(logic,"GND");doc.connect({"board","D5"},{logic,"D0"});}
        }
        doc.part(scope).value=id=="analog_rl"?5:10;
    }
    else if(id=="sound_melody"){
        doc.part("board").model="arduino-uno";
        const auto piezo=doc.addPart(PartKind::PiezoBuzzer,650,80),speaker=doc.addPart(PartKind::Speaker,650,280),resistor=doc.addPart(PartKind::Resistor,450,280);
        doc.part(resistor).value=470;
        doc.connect({"board","D3"},{piezo,"+"});doc.connect({piezo,"-"},{"board","GND"});
        doc.connect({"board","D3"},{resistor,"1"});doc.connect({resistor,"2"},{speaker,"+"});doc.connect({speaker,"-"},{"board","GND"});
    }
    else if(id=="tmp36_temperature"){
        doc.part("board").model="arduino-uno";led("D9",80,"#22c55e");const auto sensor=doc.addPart(PartKind::TemperatureSensor,650,280);
        doc.connect({sensor,"VCC"},{"board","5V"});doc.connect({sensor,"GND"},{"board","GND"});doc.connect({sensor,"OUT"},{"board","A0"});
    }
    else if(id=="esp32_blink"||id=="esp32_s3_adc"){
        doc.part("board").model=id=="esp32_blink"?"esp32":"esp32-s3";led("GPIO5",80,"#22c55e");
        if(id=="esp32_s3_adc"){auto source=doc.addPart(PartKind::AnalogSource,430,270);doc.part(source).value=1.65;doc.connect({source,"OUT"},{"board","A0"});doc.connect({source,"GND"},{"board","GND"});}
    }
    else if(id=="oled_text_32"){oled(390,80,60);doc.part(doc.parts().back().id).model="ssd1306-128x32";}
    else if(id=="animation")oled(390,80,60);
    else if(id=="two_oleds"){oled(430,0,60);oled(800,0,61);}
    else if(id=="traffic_light"){led("GPIO14",0,"#fb586b");led("GPIO12",140,"#e9ad33");led("GPIO13",280,"#22c55e");}
    else if(id=="led_chaser"){led("GPIO5",0,"#fb586b");led("GPIO4",140,"#e9ad33");led("GPIO14",280,"#22c55e");led("GPIO12",420,"#378bcb");}
    else {
        led("GPIO14",80,"#fb586b");
        if(id=="button_led"){const auto button=doc.addPart(PartKind::Button,430,270);doc.connect({"board","GPIO12"},{button,"1"},"#378bcb");doc.connect({button,"2"},{"board","GND"},"#64748b");}
        if(id=="analog_threshold"){const auto source=doc.addPart(PartKind::AnalogSource,430,270);doc.part(source).value=0.75;doc.connect({source,"OUT"},{"board","A0"},"#378bcb");doc.connect({source,"GND"},{"board","GND"},"#64748b");}
    }
    doc.validate();return result;
}
