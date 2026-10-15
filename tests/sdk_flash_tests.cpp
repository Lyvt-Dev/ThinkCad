#include <Arduino.h>
#include <avr/pgmspace.h>
#include <iostream>
int main(){
    const uint8_t frames[][4] PROGMEM={{0x18,0xff,0x42,0x81},{0xa5,0x5a,0x00,0x7e}};
    const uint8_t* const table[] PROGMEM={frames[0],frames[1]};
    const auto* frame=static_cast<const uint8_t*>(pgm_read_ptr(&table[1]));
    if(pgm_read_byte(frame)!=0xa5 || pgm_read_byte_near(frame+3)!=0x7e)return 1;
    const uint16_t words[] PROGMEM={0x1234,0xabcd};
    if(pgm_read_word(&words[1])!=0xabcd || pgm_read_word_near(words)!=0x1234)return 2;
    const uint32_t dword=0x89abcdef;unsigned char unaligned[5];std::memcpy(unaligned+1,&dword,4);
    if(pgm_read_dword(unaligned+1)!=dword || pgm_read_dword_near(unaligned+1)!=dword)return 3;
    const float scale PROGMEM=1.25f;if(pgm_read_float(&scale)!=1.25f)return 4;
    const char text[] ICACHE_RODATA_ATTR="Flash animation";char copy[32]{};
    strcpy_P(copy,text);if(strlen_P(text)!=15 || memcmp_P(copy,text,16)!=0)return 5;
    memcpy_P(copy,PSTR("Frames"),7);if(std::strcmp(copy,"Frames")!=0)return 6;
    if(std::strcmp(F("Ready"),"Ready")!=0)return 7;
    std::cout<<"Flash bitmap, pointer table, unaligned reads and strings passed\n";
}
