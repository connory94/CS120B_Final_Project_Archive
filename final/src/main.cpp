//+++++++++++++++++++++++++
// CS120B FINAL PROJECT
// its a song maker. details on the report
// numbering of file name is based on versions
//+++++++++++++++++++++++++

#include <math.h> //probably the only library I'll need to use

//wasn't sure which helpers to use, so I included all of them
#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "spiAVR.h"
//#include "serialATmega-4-1.h"

//(DONE) task for buttons
//(DONE) task for joystick
//(DONE) task for buzzer preview
//(DONE) task for buzzer playback
//(DONE) task for screen initialization
//(DONE) task for screen drawing
//(DONE) task for LED mode display
//(DONE) task for recording/deleting notes

#define NUM_TASKS 8

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;

//=========================================
// from previous labs
//=========================================

//borrowed
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//=========================================
// periods
//=========================================
const unsigned long JoyPeriod = 1;
const unsigned long ButtonPeriod = 50;
const unsigned long PrevPeriod = 1;
const unsigned long InitPeriod = 10;
const unsigned long ScreenPeriod = 1;
const unsigned long LEDPeriod = 100;
const unsigned long PlayPeriod = 400;
const unsigned long RecPeriod = 1;

// e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long GCD_PERIOD = 1;//TODO:Set the GCD Period

//=========================================
// given stuff that makes tasks work
//=========================================

task tasks[NUM_TASKS]; // declared task array with 5 tasks


void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

//=========================================
// my variables
//=========================================

double joyX = 0;              //X direction of joystick
double joyY = 0;              //Y direction of joystick
unsigned char joyPress = 0;   //joystick press
unsigned char left = 0;       //button variables
unsigned char right = 0;      //button variables
unsigned char endInit = 0;    //to tell program that initialization ended
unsigned char octave = 1;     //affects buzzer tone
unsigned char play = 0;       //turns playback mode on/off;
unsigned char recording = 0;  //tells recording task to record a note
unsigned char deleting = 0;   //tells recoridng task to delete a note
unsigned char noteAmount = 0; //tracks number of notes recorded. never exceeds 32.
int notes[] = {523, 554, 587, //for playing/recording notes.
               622, 659, 698, 
               740, 784, 831, 
               880, 932, 988};
int recordedNotes[32] = {0};  //self explanatory
int recordedOctave[32] = {0}; //so that the right octave is played

//=========================================
// my helper functions
//=========================================

void sendCom(unsigned char command){
  PORTD = (PORTD & 0x7F);
  SPI_SEND(command);
}

void sendData(unsigned char data){
  PORTD = (PORTD & 0x7F) | 0x80;
  SPI_SEND(data);
}

//sets window to draw in
void setWindow(int x0, int y0, int x1, int y1){
  sendCom(0x2A); //CASET: col range
  sendData(0x00);    //apparently needs to send high and low byte for 16bit
  sendData(x0+2);    //but 127 can be done w/ 8 bits, so always send 0 first
  sendData(0x00);
  sendData(x1+2);

  sendCom(0x2B); //RASET: row range
  sendData(0x00);//btw these include offsets specific to my screen
  sendData(y0+3);
  sendData(0x00);
  sendData(y1+3);
}

//fills a given window w/ the specified color
void drawColor(int x0, int y0, int x1, int y1, unsigned char colorHigh, unsigned char colorLow){
      setWindow(x0, y0, x1, y1);
      sendCom(0x2C); //RAMWR
      for(int i = 0; i < (x1+1)*(y1+1); i++){
        sendData(colorHigh);
        sendData(colorLow);
      }
}

