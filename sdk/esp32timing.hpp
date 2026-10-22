#pragma once
// Deterministic host adapters for common ESP32 Arduino LEDC and timer APIs.
// They model peripheral time, not CPU instructions or interrupt latency.
namespace thinker {
struct LedChannel {int pin=-1;unsigned hz=1000,bits=8,duty=0;};inline std::map<unsigned,LedChannel> ledChannels;inline bool ledPinApi=false;
inline unsigned maxLedChannels(){return 6;} // C3-compatible resource bound.
inline void ledWrite(unsigned channel,uint32_t duty){auto& c=ledChannels.at(channel);if(c.pin<0)throw std::runtime_error("LEDC channel has no attached pin");unsigned range=(1u<<c.bits)-1;if(duty>range)throw std::invalid_argument("LEDC duty exceeds resolution");c.duty=duty;thinker::waveformStepLimit=std::min<uint64_t>(thinker::waveformStepLimit,std::max<uint64_t>(1,500000000ull/c.hz));pinMode(c.pin,OUTPUT);std::cout<<"PWM "<<c.pin<<' '<<duty<<' '<<range<<' '<<c.hz<<std::endl;}
}
inline double ledcSetup(uint8_t channel,double hz,uint8_t bits){if(channel>=thinker::maxLedChannels()||!std::isfinite(hz)||hz<1||hz>100000||bits<1||bits>16)return 0;auto& c=thinker::ledChannels[channel];c.hz=unsigned(std::llround(hz));c.bits=bits;return c.hz;}
inline void ledcAttachPin(uint8_t pin,uint8_t channel){thinker::validatePin(pin);if(channel>=thinker::maxLedChannels()||!thinker::ledChannels.contains(channel))throw std::invalid_argument("Call ledcSetup before ledcAttachPin");pinMode(pin,OUTPUT);thinker::ledChannels[channel].pin=pin;}
inline bool ledcAttachChannel(uint8_t pin,uint32_t hz,uint8_t bits,uint8_t channel){thinker::ledPinApi=true;if(!ledcSetup(channel,hz,bits))return false;ledcAttachPin(pin,channel);return true;}
inline bool ledcAttach(uint8_t pin,uint32_t hz,uint8_t bits){for(unsigned ch=0;ch<thinker::maxLedChannels();++ch)if(!thinker::ledChannels.contains(ch)||thinker::ledChannels[ch].pin<0)return ledcAttachChannel(pin,hz,bits,ch);return false;}
// Arduino ESP32 2.x uses channel; 3.x uses pin. Select with major-version macro.
inline bool ledcWrite(uint8_t value,uint32_t duty){
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR>=3
 for(auto& [channel,c]:thinker::ledChannels)if(c.pin==value){thinker::ledWrite(channel,duty);return true;}return false;
#else
 if(thinker::ledPinApi){for(auto& [channel,c]:thinker::ledChannels)if(c.pin==value){thinker::ledWrite(channel,duty);return true;}return false;}thinker::ledWrite(value,duty);return true;
#endif
}
inline bool ledcWriteChannel(uint8_t channel,uint32_t duty){thinker::ledWrite(channel,duty);return true;}
inline void ledcDetachPin(uint8_t pin){for(auto& [channel,c]:thinker::ledChannels)if(c.pin==pin){noTone(pin);c.pin=-1;}}
inline bool ledcDetach(uint8_t pin){ledcDetachPin(pin);return true;}
struct hw_timer_t {uint32_t hz=1000000;uint64_t base=0,origin=0,alarm=0,deadline=0;bool enabled=false,repeat=false;void(*callback)()=nullptr;};
inline hw_timer_t* timerBegin(uint32_t hz){if(!hz||hz>1000000)throw std::invalid_argument("Timer model supports 1..1000000 ticks/s");auto* timer=new hw_timer_t;timer->hz=hz;timer->origin=thinker::clockUs;thinker::timeObservers[timer]=[timer]{if(timer->enabled&&thinker::clockUs>=timer->deadline){if(timer->repeat)timer->deadline+=std::max<uint64_t>(1,timer->alarm*1000000/timer->hz);else timer->enabled=false;if(timer->enabled)thinker::eventDeadlines[timer]=timer->deadline;else thinker::eventDeadlines.erase(timer);if(timer->callback)timer->callback();}};return timer;}
inline hw_timer_t* timerBegin(uint8_t number,uint16_t divider,bool countUp){if(number>3||!divider||!countUp)throw std::invalid_argument("Timer requires count-up and nonzero divider");return timerBegin(80000000u/divider);}
inline void timerAttachInterrupt(hw_timer_t* timer,void(*callback)()){if(!timer)throw std::invalid_argument("Null timer");timer->callback=callback;}
inline void timerAttachInterrupt(hw_timer_t* timer,void(*callback)(),bool){timerAttachInterrupt(timer,callback);}
inline void timerAlarmWrite(hw_timer_t* timer,uint64_t ticks,bool repeat){if(!timer||!ticks)throw std::invalid_argument("Timer alarm must be nonzero");timer->alarm=ticks;timer->repeat=repeat;}
inline void timerAlarmEnable(hw_timer_t* timer){if(!timer||!timer->alarm)throw std::invalid_argument("Configure timer alarm first");timer->enabled=true;timer->deadline=thinker::clockUs+std::max<uint64_t>(1,timer->alarm*1000000/timer->hz);thinker::eventDeadlines[timer]=timer->deadline;}
inline void timerAlarm(hw_timer_t* timer,uint64_t ticks,bool repeat,uint64_t reloadCount){if(reloadCount)throw std::invalid_argument("Finite timer reload count is not supported; use one-shot or unlimited autoreload");timerAlarmWrite(timer,ticks,repeat);timerAlarmEnable(timer);}
inline void timerAlarmDisable(hw_timer_t* timer){if(timer){timer->enabled=false;thinker::eventDeadlines.erase(timer);}}
inline uint64_t timerRead(hw_timer_t* timer){if(!timer)throw std::invalid_argument("Null timer");return timer->base+(thinker::clockUs-timer->origin)*timer->hz/1000000;}
inline void timerWrite(hw_timer_t* timer,uint64_t value){if(!timer)throw std::invalid_argument("Null timer");timer->base=value;timer->origin=thinker::clockUs;}
inline void timerEnd(hw_timer_t* timer){thinker::timeObservers.erase(timer);thinker::eventDeadlines.erase(timer);delete timer;}
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

inline uint32_t ledcRead(uint8_t value){if(thinker::ledPinApi){for(auto& [channel,c]:thinker::ledChannels)if(c.pin==value)return c.duty;return 0;}auto it=thinker::ledChannels.find(value);return it==thinker::ledChannels.end()?0:it->second.duty;}
inline double ledcReadFreq(uint8_t value){if(thinker::ledPinApi){for(auto& [channel,c]:thinker::ledChannels)if(c.pin==value)return c.hz;return 0;}auto it=thinker::ledChannels.find(value);return it==thinker::ledChannels.end()?0:it->second.hz;}
inline void timerStop(hw_timer_t* timer){timerAlarmDisable(timer);}
inline void timerStart(hw_timer_t* timer){timerAlarmEnable(timer);}
inline uint64_t timerReadMicros(hw_timer_t* timer){return timerRead(timer)*1000000/timer->hz;}
inline uint64_t timerReadMillis(hw_timer_t* timer){return timerReadMicros(timer)/1000;}
