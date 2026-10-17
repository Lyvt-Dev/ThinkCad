#pragma once
#include "circuit.hpp"
namespace thinkercad {
class InterruptController {
public:
    enum class Mode {Low=0,Change=1,Falling=2,Rising=3};
    void reset(){handlers_.clear();}
    void attach(const CircuitDocument&,const CircuitState&,const std::string& board,int pin,Mode);
    void detach(const std::string& board,int pin);
    void sample(const CircuitDocument&,const CircuitState&);
    std::vector<int> take(const std::string& board);
    bool empty()const{return handlers_.empty();}
private:
    struct Handler {Mode mode;Level previous=Level::Floating;bool pending=false;};
    std::map<std::string,std::map<int,Handler>> handlers_;
};
}