//writes letter based on joystick direction, color represents octave
void drawNote(short angle){
  unsigned int i = angle/30;

  int colHigh = 0;
  int colLow = 0;

  switch(octave){
    case(0): //red
      colHigh = 0xF8;
      colLow = 0x00;
      break;

    case(1): //blue
      colHigh = 0x00;
      colLow = 0x1F;
      break;

    case(2): //green
      colHigh = 0x07;
      colLow = 0xE0;
      break;
  }

  //center at 64, 84
  switch(i){
    case(0): //C
      drawColor(49, 74, 69, 76, colHigh, colLow);
      drawColor(49, 74, 51, 94, colHigh, colLow);
      drawColor(49, 94, 69, 96, colHigh, colLow);
      break;

    case(1): //C#
      drawColor(49, 74, 69, 76, colHigh, colLow);
      drawColor(49, 74, 51, 94, colHigh, colLow);
      drawColor(49, 94, 69, 96, colHigh, colLow);

      //#
      drawColor(74, 80, 85, 81, colHigh, colLow);
      drawColor(74, 90, 85, 91, colHigh, colLow);
      drawColor(75, 74, 77, 96, colHigh, colLow);
      drawColor(81, 74, 83, 96, colHigh, colLow);
      break;

    case(2): //D
      drawColor(49, 69, 51, 94, colHigh, colLow);
      drawColor(51, 69, 64, 72, colHigh, colLow);
      drawColor(64, 72, 67, 95, colHigh, colLow);
      drawColor(49, 92, 64, 95, colHigh, colLow);
      break;

    case(3): //D#
      drawColor(49, 69, 51, 94, colHigh, colLow);
      drawColor(51, 69, 64, 72, colHigh, colLow);
      drawColor(64, 72, 67, 95, colHigh, colLow);
      drawColor(49, 92, 64, 95, colHigh, colLow);

      drawColor(74, 80, 85, 81, colHigh, colLow);
      drawColor(74, 90, 85, 91, colHigh, colLow);
      drawColor(75, 74, 77, 96, colHigh, colLow);
      drawColor(81, 74, 83, 96, colHigh, colLow);
      break;

    case(4): //E
      drawColor(49, 69, 51, 94, colHigh, colLow);
      drawColor(49, 69, 69, 72, colHigh, colLow);
      drawColor(49, 79, 69, 81, colHigh, colLow);
      drawColor(49, 91, 69, 94, colHigh, colLow);
      break;

    case(5): //F
      drawColor(49, 69, 51, 94, colHigh, colLow);
      drawColor(49, 69, 69, 72, colHigh, colLow);
      drawColor(49, 79, 69, 81, colHigh, colLow);
      break;

    case(6): //F#
      drawColor(49, 69, 51, 94, colHigh, colLow);
      drawColor(49, 69, 69, 72, colHigh, colLow);
      drawColor(49, 79, 69, 81, colHigh, colLow);

      drawColor(74, 80, 85, 81, colHigh, colLow);
      drawColor(74, 90, 85, 91, colHigh, colLow);
      drawColor(75, 74, 77, 96, colHigh, colLow);
      drawColor(81, 74, 83, 96, colHigh, colLow);
      break;

    case(7): //G
      drawColor(49, 69, 51, 94, colHigh, colLow);
      drawColor(49, 69, 69, 72, colHigh, colLow);
      drawColor(59, 79, 69, 81, colHigh, colLow);
      drawColor(67, 79, 69, 94, colHigh, colLow);
      drawColor(49, 91, 69, 94, colHigh, colLow);
      break;

    case(8): //G#
      drawColor(49, 69, 51, 94, colHigh, colLow);
      drawColor(49, 69, 69, 72, colHigh, colLow);
      drawColor(59, 79, 69, 81, colHigh, colLow);
      drawColor(67, 79, 69, 94, colHigh, colLow);
      drawColor(49, 91, 69, 94, colHigh, colLow);

      drawColor(74, 80, 85, 81, colHigh, colLow);
      drawColor(74, 90, 85, 91, colHigh, colLow);
      drawColor(75, 74, 77, 96, colHigh, colLow);
      drawColor(81, 74, 83, 96, colHigh, colLow);
      break;

    case(9): //A
      drawColor(49, 74, 69, 76, colHigh, colLow);
      drawColor(49, 74, 51, 94, colHigh, colLow);
      drawColor(66, 74, 69, 94, colHigh, colLow);
      drawColor(49, 80, 69, 82, colHigh, colLow);
      break;
    
    case(10): //A#
      drawColor(49, 74, 69, 76, colHigh, colLow);
      drawColor(49, 74, 51, 94, colHigh, colLow);
      drawColor(66, 74, 69, 94, colHigh, colLow);
      drawColor(49, 80, 69, 82, colHigh, colLow);

      drawColor(74, 80, 85, 81, colHigh, colLow);
      drawColor(74, 90, 85, 91, colHigh, colLow);
      drawColor(75, 74, 77, 96, colHigh, colLow);
      drawColor(81, 74, 83, 96, colHigh, colLow);
      break;

    case(11): //B
      drawColor(49, 69, 51, 94, colHigh, colLow);
      drawColor(51, 69, 64, 72, colHigh, colLow);
      drawColor(49, 79, 67, 81, colHigh, colLow);
      drawColor(64, 72, 67, 95, colHigh, colLow);
      drawColor(49, 92, 64, 95, colHigh, colLow);
      break;
  }

}

