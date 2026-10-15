#include "language.hpp"
#include "examples.hpp"
#include "paths.hpp"
#include <QCoreApplication>
#include <QFile>
#include <stdexcept>
#include <algorithm>
using namespace thinkercad;
QList<CircuitExample> ExampleCatalog::entries(){return {
    {"sound_melody",Language::text("Melodie mit Piezo und Lautsprecher"),Language::text("Arduino D3: Piezo direkt, 8-Ω-Lautsprecher über 470 Ω. tone() spielt Noten, noTone() erzeugt Pausen.")},
    {"tmp36_temperature",Language::text("Temperatur messen (TMP36)"),Language::text("TMP36 an Arduino 5V/GND/A0. Temperatur im Eigenschaftenfeld ändern; über 20 °C leuchtet die LED.")},
    {"animation",Language::text("OLED-Animation"),Language::text("ESP8266 und SSD1306: zwei Bitmap-Frames aus PROGMEM mit realistischer I²C-Transferzeit.")},
    {"led_blink",Language::text("LED blinken lassen"),Language::text("Externe LED an D5 mit 220-Ω-Vorwiderstand. Mit dem Temporegler die Blinkgeschwindigkeit vergleichen.")},
    {"button_led",Language::text("Taster schaltet LED"),Language::text("Taster an D6 gegen GND, interner Pullup, LED an D5. Taster per Doppelklick oder Auswahl-Checkbox drücken.")},
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
    if(id=="sound_melody"){
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
