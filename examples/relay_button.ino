// Active-high relay IN=D2. Button D7 to GND, with the internal pull-up.
void setup(){Serial.begin(115200);pinMode(2,OUTPUT);pinMode(7,INPUT_PULLUP);digitalWrite(2,LOW);Serial.println("Relay ready: hold the button");}
void loop(){digitalWrite(2,digitalRead(7)==LOW?HIGH:LOW);delay(10);}
