// #include <Keypad.h>
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1','2','3'},//,'A'},
  {'4','5','6'},//,'B'},
  {'7','8','9'}//,'C'},
  // {'*','0','#','D'}
};
byte target_number = 10;

// byte rowPins[ROWS] = {7, 6, 5};//, 6}; //connect to the row pinouts of the keypad
// byte colPins[COLS] = {4, 3, 2};//, 2}; //connect to the column pinouts of the keypad

//Create an object of keypad
// Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

byte rowPins[] = {4, 3, 2};
byte colPins[] = {5, 6, 7, 8, 9, 10};

void setup(){
  Serial.begin(9600);

  // Output to control LED
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
}
 
struct Matrix
{
  byte *column;
  byte *rows;
  byte mode;
};

struct LED
{
  int R; // HIGH or LOW
  int G; // HIGH or LOW
  int B; // HIGH or LOW
};

struct LED White = {LOW, LOW, LOW};
struct LED Red = {LOW, HIGH, HIGH};
struct LED Green = {HIGH, LOW, HIGH};
struct LED Blue = {HIGH, HIGH, LOW};
struct LED Off = {HIGH, HIGH, HIGH};


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

void toggle_active_row(int active_row) {
  for (int row = 0; row < 3; row++) {
    if (row == active_row) {
      digitalWrite(rowPins[row], HIGH);
    } else {
      digitalWrite(rowPins[row], LOW);
    }
  }
}

void multiplex_lights(struct Matrix *mat) {
  /* What is multiplexing? 
      - To address an individual LED, need to activate the columns with the appropriate values, then deactivate the row
  */

  struct LED target;
  struct LED others;

  if ((*mat).mode == 0) {
    target = White;
    others = Off;
  } else if ((*mat).mode == 1) {
    target = Green;
    others = Red;
  } else if ((*mat).mode == 2) {
    target = Red;
    others = Blue;
  } else {
    target = Blue;
    others = Green;
  }
  
  for (int row = 0; row < 3; row++) {
    if ((*mat).rows[row] == 1 || (*mat).mode != 0) {
      toggle_active_row(row);
      for (int col = 0; col < 3; col++) {
        if ((*mat).column[col] == 1) {
          digitalWrite(colPins[col*3], target.R);
          digitalWrite(colPins[(col*3)+1], target.G);
          digitalWrite(colPins[(col*3)+2], target.B);
        } else {
          digitalWrite(colPins[col*3], others.R);
          digitalWrite(colPins[(col*3)+1], others.G);
          digitalWrite(colPins[(col*3)+2], others.B);
        }
      }
    } else { // if no LED to turn on, set row and all columns to off.
      digitalWrite(colPins[0], LOW);
      digitalWrite(colPins[1], LOW);
      digitalWrite(colPins[2], LOW);
      digitalWrite(colPins[3], LOW);
      digitalWrite(colPins[4], LOW);
      digitalWrite(colPins[5], LOW);
      digitalWrite(rowPins[row], LOW);
    }
  }
}

void loop() {
  struct Matrix mat;
  byte col_mask[] = {0,1,0};
  byte row_mask[] = {0,1,0};
  mat.mode = 1;
  mat.column = col_mask;
  mat.rows = row_mask;

  multiplex_lights(&mat);

}