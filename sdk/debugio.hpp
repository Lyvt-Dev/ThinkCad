#pragma once
// Only debug builds use this authenticated loopback channel. Normal sketches
// retain their existing stdin/stdout transport and do not open a socket.
#ifdef THINKCAD_DEBUG_TRANSPORT
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Socket transport needs no Win32 user-interface declarations. In particular,
// winuser.h's INPUT typedef conflicts with the Arduino pin-mode constant.
#ifndef NOUSER
#define NOUSER
#define THINKCAD_DEBUG_RESTORE_NOUSER
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef THINKCAD_DEBUG_RESTORE_NOUSER
#undef NOUSER
#undef THINKCAD_DEBUG_RESTORE_NOUSER
#endif
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include <array>
#include <cstdlib>
#include <iostream>
#include <streambuf>
#include <string>
namespace thinker::debugio {
#ifdef _WIN32
using Socket=SOCKET;
constexpr Socket invalid=INVALID_SOCKET;
inline void close(Socket socket){closesocket(socket);}
#else
using Socket=int;
constexpr Socket invalid=-1;
inline void close(Socket socket){::close(socket);}
#endif
class Channel final:public std::streambuf {
    Socket socket_=invalid;
    std::array<char,4096> input_{},output_{};
    bool sendAll(const char* data,std::size_t size){
        while(size){
#ifdef _WIN32
            const auto sent=::send(socket_,data,static_cast<int>(size),0);
#else
            const auto sent=::send(socket_,data,size,MSG_NOSIGNAL);
#endif
            if(sent<=0)return false;data+=sent;size-=static_cast<std::size_t>(sent);
        }return true;
    }
protected:
    int sync()override{const auto count=pptr()-pbase();if(count&&!sendAll(pbase(),static_cast<std::size_t>(count)))return -1;setp(output_.data(),output_.data()+output_.size());return 0;}
    int_type overflow(int_type c)override{if(sync()!=0)return traits_type::eof();if(!traits_type::eq_int_type(c,traits_type::eof())){*pptr()=traits_type::to_char_type(c);pbump(1);}return traits_type::not_eof(c);}
    int_type underflow()override{
        if(gptr()<egptr())return traits_type::to_int_type(*gptr());
        if(sync()!=0)return traits_type::eof();
        const auto received=::recv(socket_,input_.data(),static_cast<int>(input_.size()),0);if(received<=0)return traits_type::eof();setg(input_.data(),input_.data(),input_.data()+received);return traits_type::to_int_type(*gptr());
    }
public:
    Channel(){setp(output_.data(),output_.data()+output_.size());}
    bool connect(){
        const auto* portText=std::getenv("THINKCAD_DEBUG_PORT");const auto* ticket=std::getenv("THINKCAD_DEBUG_TOKEN");if(!portText||!ticket||std::string(ticket).size()!=64)return false;
        char* end=nullptr;const auto port=std::strtol(portText,&end,10);if(!end||*end||port<1||port>65535)return false;
#ifdef _WIN32
        WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data)!=0)return false;
#endif
        socket_=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(socket_==invalid)return false;
        sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(static_cast<unsigned short>(port));address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        if(::connect(socket_,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0)return false;
        const std::string greeting=std::string("THINKCAD_DEBUG ")+ticket+"\n";return sendAll(greeting.data(),greeting.size());
    }
    ~Channel()override{sync();if(socket_!=invalid)close(socket_);}
};
class Diagnostic final:public std::streambuf {
    Channel& channel_;std::string line_;
    int sync()override{if(line_.empty())return 0;static constexpr char hex[]="0123456789abcdef";std::string message="DEBUG_LOG ";for(unsigned char byte:line_){message+=hex[byte>>4];message+=hex[byte&15];}message+='\n';line_.clear();if(channel_.sputn(message.data(),message.size())!=static_cast<std::streamsize>(message.size()))return -1;return channel_.pubsync();}
    int_type overflow(int_type c)override{if(traits_type::eq_int_type(c,traits_type::eof()))return sync()==0?traits_type::not_eof(c):traits_type::eof();const auto byte=traits_type::to_char_type(c);line_+=byte;if(byte=='\n'||line_.size()>=1024)if(sync()!=0)return traits_type::eof();return c;}
public:
    explicit Diagnostic(Channel& channel):channel_(channel){}
    ~Diagnostic()override{sync();}
};
class Redirect {
    Channel channel_;Diagnostic diagnostic_{channel_};
    std::streambuf *input_=nullptr,*output_=nullptr,*error_=nullptr,*log_=nullptr;
public:
    Redirect(){if(!channel_.connect())std::_Exit(125);input_=std::cin.rdbuf(&channel_);output_=std::cout.rdbuf(&channel_);error_=std::cerr.rdbuf(&diagnostic_);log_=std::clog.rdbuf(&diagnostic_);}
    ~Redirect(){std::cerr.flush();std::clog.flush();std::cout.flush();std::cin.rdbuf(input_);std::cout.rdbuf(output_);std::cerr.rdbuf(error_);std::clog.rdbuf(log_);}
};
inline Redirect redirect;
}
#endif
