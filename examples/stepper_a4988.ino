// Uno D2 STEP, D3 DIR, D4 ENABLE. A4988 VDD=5 V, VMOT=12 V, shared GND.
void setup(){Serial.begin(115200);pinMode(2,OUTPUT);pinMode(3,OUTPUT);pinMode(4,OUTPUT);pinMode(6,INPUT_PULLUP);digitalWrite(4,LOW);}
void loop(){for(int direction=1;direction>=0;--direction){digitalWrite(3,direction);for(int i=0;i<20;++i){if(digitalRead(6)==HIGH){Serial.println("Endstop reached");delay(50);return;}digitalWrite(2,HIGH);delayMicroseconds(1000);digitalWrite(2,LOW);delayMicroseconds(1000);}Serial.println(direction?"position=20":"position=0");delay(300);}}
