#pragma once
#include "simulation.hpp"
#include <deque>
#include <string>
#include <vector>
#include <algorithm>
namespace thinkercad {
struct BusTransaction {
    Microseconds time=0;
    std::string board,bus,target,status;
    unsigned clock=0;
    std::vector<std::uint8_t> tx,rx;
    std::size_t txBytes=0,rxBytes=0;
    bool stop=true,repeatedStart=false;
};
// Optional acquisition only. A closed inspector does not retain traffic.
class BusTraffic {
public:
    bool enabled=false;
    void append(BusTransaction entry){
        if(!enabled)return;
        entry.txBytes=entry.tx.size();entry.rxBytes=entry.rx.size();
        if(entry.tx.size()>128)std::vector<std::uint8_t>(entry.tx.begin(),entry.tx.begin()+128).swap(entry.tx);
        if(entry.rx.size()>128)std::vector<std::uint8_t>(entry.rx.begin(),entry.rx.begin()+128).swap(entry.rx);
        for(auto* field:{&entry.board,&entry.bus,&entry.target,&entry.status})if(field->size()>256)std::string(*field,0,256).swap(*field);
        if(entries_.size()>=capacity_){entries_.pop_front();++overwritten_;}entries_.push_back(std::move(entry));++revision_;
    }
    void clear(){entries_.clear();overwritten_=0;++revision_;}
    void setCapacity(std::size_t value){capacity_=std::clamp<std::size_t>(value,128,4096);while(entries_.size()>capacity_){entries_.pop_front();++overwritten_;}++revision_;}
    const std::deque<BusTransaction>& entries()const{return entries_;}
    std::size_t capacity()const{return capacity_;}
    std::uint64_t revision()const{return revision_;}
    std::uint64_t overwritten()const{return overwritten_;}
private:
    std::deque<BusTransaction> entries_;
    std::size_t capacity_=1024;
    std::uint64_t revision_=0,overwritten_=0;
};
}
