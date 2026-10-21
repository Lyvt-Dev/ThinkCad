#include "thinkercad/modules.hpp"
namespace thinkercad {
void registerHardwareModules(std::map<std::string,ModuleDefinition>& catalog){
    auto add=[&](std::string id,std::string label,std::string bus,std::vector<std::string> pins,double width,double height,double low,double high,std::string notes){
        ModuleDefinition d;d.id=id;d.label=label;d.bus=bus;d.implementation=id;d.pins=pins;d.width=width;d.height=height;d.minVoltage=low;d.maxVoltage=high;d.builtin=true;d.notes=notes;d.idleCurrent=.005;
        for(std::size_t i=0;i<pins.size();++i){auto half=(pins.size()+1)/2;auto row=i<half?i:pins.size()-i-1;d.pinPositions[pins[i]]={i<half?0:width,(height-(half-1)*2.54)/2+row*2.54};}
        catalog.emplace(id,std::move(d));
    };
    add("pn532-transmitter","NFC transmitter / reader-writer · PN532","electrical",{"VCC","GND","SDA","SCL","IRQ","RST"},43,41,2.7,5.5,"Active 13.56 MHz NFC reader/writer for synthetic test tags. Adafruit_PN532 API: UID, NTAG page reads and writes (user pages 4–39). Writes last for the simulation session. ThinkCad extensions also emulate a synthetic tag for a second PN532, with configurable UID/NDEF and read-only or writable user pages. No raw PN532 frames, peer-to-peer, payment-card or 125 kHz support.");
    catalog.at("pn532-transmitter").pullups={{"SDA",4700},{"SCL",4700}};
    {auto& d=catalog.at("pn532-transmitter");for(std::size_t i=0;i<d.pins.size();++i)d.pinPositions[d.pins[i]]={15.15+i*2.54,41};}
    add("cc1101","CC1101 · sub-GHz SPI transceiver","spi",{"VCC","GND","SCK","MISO","MOSI","CS","GDO0","GDO2","ANT"},20,34,1.8,3.6,"Packet-level model: SPI registers, strobes and bounded FIFOs. Matching frequency, channel and sync word required. No real RF output, propagation or jamming model.");
    catalog.at("cc1101").maxClock=10000000;
    add("sx1276","SX1276 / RFM95 · LoRa transceiver","spi",{"VCC","GND","SCK","MISO","MOSI","CS","RESET","DIO0","DIO1","DIO2","ANT"},16,16,1.8,3.7,"LoRa packet model: FIFO, frequency, sync word, spreading factor and bandwidth matching. FSK, RF propagation and precise airtime are not modeled.");
    catalog.at("sx1276").maxClock=10000000;catalog.at("sx1276").pullups["RESET"]=10000;
    add("mcp2515","MCP2515 · SPI CAN controller","spi",{"TXCAN","RXCAN","CLKOUT","TX0RTS","TX1RTS","TX2RTS","OSC2","OSC1","GND","RX1BF","RX0BF","INT","SCK","MOSI","MISO","CS","RESET","VCC"},7.62,22.86,2.7,5.5,"CAN controller register model with reset, bit modify, mode selection and loopback frames. External CAN arbitration/error confinement, oscillator and clock-output behavior are not modeled. Pair TXCAN/RXCAN with a transceiver for planning.");
    catalog.at("mcp2515").maxClock=10000000;catalog.at("mcp2515").pullups["RESET"]=10000;
    add("tja1050","TJA1050 · CAN physical transceiver","electrical",{"TXD","GND","VCC","RXD","VREF","CANL","CANH","S"},7.62,10.7,4.75,5.25,"Behavioral dominant/recessive CAN electrical transceiver. S high selects silent mode. Add 120 ohm termination at each bus end. No automotive network connection.");
    add("sim800l","SIM800L · GSM modem (test model)","uart",{"VCC","GND","TX","RX","RST","PWRKEY","ANT"},25,23,3.4,4.4,"Offline UART test modem, 9600 baud 8N1, 2.8 V logic. Supports AT, ATI, CPIN?, CSQ, CREG?, CMGF and a synthetic SMS inbox via CMGL. No cellular network or real SMS.");
    add("sim7600","SIM7600 · LTE UART carrier (test model)","uart",{"VCC","GND","TX","RX","RESET","PWRKEY","ANT"},30,30,4.75,5.25,"Offline UART test carrier, 115200 baud 8N1, 3.3 V logic with onboard translation, 5 V input. Supports basic AT status and synthetic SMS inbox commands. Not the bare 1.8 V-logic modem.");
    for(auto id:{"sim800l","sim7600"})catalog.at(id).parameters={{"registered","Registered (synthetic)","count",1,0,1},{"signal","Signal strength (CSQ)","count",20,0,31},{"sms","Test SMS present","count",0,0,1}};
    add("neo-6m","NEO-6M · GNSS receiver","uart",{"VCC","GND","TX","RX","PPS"},16,12.2,2.7,3.6,"Synthetic GNSS fixes over UART, 9600 baud 8N1. Poll with a valid NMEA PUBX,00 sentence to receive checksummed GPRMC/GPGGA. No satellite RF, UBX configuration or autonomous periodic output yet.");
    catalog.at("neo-6m").parameters={{"latitude","Latitude","deg",52.52,-90,90},{"longitude","Longitude","deg",13.405,-180,180},{"altitude","Altitude","m",34,-500,18000},{"fix","Position fix","count",1,0,1}};
    add("fingerprint-uart","Optical fingerprint sensor · UART","uart",{"VCC","GND","TX","RX","TOUCH"},20,56,3.6,6,"Synthetic fingerprint protocol (EF01 packets), 57600 baud 8N1, 3.3 V logic. GetImage, Image2Tz, Search, password verification and template count. No real biometric data or matching algorithm.");
    catalog.at("fingerprint-uart").parameters={{"finger","Finger present","count",0,0,1},{"match","Test template matches","count",1,0,1},{"template","Template ID","count",1,0,999}};
    add("nrf52840","nRF52840 · MDBT50Q module (planning only)","electrical",{"VCC","GND","SWDIO","SWDCLK","RESET","P0_06","P0_08","P0_13","P0_15","P0_17","P0_20","USB_D-","USB_D+","VBUS"},10.5,15.5,1.7,3.6,"Planning-only module with selected exposed pins, not a complete pad map. No nRF52840 firmware execution or Bluetooth stack. Use the existing nRF24L01 for simulated 2.4 GHz packet exchange.");
    add("rtl2832u","RTL2832U · SDR USB dongle (planning only)","electrical",{"VCC","GND","USB_D+","USB_D-","ANT"},24,70,4.75,5.25,"Planning-only USB dongle abstraction with RF tuner. RTL2832U itself is a USB demodulator, not an RF tuner. USB enumeration, RF capture and IQ decoding are not implemented.");
    add("ad9361","AD9361 · SDR front-end module (planning only)","electrical",{"VCC","GND","SCK","MOSI","MISO","CS","RESET","RX1","RX2","TX1","TX2","REF_CLK"},45,45,3.1,3.5,"Planning-only powered module abstraction, not the bare BGA pinout. No register/USB interface, RF conversion or IQ streaming model.");
    for(auto id:{"li-ion-18650","lipo-1s","nimh-aa"}){
        const bool nimh=std::string(id)=="nimh-aa";add(id,nimh?"NiMH AA · rechargeable cell":std::string(id)=="lipo-1s"?"LiPo · 1S rechargeable pouch":"Li-ion 18650 · rechargeable cell","electrical",{"+","-"},nimh?14.5:std::string(id)=="lipo-1s"?30:18,nimh?50.5:std::string(id)=="lipo-1s"?40:65,1,6,"Static state-of-charge voltage source with internal resistance. SOC is set in properties; capacity, temperature, depletion and charge accumulation are not simulated.");
        auto& d=catalog.at(id);d.power=d.ground="";d.idleCurrent=0;d.parameters={{"soc","State of charge","%",80,0,100},{"resistance","Internal resistance","ohm",nimh?.1:.08,.01,10}};d.pinPositions={{"+",{d.width/2,0}},{"-",{d.width/2,d.height}}};
    }
    add("usb-c-charging","USB-C · 5 V charging input","electrical",{"VBUS","GND","CC1","CC2"},15.24,15.24,4.75,5.25,"Passive USB-C sink breakout: 5.1 kohm Rd resistors on CC1/CC2. Connect an external 5 V supply to VBUS/GND and a charger to the cell. No USB-PD negotiation or implicit power source.");
    catalog.at("usb-c-charging").power=catalog.at("usb-c-charging").ground="";catalog.at("usb-c-charging").idleCurrent=0;
    add("tp4056","TP4056 · 1S Li-ion charger (DC model)","electrical",{"VCC","GND","BAT+","BAT-","CHRG","STDBY"},17,25,4,5.5,"DC approximation: current limited below 4.2 V, voltage limit 4.2 V, common BAT-/GND, open-drain status. Requires external 5 V input. No thermal shutdown, protection MOSFETs, charge timer or cell SOC integration.");
    catalog.at("tp4056").parameters={{"current","Charge current limit","A",.5,.01,1}};
}
}