//draws when notes are recorded, assumed that we don't call when at 32 notes
void drawRecord(){
  unsigned char noteX = noteAmount * 4;
  int colHigh = 0;
  int colLow = 0;

  switch(octave){
    case(0): //red
      colHigh = 0xF8;
      colLow = 0x00;
      break;

    case(1): //blue
      colHigh = 0x00;
      colLow = 0x1F;
      break;

    case(2): //green
      colHigh = 0x07;
      colLow = 0xE0;
      break;
  }

  drawColor(noteX, 5, noteX+2, 8, colHigh, colLow);
}

//clears previous note on screen, assumed that it's never called when no notes exist
void drawDelete(){
  unsigned char clearX = (noteAmount - 1) * 4;
  drawColor(clearX, 5, clearX+2, 8, 0xFF, 0xFF);
}

//set ICR1 for buzzer frequency
void setNote(short angle, char currOctave){
  int index = angle/30;
  int frequency = notes[index];

  //0 = -1, 2 = +1 
  //c5
  if(currOctave == 0){frequency /= 4;}
  else if(currOctave == 1){frequency /= 2;}

  ICR1 = 2000000 / frequency;

}

//gets angle for determining note
//basically uses inverse tangent to get angle from X and Y
//found this online
int getAngle(unsigned char x, unsigned char y){
  int offsetX = x - 50;
  int offsetY = y - 50;

  double angle = (90 - atan2(offsetY, offsetX) * 180 / M_PI); 
  
  if(angle < 0){angle += 360;}
  return (int)angle;
}

//returns 0 when in deadzone, returns 1 when out of it
unsigned char deadzone(){
  char x = joyX - 50;
  char y = joyY - 50;

  if(x < 0){x *= -1;}
  if(y < 0){y *= -1;}

  if(x >= 15 || y >= 15){return 1;}
  else{return 0;}
}

//=========================================
// my code
//=========================================

//just reads joystick. nothing fancy, don't need more than one state for it.
enum JoyStates {JoyStart, JoyRead};
int JoyTick(int state){
  switch(state){ //transition + action
    case(JoyStart):
      state = JoyRead;
      break;

    case(JoyRead):
      joyX = map(ADC_read(1), 0, 1023, 0, 100);
      joyY = map(ADC_read(0), 0, 1023, 0, 100);
      joyPress = (PINC & 0x04);
  }
  
  switch(state){ //action
  }

  return state;
}

// joyPress deletes notes
//left button records notes
//left held = play song
//right button changes octave
//right held = ...

