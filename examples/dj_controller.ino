#include <Encoder.h>
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
Encoder encoder(2,3);
long lastPosition=0;int lastFader=-1;bool lastPad=false;unsigned long sampled=0;
void setup(){MIDI.begin(MIDI_CHANNEL_OMNI);MIDI.turnThruOff();pinMode(4,INPUT_PULLUP);pinMode(5,INPUT_PULLUP);}
void loop(){
  long position=encoder.read();if(position!=lastPosition){MIDI.sendControlChange(16,position>lastPosition?65:63,1);lastPosition=position;}
  if(millis()-sampled>=20){sampled=millis();int fader=analogRead(A0)>>3;if(fader!=lastFader){MIDI.sendControlChange(7,fader,1);lastFader=fader;}
    bool pad=digitalRead(5)==LOW;if(pad!=lastPad){if(pad)MIDI.sendNoteOn(36,100,1);else MIDI.sendNoteOff(36,0,1);lastPad=pad;}
    MIDI.sendControlChange(17,digitalRead(4)==LOW?127:0,1);
  }delay(2);
}
