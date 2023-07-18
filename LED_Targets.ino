// #include <Key.h>
#include <Keypad.h>

// #include <Keypad.h>
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};


// Shift Register
// //Setting the pin connections for the data pins on the shift registers.
// int data_pin = 4;
// //setting the pins that the clock pins are connected to.
// int clock1 = 5;
// //setting the pins that the latch pins are connected to.
// int latch1 = 3;
int shift_pins = 1;

int dataPins[] = {1, 4};
int clockPins[] = {2, 5};
int latchPins[] = {0, 3};

byte rowPins[ROWS] = {13, 12, 11, 10}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad

//Create an object of keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

 //clearing the shift registers.
 void clearRegisters(int dataPin, int clockPin){
  digitalWrite(dataPin, 0);
  digitalWrite(clockPin, 0);
 }

 void shift_out(int latchPin, int dataPin, int clockPin, int mask) {
  //internal function setup
  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(latchPin, 0);
  digitalWrite(dataPin, 0);
  digitalWrite(clockPin, 0);

  int pinState = 0;

  //for each bit in the byte myDataOut&#xFFFD;
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights.
  for (int i=12; i>=0; i--)  {
    digitalWrite(clockPin, 0);
    //if the value passed to myDataOut and a bitmask result
    // true then... so if we are at i=6 and our value is
    // %11010100 it would the code compares it to %01000000
    // and proceeds to set pinState to 1.
    if (mask & (1<<i)) {
      pinState = 1;
    } else {
      pinState = 0;
    }

    //Sets the pin to HIGH or LOW depending on pinState
    digitalWrite(dataPin, pinState);
    //register shifts bits on upstroke of clock pin
    digitalWrite(clockPin, 1);
    //zero the data pin after shift to prevent bleed through
    digitalWrite(dataPin, 0);
    // delay(100);
  }

  //stop shifting
  digitalWrite(clockPin, 0);
  digitalWrite(latchPin, 1);
}
 
 //inspired by the shiftOut method provided on the arduino website here: http://arduino.cc/en/tutorial/ShiftOut
 void reset(){
   //turning off the leds.
   shift_out(latchPins[0], dataPins[0], clockPins[0], 0);
   shift_out(latchPins[0], dataPins[0], clockPins[0], 0);
   shift_out(latchPins[1], dataPins[1], clockPins[1], 0);
   shift_out(latchPins[1], dataPins[1], clockPins[1], 0);
   delay(500);
   //turning on the leds.
   shift_out(latchPins[0], dataPins[0], clockPins[0], 4095);
   shift_out(latchPins[0], dataPins[0], clockPins[0], 4095);
   shift_out(latchPins[1], dataPins[1], clockPins[1], 4095);
   shift_out(latchPins[1], dataPins[1], clockPins[1], 4095);
   delay(500);
   //turning the leds back off.
   shift_out(latchPins[0], dataPins[0], clockPins[0], 0);
   shift_out(latchPins[0], dataPins[0], clockPins[0], 0);
   shift_out(latchPins[1], dataPins[1], clockPins[1], 0);
   shift_out(latchPins[1], dataPins[1], clockPins[1], 0);
   clearRegisters(dataPins[0], clockPins[0]);
   clearRegisters(dataPins[1], clockPins[1]);
 }

/* control bitmasks:
    Mode (LED Colours)|Active Row|Active Column
    00.................000........000           = All off

    Just Positional:
    100 100 = Top Left
    100 010 = Mid Left
    100 001 = Bottom Left
    010 100 = Top Mid
    010 010 = Mid Mid
    010 001 = Bottom Mid
    001 100 = Top Right
    001 010 = Mid Right
    001 001 = Bottom Right

    Just Modes/Colours
    00 = Single LED (White?)
    01 = Green target, rest red
    10 = Red target, rest blue
    11 = Blue target, rest green

    Ideally manager app will just need to send this byte to identify the target appearance
*/

int process_target_pos(byte mask, boolean col) {
  int idx = 5;
  if (col) {
    idx = 2;
  }

  for (int i=idx; i>=idx-3; i--)  {
    if (mask & (1<<i)) {
      return idx - i + 1;
    }
  }
  return 0;
}

byte WHITE =  B00000111;
byte RED =    B00000100;
byte GREEN =  B00000010;
byte BLUE =   B00000001;
byte OFF =    B00000000;

