#pragma once
// Included by Arduino.h after the native clock / GPIO API has been declared.
constexpr uint32_t SERIAL_5N1=0x00,SERIAL_6N1=0x02,SERIAL_7N1=0x04,SERIAL_8N1=0x06,
    SERIAL_5N2=0x08,SERIAL_6N2=0x0a,SERIAL_7N2=0x0c,SERIAL_8N2=0x0e,
    SERIAL_5E1=0x20,SERIAL_6E1=0x22,SERIAL_7E1=0x24,SERIAL_8E1=0x26,
    SERIAL_5E2=0x28,SERIAL_6E2=0x2a,SERIAL_7E2=0x2c,SERIAL_8E2=0x2e,
    SERIAL_5O1=0x30,SERIAL_6O1=0x32,SERIAL_7O1=0x34,SERIAL_8O1=0x36,
    SERIAL_5O2=0x38,SERIAL_6O2=0x3a,SERIAL_7O2=0x3c,SERIAL_8O2=0x3e;
class HardwareSerial:public Stream {
    int port_,rx_=-1,tx_=-1,requestedRx_=-1,requestedTx_=-1;
    unsigned long baud_=115200;
    uint32_t format_=SERIAL_8N1;
    bool active_=false;
    bool configure(unsigned long baud,uint32_t format,int rx,int tx,bool optional){
        if(baud<300||baud>2000000||(format&~0x3eu)||(format&0x30u)==0x10u||rx< -2||tx< -2)throw std::invalid_argument("Invalid native UART configuration");
        if(thinker::standalone){active_=false;return false;}
        const int ready=thinker::request("UART_BEGIN "+std::to_string(port_)+" "+std::to_string(baud)+" "+std::to_string(format)+" "+std::to_string(rx)+" "+std::to_string(tx)+" "+std::to_string(optional));
        if(!(std::cin>>rx_>>tx_)||(ready!=0&&ready!=1)||rx_< -1||tx_< -1)throw std::runtime_error("Invalid UART initialization response");
        active_=ready;baud_=baud;format_=format;if(active_){if(rx_>=0){thinker::validatePin(rx_);thinker::modes.at(rx_)=INPUT;}if(tx_>=0){thinker::validatePin(tx_);thinker::modes.at(tx_)=OUTPUT;}}return active_;
    }
public:
    explicit HardwareSerial(int port=1):port_(port){if(port<0||port>2)throw std::invalid_argument("Invalid UART number");}
    using Print::write;
    void begin(unsigned long baud,uint32_t format=SERIAL_8N1,int8_t rx=-1,int8_t tx=-1,bool invert=false,unsigned long timeout=20000,uint8_t threshold=112){(void)timeout;(void)threshold;if(invert)throw std::invalid_argument("Inverted UART signaling is not modeled");if(rx!=-1)requestedRx_=rx;if(tx!=-1)requestedTx_=tx;configure(baud,format,requestedRx_,requestedTx_,false);}
    bool beginMonitor(unsigned long baud,uint32_t format){return configure(baud,format,-1,-1,true);}
    explicit operator bool()const{return active_;}
    unsigned long baudRate()const{return baud_;}
    int getRxPin()const{return rx_;}int getTxPin()const{return tx_;}
    int available()override{return active_?thinker::request("UART_AVAILABLE "+std::to_string(port_)):0;}
    int read()override{return active_?thinker::request("UART_READ "+std::to_string(port_)):-1;}
    int peek()override{return active_?thinker::request("UART_PEEK "+std::to_string(port_)):-1;}
    int availableForWrite()override{return active_&&tx_>=0?512:0;}
    void end(){if(active_){thinker::request("UART_END "+std::to_string(port_));if(tx_>=0)thinker::modes.at(tx_)=INPUT;}active_=false;}
    bool setPins(int8_t rx,int8_t tx,int8_t cts=-1,int8_t rts=-1){if(cts>=0||rts>=0)return false;requestedRx_=rx;requestedTx_=tx;if(active_)return configure(baud_,format_,rx,tx,false);rx_=rx;tx_=tx;return true;}
    size_t setRxBufferSize(size_t requested){return requested?256:0;}
    void flush()override{} // write() waits for its modeled transmission in Emulation.
    void flush(bool txOnly){if(!txOnly)while(available())read();}
    void waitTransmit(std::size_t count){
#ifdef THINKERCAD_EMULATION
        if(active_){const unsigned bits=1+5+((format_>>1)&3)+((format_&8)?2:1)+((format_&0x30)?1:0);thinker::advance(((bits*1000000ull+baud_-1)/baud_)*count);}
#else
        (void)count;
#endif
    }
    size_t write(uint8_t byte)override{return write(&byte,1);}
    size_t write(const uint8_t* data,size_t size)override{
        if(!active_||tx_<0||(!data&&size))return 0;static constexpr char hex[]="0123456789abcdef";std::size_t sent=0;
        while(sent<size){const auto count=std::min<std::size_t>(512,size-sent);std::string encoded;encoded.reserve(count*2);for(std::size_t i=0;i<count;++i){const auto byte=data[sent+i];encoded+=hex[byte>>4];encoded+=hex[byte&15];}const int accepted=thinker::request("UART_WRITE "+std::to_string(port_)+" "+encoded);if(accepted<0||std::size_t(accepted)>count)throw std::runtime_error("Invalid UART write response");sent+=accepted;waitTransmit(accepted);if(std::size_t(accepted)!=count)break;}return sent;
    }
protected:
    uint64_t streamClock()const override{return thinker::clockUs;}
    void streamWait(uint64_t us)override{thinker::advance(us);}
};
inline HardwareSerial Serial1(1),Serial2(2);
