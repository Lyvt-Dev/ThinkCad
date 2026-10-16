#pragma once
#include "LiquidCrystal.h"
#include "Wire.h"
class LiquidCrystal_I2C:public LiquidCrystal {
public:
    LiquidCrystal_I2C(int address,int columns,int rows):LiquidCrystal(address){columns_=columns;rows_=rows;}
    void init(){if(!Wire.ready())Wire.begin();initialize(columns_,rows_,true);}
    void begin(){init();}void begin(int columns,int rows){columns_=columns;rows_=rows;init();}
    void setBacklight(uint8_t value){LiquidCrystal::setBacklight(value!=0);}
    void backlight(){setBacklight(true);}void noBacklight(){setBacklight(false);}
};
