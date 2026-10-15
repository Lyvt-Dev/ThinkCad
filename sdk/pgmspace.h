#pragma once
// Host adapter: ESP flash storage becomes ordinary immutable process memory.
// Firmware builds use the board's real pgmspace.h; sketch source stays unchanged.
#include <cstdint>
#include <cstring>
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef ICACHE_RODATA_ATTR
#define ICACHE_RODATA_ATTR
#endif
#ifndef PSTR
#define PSTR(text) (text)
#endif
using PGM_P = const char*;
using PGM_VOID_P = const void*;
namespace thinker {
template<class T> inline T readProgramMemory(const void* address) {
    T value;std::memcpy(&value,address,sizeof value);return value;
}
}
inline std::uint8_t pgm_read_byte(const void* p){return thinker::readProgramMemory<std::uint8_t>(p);}
inline std::uint16_t pgm_read_word(const void* p){return thinker::readProgramMemory<std::uint16_t>(p);}
inline std::uint32_t pgm_read_dword(const void* p){return thinker::readProgramMemory<std::uint32_t>(p);}
inline float pgm_read_float(const void* p){return thinker::readProgramMemory<float>(p);}
inline const void* pgm_read_ptr(const void* p){return thinker::readProgramMemory<const void*>(p);}
inline std::uint8_t pgm_read_byte_near(const void* p){return pgm_read_byte(p);}
inline std::uint16_t pgm_read_word_near(const void* p){return pgm_read_word(p);}
inline std::uint32_t pgm_read_dword_near(const void* p){return pgm_read_dword(p);}
inline void* memcpy_P(void* dest,const void* source,std::size_t count){return std::memcpy(dest,source,count);}
inline int memcmp_P(const void* a,const void* b,std::size_t count){return std::memcmp(a,b,count);}
inline std::size_t strlen_P(PGM_P text){return std::strlen(text);}
inline char* strcpy_P(char* dest,PGM_P text){return std::strcpy(dest,text);}
inline char* strncpy_P(char* dest,PGM_P text,std::size_t count){return std::strncpy(dest,text,count);}
