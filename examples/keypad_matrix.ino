#include <Keypad.h>
char keys[4][4]={{'1','2','3','A'},{'4','5','6','B'},{'7','8','9','C'},{'*','0','#','D'}};
byte rows[4]={2,3,4,5}, columns[4]={6,7,8,9};
Keypad keypad(makeKeymap(keys),rows,columns,4,4);
void setup(){Serial.begin(115200);keypad.setDebounceTime(20);Serial.println("Keypad ready");}
void loop(){char key=keypad.getKey();if(key)Serial.println(key);delay(20);}