enum ButtonStates {ButtonStart, ButtonIdle, ButtonRight, ButtonLeft, ButtonJoy};
int ButtonTick(int state){
  static unsigned char i = 0;
  switch(state){  //transition + action
    case(ButtonStart):
      state = ButtonIdle;
      break;
        
    case(ButtonIdle):
      if((PINC & 0x08)){state = ButtonLeft;}
      else if((PINC & 0x10)){state = ButtonRight;}
      else if(!joyPress){state = ButtonJoy;}
      break;
        
    case(ButtonRight):
      right = 1;
      if(!(PINC & 0x10)){
        switch(octave){
          case(0):
             octave = 1;
            break;
          case(1):
            octave = 2;
            break;
          case(2):
            octave = 0;
            break;
        }
       state = ButtonIdle;
      }
      break;

    case(ButtonLeft):
      left = 1;
      i++;
      if(!(PINC & 0x08)){
        if(i > 40){play = 1;}
        else{recording = 1;}
        state = ButtonIdle;
        i = 0;
      }
      break;
     
      
    case(ButtonJoy):
      if(joyPress){
        deleting = 1;
        state = ButtonIdle;
      }
    }
    return state;
}


//controls buzzer note during preview, paused during playback
//12 notes per octave, preview of note to record
enum PreviewStates {BuzzStart, BuzzIdle, BuzzPreview, BuzzPlay};
int PrevTick(int state){
  switch(state){  //transition
    case(BuzzStart):
      state = BuzzIdle;
      break;
    case(BuzzIdle):
      if(play){ 
        state = BuzzPlay;
      }
      else if(deadzone()){ //deadzone gives 1 when out of it btw
        state = BuzzPreview;
      }
      break;

    case(BuzzPreview):
      if(play){
        state = BuzzPlay;
      }
      if(!deadzone()){
        state = BuzzIdle;
      }
      break;

    case(BuzzPlay): //disable this task until playback is over.
      if(!play){
        state = BuzzIdle;
      }
      break;
    }

  switch(state){  //action
    case(BuzzIdle):
      OCR1A = 0;
      break;

    case(BuzzPreview):
      setNote(getAngle(joyX, joyY), octave);
      OCR1A = ICR1/2;
      break;

    case(BuzzPlay):
      
      break;
    }

    return state;
}


//plays back recorded notes
//for now, holds a placeholder array of notes. will use the actual notes array later.
enum PlaybackStates {PlayStart, PlayIdle, Playback};
int PlayTick(int state){
  static unsigned char i = 0;

  switch(state){ //transition
    case(PlayStart):
      state = PlayIdle;
      break;

    case(PlayIdle):
      if(play){
        state = Playback;
      }
      break;

    case(Playback):
      if(i >= 32 || recordedNotes[i] == 0){
        play = 0;
        i = 0;
        state = PlayIdle;
      }

  }

  switch(state){ //action
    case(Playback):
      setNote(recordedNotes[i], recordedOctave[i]);
      OCR1A = ICR1/2;
      i++;
      break;
  }

  return state;
}


enum InitStates {InitStart, Init_HWRESET, Init_SWRESET, Init_SLPOUT, Init_NoDelay, Init_DISPON, InitEnd};
int InitTick(int state){
  static unsigned char i = 0;
  
  switch(state){ // transitions
    case(InitStart):
      state = Init_HWRESET;
      break;

    //set reset to low, then high. 200ms each
    case(Init_HWRESET):
      if(i <= 20){
        PORTB = (PORTB & 0xFE);
        i++;
      }
      else if(i <= 40 && i > 20){
        PORTB = (PORTB & 0xFE) | 0x01;
        i++;
      }
      else if(i > 40){
        sendCom(0x01); //swreset
        i = 0;
        state = Init_SWRESET;
      }
      else{i++;}
      break;

    //send command, wait 150ms 
    case(Init_SWRESET):
      if(i > 15){
        sendCom(0x11); //slpout: sleep out
        i = 0;
        state = Init_SLPOUT;
      }
      else{i++;}
      break;
    
    //send command, wait 200ms
    case(Init_SLPOUT):
      if(i > 15){
        i = 0;
        state = Init_NoDelay;
      }
      else{i++;}
      break;

    //everything from here to dispon don't need delays 
    case(Init_NoDelay):
      sendCom(0xB1);  //FRMCTR1: sets framerate
      sendData(0x0C); //60hz
      sendData(0x14); //idle framerate?? idk what that means

      sendCom(0xB4);  //INVCTR: inversion control?
      sendData(0x07); //dot inversion

      sendCom(0xC0);  //PWCTR1: power control
      sendData(0x0C); //something something 4.2V
      sendData(0x05); //i just copied the init sequence, idk how this works

      sendCom(0x26);  //GAMMASET: Gamma enable
      sendData(0x04); //gamma curve = 1

      sendCom(0x3A);  //COLMOD: pixel format?
      sendData(0x05); //16-bit color mode?

      sendCom(0x36);  // MADCTL: Memory access control
      sendData(0xC8); // MX = 1, MY = 1, RGB = 0

      sendCom(0x29); //dispon
      state = Init_DISPON;
      break;

    //send command, wait 200ms
    case(Init_DISPON):
      if(i > 20){
        i = 0;
        state = InitEnd;
      }
      else{i++;}
      break;

    case(InitEnd):
      endInit = 1;
      break;
  }

  return state;
}