// Derived from ShftOut13 from: https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
void generate_masks(byte mask, int *generated_masks) {
  // process mask
  byte mode = mask >> 6;

  byte target = WHITE;
  byte others = OFF;
   if (mode == 1) {
    target = GREEN;
    others = RED;
  } else if (mode == 2) {
    target = RED;
    others = BLUE;
  } else if (mode == 3) {
    target = BLUE;
    others = GREEN;
  }

  int target_col = process_target_pos(mask, true);
  int target_row = process_target_pos(mask, false);  

  // From this data, we need to compose the 12bit mask, where the first 9 are 3xRGB (for columns 1-3), the last 3 being power for rows 1-3.

  if (target_col == 0 && target_row == 0) {
    generated_masks[0] = 0;
    generated_masks[1] = 0;
    generated_masks[2] = 0;
  } else {
    target_col--;
    target_row--;
    for (int current_row = 0; current_row < 3; current_row++) {
      int pin_out = 0x00;

      if (mode != 0 || current_row == target_row) {
        pin_out = pin_out | (0x1<<(2-current_row));
      }

      for (int current_column=2; current_column>=0; current_column--) {
        if (current_column == target_col && current_row == target_row) {
          pin_out = pin_out | (target<<(current_column*3)+3);
        } else {
          pin_out = pin_out | (others<<(current_column*3)+3);
        }
      }
      generated_masks[current_row] = pin_out;
    }
  }
}

void multiplex_leds(int latchPin, int dataPin, int clockPin, int *masks, int column_idx){
  clearRegisters(dataPin, clockPin);
  shift_out(latchPin, dataPin, clockPin, masks[column_idx]);
}

int masks[] = {0,0,0};
int mask_idx = -1;

byte mode = 0;
byte position = 0;

void setup(){
  Serial.begin(9600);
  // Output to control LED
  pinMode(dataPins[0], OUTPUT);
  pinMode(clockPins[0], OUTPUT);
  pinMode(latchPins[0], OUTPUT);
  pinMode(dataPins[1], OUTPUT);
  pinMode(clockPins[1], OUTPUT);
  pinMode(latchPins[1], OUTPUT);

  reset();
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    Serial.println(key);
    clearRegisters(dataPins[0], clockPins[0]);
    clearRegisters(dataPins[1], clockPins[1]);
    // switch (key) { // row
    //   // case '4':
    //   case 'A':
    //     mode = 0;
    //     position = 0;
    //     Serial.println("Reset");
    //     break;
    //   case 'B':
    //     if (shift_pins > 0) {
    //       shift_pins = 0;
    //     } else {
    //       shift_pins++;
    //     }
    //     Serial.println("Matrix Change");
    //     break;
    //   case '1':
    //   case '2':
    //   case '3':
    //     position = B00100000;
    //     Serial.println("Top Row");
    //     break;
    //   case '4':
    //   case '5':
    //   case '6':
    //     position = B00010000;
    //     Serial.println("Mid Row");
    //     break;
    //   case '7':
    //   case '8':
    //   case '9':
    //     position = B00001000;
    //     Serial.println("Bottom Row");
    //     break;

    //   default:
    //     if (mode > 2) {
    //       mode = 0;
    //     } else {
    //       mode++;
    //     }
    //     Serial.println("Mode Change");
    //     break;
    // }
    // switch (key) { // column
    //   case '1':
    //   case '4':
    //   case '7':
    //     position = position | B00000100;
    //     Serial.println("Left Column");
    //     break;
    //   case '2':
    //   case '5':
    //   case '8':
    //     position = position | B00000010;
    //     Serial.println("centre Column");
    //     break;
    //   case '3':
    //   case '6':
    //   case '9':
    //     position = position | B00000001;
    //     Serial.println("Right Column");
    //     break;
    //   default:
    //     break;
    // }
    switch (key) { // row
      // case '4':
      case 'A':
        mode = 0;
        position = 0;
        Serial.println("Reset");
        break;
      case 'B':
        // if (shift_pins > 0) {
        //   shift_pins = 0;
        // } else {
        //   shift_pins++;
        // }
        position = B10111111;
        Serial.println("Matrix Change");
        break;
      case '1':
        position = B00100100;
        break;
      case '2':
        position = B00100010;
        break;
      case '3':
        position = B00100001;
        break;
      case '4':
        position = B00010100;
        break;
      case '5':
        position = B00010010;
        break;
      case '6':
        position = B00010001;
        break;
      case '7':
        position = B00001100;
        break;
      case '8':
        position = B00001010;
        break;
      case '9':
        position = B00001001;
        break;
      default:
        if (mode > 2) {
          mode = 0;
        } else {
          mode++;
        }
        Serial.println("Mode Change");
        break;
    }

    generate_masks(position | (mode << 6), masks);
    mask_idx++;
  } 

  multiplex_leds(latchPins[shift_pins], dataPins[shift_pins], clockPins[shift_pins], masks, mask_idx);
  
  if (mask_idx > 1) { // want 2 branches of equal time to avoid flicker of LED rows
    mask_idx = 0;
  } else {
    mask_idx++;
  }
}