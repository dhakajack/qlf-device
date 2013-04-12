/*
Copyright 2013 Jack Welch (dhakajack@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
   What is this?
   This project was meant to serve a very specific need - to send an "F1"
   or meta-character to a computer via USB. The simplest/cheapest way to
   do that seemed to be strip a logic board out of a real USB keyboard and
   to interface it to an ATTINY microcontroller.
   
   The project and circuit diagrams are on my blog, starting with this 
   entry: http://blog.templaro.com/?p=1033
  
   The project invoves three switches -- one to trigger sending of the
   character (I used a foot switch), and the others to set the mode and select 
   the character (F1 or CTRL). The modes are push-to-talk (PTT), 
   single-shot, or toggle on/off. The characters could be anything that the
   keyboard itself could have sent, for my purposes, I wanted F1 and 
   control, but others could be substituted.
   
   Where does this project live?
   The mercurial repository is at: https://code.google.com/p/qlf-device/
   
*/

//pins
#define SWITCH           1 //A1, pin 7
#define SPEAKER          5 //i.e., the pin usually used for RESET
#define MODE_LED         0 //
#define CHARACTER_LED    1 //
#define TRIGGER_F1       3 //
#define TRIGGER_META     4 //

//switch state
#define SWITCH_OFF       0
#define SWITCH_PEDAL     1
#define SWITCH_CHARACTER 2
#define SWITCH_MODE      3    

//timing in milliseconds
#define DEBOUNCE_DELAY   10
#define ONE_SHOT_DELAY   200
#define LED_BLINK_RATE   200 

//mode state
#define MODE_PTT         1
#define MODE_ONE_SHOT    2
#define MODE_TOGGLE      3

//character state
#define CHARACTER_F1     0
#define CHARACTER_META   1

//keying state
#define NOT_KEYED        0
#define KEYED            1

//sound parameters
#define MODE_TONE        1000  //freq in hz
#define CHARACTER_TONE   400   //freq in hz
#define BEEP_DURATION    100   //ms
#define PAUSE_DURATION   100   //ms between beeps


int mode_state = MODE_PTT;
int character_state = CHARACTER_F1;

int switch_state = SWITCH_OFF;
int sample_state = SWITCH_OFF;
int temp_state = SWITCH_OFF;

boolean modeLED_state = LOW;
boolean characterLED_state = LOW;
boolean key = NOT_KEYED;

long lastDebounceTime = 0;
long modeLedTime = 0;
long shotTime = 0;

void setup() {
  //Serial.begin(9600);
  pinMode(SWITCH, INPUT);
  pinMode(SPEAKER, OUTPUT);   
  pinMode(MODE_LED, OUTPUT);
  pinMode(CHARACTER_LED, OUTPUT);
  pinMode(TRIGGER_F1, OUTPUT);
  pinMode(TRIGGER_META, OUTPUT);
}

void beep(int reps, int frequency) {
 
  while(reps) {
    tone(SPEAKER, frequency);
    delay(BEEP_DURATION);
    noTone(SPEAKER);
    delay(PAUSE_DURATION);
    reps--;
  }
  
  
}

void set_character() {
  key = KEYED;
  if(character_state == CHARACTER_F1){
    digitalWrite(TRIGGER_META, LOW); //insurance to make sure these are mutually exclusive
    digitalWrite(TRIGGER_F1, HIGH);
  }
  else {
    digitalWrite(TRIGGER_F1, LOW);
    digitalWrite(TRIGGER_META, HIGH);
  }
}

void reset_character() {  // as insurance, reset both
    key = NOT_KEYED;
    digitalWrite(TRIGGER_F1, LOW);
    digitalWrite(TRIGGER_META, LOW);
} 

void loop () {
  
  int sample_value = analogRead(SWITCH);
  //Serial.println(sample_value);
  
  //categorize which "virtual switch" was wiggled
  if (sample_value < 415)
    sample_state = SWITCH_PEDAL;
  else if (sample_value < 635)
    sample_state = SWITCH_CHARACTER;
  else if (sample_value < 905)
    sample_state = SWITCH_MODE;
  else
    sample_state = SWITCH_OFF;  
    
  //debounce  
  if(sample_state != switch_state) {
    if(sample_state != temp_state) {
      temp_state = sample_state;
      lastDebounceTime = millis();
    }
    else {
      if(millis() - lastDebounceTime > DEBOUNCE_DELAY) { 
         switch_state = sample_state;
         lastDebounceTime = millis();

         //execute appropriate behavior after change of switch state
         switch (switch_state) {
           case SWITCH_OFF:
             if (mode_state == MODE_PTT){
               reset_character();
             }
             break;
           case SWITCH_PEDAL:
            switch (mode_state) {
              case MODE_PTT:
                set_character();
                break;
              case MODE_ONE_SHOT:
                shotTime = millis();
                set_character();
                break;
              case MODE_TOGGLE:
                key = !key;
                if(key)
                  set_character();
                else
                  reset_character();
                break;
            }
            break;
           case SWITCH_CHARACTER:
            beep(character_state+1, CHARACTER_TONE);
            if(character_state == CHARACTER_F1)
              character_state = CHARACTER_META;
            else
              character_state = CHARACTER_F1;
            digitalWrite(CHARACTER_LED, character_state);
            //catch the loop-hole condition, of being in toggle mode with one 
            //character keyed, when the character button is hit again, changing
            //the active character. In that case, reset the outputs.
            if(mode_state == MODE_TOGGLE && key == KEYED)
              reset_character();
            break;
           case SWITCH_MODE:
            mode_state +=1;
            if (mode_state > MODE_TOGGLE)
              mode_state = MODE_PTT;
            beep(mode_state, MODE_TONE);
            if(mode_state == MODE_PTT)
              modeLED_state = LOW;
            else {
              modeLED_state = HIGH;
              modeLedTime = millis();
            }
            digitalWrite(MODE_LED, modeLED_state);
            break; 
            
         }//which switch switched states switch statement   
      }//debounced key read    
    }//same sample as last time  
  }//sample does not match recorded switch state  
  
  
  //Check one-shot timer
  if(key == KEYED && mode_state == MODE_ONE_SHOT && millis() - shotTime > ONE_SHOT_DELAY)
    reset_character();
  
  //Mode LED status update
  if(mode_state == MODE_TOGGLE && millis() - modeLedTime > LED_BLINK_RATE) {
    modeLED_state = !modeLED_state;
    digitalWrite(MODE_LED, modeLED_state);
    modeLedTime = millis();    
  }
  
}//loop