//IMPORTANT: screen retains color data, and must be cleared before every write
enum ScreenStates {ScreenStart, ScreenInit, ScreenIdle, ScreenDraw};
int ScreenTick(int state){
  static unsigned char i = 0;
  switch(state){ //transition
    case(ScreenStart):
      state = ScreenInit;
      break;

    case(ScreenInit):
      if(endInit == 1){
        //x0, y0, x1, y1, colorHigh, colorLow
        drawColor(0, 0, 127, 127, 0xFF, 0xFF); //background
        drawColor(32, 52, 96, 116, 0x00, 0x00); //note display bg
        drawColor(34, 54, 94, 114, 0xAF, 0x1F); //note display bg
        state = ScreenIdle;
      }
      break;

    case(ScreenIdle):
      /*example of how i can change something on the screen while staying in a state
        its a color-changing square
      if(deadzone()){
        spriteColHigh = 0x00;
        spriteColLow = 0x1F;
      }
      else{
        spriteColHigh = 0xF0;
        spriteColLow = 0x1F;
      }
      //84, 84, 118, 118
      drawColor(84, 84, 118, 118, spriteColHigh, spriteColLow);
      */
      if(recording || deleting){
        state = ScreenDraw;
      }
      if(deadzone()){
          drawColor(34, 54, 94, 114, 0xAF, 0x1F); //clear
          drawNote(getAngle(joyX, joyY));
      }
      break;

    case(ScreenDraw):
      if(i > 5){
        i = 0;
        state = ScreenIdle;
      }
      break;
  }

  switch(state){ //action
    case(ScreenDraw):
      if(recording){
        drawRecord();
      }
      else if(deleting){
        drawDelete();
      }
      i++;
      break;
  }

  return state;
}


//LED for displaying current mode
enum LEDStates {LEDStart, LEDIdle, LEDRecDel, LEDPlay};
int LEDTick(int state){
  static unsigned char i = 0;
  switch(state){ //transitions
    case(LEDStart):
      state = LEDIdle;
      break;
    
    case(LEDIdle):
      if(recording || deleting){
        state = LEDRecDel;
      }
      else if(play){
        state = LEDPlay;
      }
      break;

    case(LEDPlay):
      if(!play){
        state = LEDIdle;
      }
      break;

    case(LEDRecDel):
      if((!recording && !deleting) || i > 1){
        i = 0;
        state = LEDIdle;
      }
      break;

  }

  switch(state){  //actions
    case(LEDIdle):
      switch(octave){
        case(0): //red
          PORTD = (PORTD & 0b11100011) | 0b00010000;
          break;
          
        case(1): //blue
          PORTD = (PORTD & 0b11100011) | 0b00000100;
          break; 

        case(2): //green
          PORTD = (PORTD & 0b11100011) | 0b00001000;
          break; 
      }
      break;

    case(LEDPlay): //white
      PORTD = (PORTD & 0b11100011) | 0b00011100;
      break;

    case(LEDRecDel): //cyan
      PORTD = (PORTD & 0b11100011) | 0b00001100;
      i++;
      break;
  }

  return state;
}


