#include "Adafruit_NeoPixel.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){
    thinker::standalone=true;std::ostringstream output;auto* original=std::cout.rdbuf(output.rdbuf());
    try{
        Adafruit_NeoPixel pixels(3,2,NEO_GRB);require(pixels.begin(),"native strip begins");pixels.setPixelColor(0,255,0,0);pixels.setPixelColor(1,0,255,0);pixels.setPixelColor(2,0,0,255);require(pixels.getPixelColor(0)==0xff0000,"API color uses RGB");require(pixels.getPixels()[0]==0&&pixels.getPixels()[1]==255,"underlying bytes use GRB order");pixels.show();require(output.str().find("NEOPIXEL 2 800000 00ff00ff00000000ff")!=std::string::npos,"host receives physical GRB frame");require(!pixels.canShow(),"reset latch interval applies");delayMicroseconds(300);require(pixels.canShow(),"ready after reset");pixels.setBrightness(127);require(pixels.getBrightness()==127&&pixels.getPixels()[1]==127,"brightness scales actual transmitted bytes");pixels.fill(Adafruit_NeoPixel::Color(30,60,90),1,1);require(pixels.getPixels()[3]==30&&pixels.getPixels()[4]==15&&pixels.getPixels()[5]==45,"fill respects range and brightness");pixels.clear();require(pixels.getPixelColor(0)==0,"clear resets pixels");require(Adafruit_NeoPixel::ColorHSV(0)==0xff0000&&Adafruit_NeoPixel::ColorHSV(21845)==0x00ff00,"HSV primary colors");require(Adafruit_NeoPixel::gamma8(0)==0&&Adafruit_NeoPixel::gamma8(255)==255,"gamma endpoints");bool rejected=false;try{pixels.updateLength(1025);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"bounded pixel memory");
        std::cout.rdbuf(original);std::cout<<"Native NeoPixel buffer, ordering, brightness, latch and color API checks passed\n";return 0;
    }catch(const std::exception& error){std::cout.rdbuf(original);std::cerr<<error.what()<<'\n';return 1;}
}
