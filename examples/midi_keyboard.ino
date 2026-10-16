#include <Keypad.h>
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
char notes[4][8]={{48,49,50,51,52,53,54,55},{56,57,58,59,60,61,62,63},{64,65,66,67,68,69,70,71},{72,73,74,75,76,77,78,79}};
byte rows[4]={2,3,4,5}, columns[8]={6,7,8,9,10,11,12,13};
Keypad keyboard(makeKeymap(notes),rows,columns,4,8);
void setup(){MIDI.begin(MIDI_CHANNEL_OMNI);MIDI.turnThruOff();keyboard.setDebounceTime(20);}
void loop(){if(keyboard.getKeys())for(int i=0;i<LIST_MAX;++i)if(keyboard.key[i].stateChanged){if(keyboard.key[i].kstate==PRESSED)MIDI.sendNoteOn(keyboard.key[i].kchar,100,1);if(keyboard.key[i].kstate==RELEASED)MIDI.sendNoteOff(keyboard.key[i].kchar,0,1);}delay(20);}