//records note when button pressed, can't record any more than 32 notes.
enum RecordStates {RecordStart, RecordIdle, RecordNote, RecordDelete};
int RecTick(int state){
  static int currentNote = 0;
  static unsigned char noteRecorded = 0;

  if(deadzone()){
    currentNote = getAngle(joyX, joyY);
  }

  switch(state){  //transition
    case(RecordStart):
      state = RecordIdle;
      break;

    case(RecordIdle):
      if(recording){state = RecordNote;}
      else if(deleting){state = RecordDelete;}
      break;

    case(RecordNote):
      if(!recording){
        noteRecorded = 0;
        state = RecordIdle;
      }
      break;

    case(RecordDelete):
      if(!deleting){state = RecordIdle;}
      break;
  }

  switch(state){ //action
    case(RecordStart):
      break;

    case(RecordIdle):
      break;

    case(RecordNote):
      //noteAmount used as index, array of 32 can't index position 32, so this works.
      if(noteAmount < 32 && recording && !noteRecorded){
        recordedNotes[noteAmount] = currentNote;
        recordedOctave[noteAmount] = octave;
        noteAmount++;
      }
      recording = 0;
      noteRecorded = 1;
      break;

    case(RecordDelete):
      //refuses to delete if no notes exist
      if(noteAmount > 0 && deleting){
        recordedNotes[noteAmount - 1] = 0;
        noteAmount--;
      }
      deleting = 0;
      break;
  }

  return state;
}

int main(void) {
  //DDRC is all inputs, other two are all output
  //A0 for X, A1 for Y
  //A2 for joystick press
  //A3 and A4 for buttons
  DDRC    = 0x00;
  PORTC   = 0xFF;

  //screen clock on 13
  //screen MOSI on 11
  //buzzer on 9
  //screen reset on 8
  DDRB     = 0xFF;
  PORTB    = 0x00;

  //screen command on 7
  //screen backlight on 6
  //RGB LED on 4-2
  DDRD   = 0xFF;
  PORTD  = 0b01000000; //the red on my rgb led is running out btw

    ADC_init();   // initializes ADC


    //buzzer initialization on timer1
    TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8
    //WGM11, WGM12, WGM13 set timer to fast pwm mode

    //buzzer sets frequency for pitch
    //duty cycle stays at 50% for a square wave
    ICR1 = 2000;
    OCR1A = 0; //ICR1/2;

    //===============
    //task initialization
    //===============

    //joytstick
    tasks[0].period = JoyPeriod;
    tasks[0].state = JoyStart;
    tasks[0].elapsedTime = JoyPeriod;
    tasks[0].TickFct = &JoyTick;

    //buttons
    tasks[1].period = ButtonPeriod;
    tasks[1].state = ButtonStart;
    tasks[1].elapsedTime = ButtonPeriod;
    tasks[1].TickFct = &ButtonTick;

    //buzzer
    tasks[2].period = PrevPeriod;
    tasks[2].state = BuzzStart;
    tasks[2].elapsedTime = PrevPeriod;
    tasks[2].TickFct = &PrevTick;

    //screen initialization
    tasks[3].period = InitPeriod;
    tasks[3].state = InitStart;
    tasks[3].elapsedTime = InitPeriod;
    tasks[3].TickFct = &InitTick;    

    //screen draw
    tasks[4].period = ScreenPeriod;
    tasks[4].state = ScreenStart;
    tasks[4].elapsedTime = ScreenPeriod;
    tasks[4].TickFct = &ScreenTick; 

    //LED
    tasks[5].period = LEDPeriod;
    tasks[5].state = LEDStart;
    tasks[5].elapsedTime = LEDPeriod;
    tasks[5].TickFct = &LEDTick; 

    //playback
    tasks[6].period = PlayPeriod;
    tasks[6].state = PlayStart;
    tasks[6].elapsedTime = PlayPeriod;
    tasks[6].TickFct = &PlayTick; 

    //record/delete
    tasks[7].period = RecPeriod;
    tasks[7].state = RecordStart;
    tasks[7].elapsedTime = RecPeriod;
    tasks[7].TickFct = &RecTick;

    //===============
    // normal initialization
    //===============

    SPI_INIT();
    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {
    }

    return 0;
}