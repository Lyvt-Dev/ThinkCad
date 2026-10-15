#pragma once
#include "Arduino.h"
#include <sstream>
#include <vector>
namespace thinker {
inline int peripheralToken(){static int token=1000;return ++token;}
inline int peripheralRead(){if(standalone)return 0;int n=0;if(!(std::cin>>n))throw std::runtime_error("Peripheral host disconnected");return n;}
inline std::string peripheralHex(const void* data,std::size_t length){const auto* bytes=static_cast<const unsigned char*>(data);std::string text;for(std::size_t i=0;i<length;++i){text+="0123456789abcdef"[bytes[i]>>4];text+="0123456789abcdef"[bytes[i]&15];}return text;}
inline int peripheralRequest(const std::string& command,int token,const std::string& args={}){return request(command+" "+std::to_string(token)+(args.empty()?"":" "+args));}
}
