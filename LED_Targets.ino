#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>

#include <SPI.h>
#include <Keypad.h>

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};


// Shift Register
int shift_pins = 0;
boolean target_shifted = false;

int array_count = 2;
int dataPins[] = {1, 4};
int clockPins[] = {2, 5};
int latchPins[] = {0, 3};

byte rowPins[ROWS] = {13, 12, 11, 10}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad

//Create an object of keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

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
  for (int idx=0; idx < array_count; idx++) {
    shift_out(latchPins[idx], dataPins[idx], clockPins[idx], 0);
    shift_out(latchPins[idx], dataPins[idx], clockPins[idx], 0);
    delayMicroseconds(5000);
    //turning on the leds.
    shift_out(latchPins[idx], dataPins[idx], clockPins[idx], 4095);
    shift_out(latchPins[idx], dataPins[idx], clockPins[idx], 4095);
    delayMicroseconds(5000);
    //turning the leds back off.
    shift_out(latchPins[idx], dataPins[idx], clockPins[idx], 0);
    shift_out(latchPins[idx], dataPins[idx], clockPins[idx], 0);
  }
 }

/* control bitmasks:
    Colour  | Array  | LED
    |XX|....|XX|.....|XXXX|

    Colour:
    00.XXXXXX  => Single LED (White?)
    01.XXXXXX  => Green target, rest red
    10.XXXXXX  => Red target, rest blue
    11.XXXXXX  => Blue target, rest green

    Array:
    XX.00.XXXX => Top
    XX.01.XXXX => Mid
    XX.10.XXXX => Bottom

    Placement: Can compress location into 4 bits
    0000.0000  => RESET
    XXXX.0001  => Top Left
    XXXX.0010  => Top Centre
    XXXX.0011  => Top Right
    XXXX.0100  => Mid Left
    XXXX.0101  => Mid Centre
    XXXX.0110  => Mid Right
    XXXX.0111  => Bottom Left
    XXXX.1000  => Bottom Centre
    XXXX.1001  => Bottom Right
    XXXX.1010  => ALL

    Ideally manager app will just need to send this byte to identify the target appearance
*/

void process_target_pos(byte mask, int *target_col, int *target_row) {
  // can skip case of mask == 0, as covered in initial if statement
  // row
  switch (mask)
  {
    case B00000001:
    case B00000010:
    case B00000011:
      *target_row = 0;
      break;
    case B00000100:
    case B00000101:
    case B00000110:
      *target_row = 1;
      break;
    case B00000111:
    case B00001000:
    case B00001001:
      *target_row = 2;
      break;
    default:
      break;
  }
  // col
  switch (mask)
  {
    case B00000001:
    case B00000100:
    case B00000111:
      *target_col = 0;
      break;
    case B00000010:
    case B00000101:
    case B00001000:
      *target_col = 1;
      break;
    case B00000011:
    case B00000110:
    case B00001001:
      *target_col = 2;
      break;
    default:
      break;
  }
}

byte WHITE  = B00000111;
byte RED    = B00000001;
byte GREEN  = B00000010;
byte BLUE   = B00000100;
byte OFF    = B00000000;

// Derived from ShftOut13 from: https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
void generate_masks(byte mask, int *generated_masks) {

  if (mask == 0x0) { // Reset
    generated_masks[0] = 0;
    generated_masks[1] = 0;
    generated_masks[2] = 0;
    return;
  }
  // process mask
  byte mode = mask >> 6;

  byte target = RED;
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

  int new_shift_pins = (mask >> 4) & 3;
  target_shifted = new_shift_pins != shift_pins;
  shift_pins = new_shift_pins;

  int target_col = -1, target_row = -1; // Default all are target
  process_target_pos(mask & B00001111, &target_col, &target_row); 

  // From this data, we need to compose the 12bit mask, where the first 9 are 3xRGB (for columns 1-3), the last 3 being power for rows 1-3.

  for (int current_row = 0; current_row < 3; current_row++) {
    int pin_out = 0x00;

    if ((target_row == -1) || mode != 0 || current_row == target_row) {
      pin_out = pin_out | (0x1<<(2-current_row));
    }

    for (int current_column=2; current_column>=0; current_column--) {
      if ((target_col == -1) || (current_column == target_col && current_row == target_row)) {
        pin_out = pin_out | (target<<(current_column*3)+3);
      } else {
        pin_out = pin_out | (others<<(current_column*3)+3);
      }
    }
    generated_masks[current_row] = pin_out;
  }

}

void multiplex_leds(int latchPin, int dataPin, int clockPin, int *masks, int column_idx) {
  for (int idx=0; idx<array_count; idx++) {
    shift_out(latchPins[idx], dataPins[idx], clockPins[idx], 0);
  }
  shift_out(latchPin, dataPin, clockPin, masks[column_idx]);
}

int masks[] = {0,0,0};
int mask_idx = 0;
byte mode = 0;
byte position = 0;

void setup(){
  // Serial.begin(9600);
  // Output to control LED
  for (int idx=0; idx<array_count; idx++) {
    pinMode(dataPins[idx], OUTPUT);
    pinMode(clockPins[idx], OUTPUT);
    pinMode(latchPins[idx], OUTPUT);
  }

  reset();
}

void process_keypad() {
  char key = keypad.getKey();
  if (key) {
    byte shift_pin = shift_pins;
    switch (key) {  
      case 'A': // entire grid
        if (position == B00001010) {
          position = 0;
          mode = 0;
        } else {
          position = B00001010;
        }
        break;
      case 'B':
        reset();
        if (shift_pin >= array_count-1) {
          shift_pin = 0;
        } else {
          shift_pin++;
        }
        break;
      case '1':
        position = B00000001;
        break;
      case '2':
        position = B00000010;
        break;
      case '3':
        position = B00000011;
        break;
      case '4':
        position = B00000100;
        break;
      case '5':
        position = B00000101;
        break;
      case '6':
        position = B00000110;
        break;
      case '7':
        position = B00000111;
        break;
      case '8':
        position = B00001000;
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
        break;
    }

    generate_masks((mode << 6) | (shift_pin << 4) | position, masks);
  } 
}

// void process_ethernet() {

// }

void loop() {
  
  process_keypad();
  // process_ethernet();

  multiplex_leds(latchPins[shift_pins], dataPins[shift_pins], clockPins[shift_pins], masks, mask_idx);
  
  if (mask_idx > 1) { // want 2 branches of equal time to avoid flicker of LED rows
    mask_idx = 0;
  } else {
    mask_idx++;
  }
  delayMicroseconds(3000);
}