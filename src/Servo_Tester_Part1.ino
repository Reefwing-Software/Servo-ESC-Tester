/**********************
  @file    Servo_Tester.ino
  @brief   Firmware for the protype Multiprotocol Servo Tester.
  @author  David Such
  Code:        David Such
  Version:     1.0
  Date:        02/03/20
**********************/

#include <LiquidCrystal.h>
#include <Encoder.h>

//PIN CONNECTIONS

//ENCODER
/*The encoder works best when used on pins with interrupt capability.
  For the Arduino Mega 2560, pins 2, 3, 18, 19, 20, 21 have interrupt
  capability, so connect A and B from the encoder interface to pins 20 & 21*/
const byte EncA = 20, EncB = 21;

Encoder enc(EncA, EncB);

//LCD
/*The LCD requires either 4 or 7 data pins and two more for 'RS' (register select)
  and 'E' (or EN, the Enable signal) and optionally 'BL' (backlight, although this
  can be wired directly to the 5V supply if the ability to adjust the backlight isn't
  needed). To save pins, we connect the LCD in 4-bit mode. There are no special
  requirements for these pins, although connecting BL to a PWM capable pin allows
  dimming of the backlight*/
const byte RS = 3, EN = 5, D4 = 6, D5 = 7, D6 = 8, D7 = 9, BL = 10;

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// PWM OUTPUT
const byte PWM = 11;

//  LEDs
const byte LED1 = 15;
const byte LED2 = 16;
const byte LED3 = 17;
const byte LED4 = 18;
const byte LED_RED = 2;
const byte LED_GREEN = 4;

//  PUSH BUTTON INPUTS
const byte PB_DEC = 54;          // Decrement button
const byte PB_INC = 55;          // Increment button
const byte PB_SLT = 56;          // Select Button
const byte PB_BCK = 57;          // Back Button

//END PIN CONNECTIONS

//ENUMS
enum BUTTON_STATE {
  ButtonDown,                   //  Push Buttons are ACTIVE LOW
  ButtonUp
};

enum MENU_LEVEL {
  Home,
  PulseWidth
};
//END ENUMS

//GLOBAL CONSTANTS
const int MIN_PULSE_WIDTH = 800;
const int MAX_PULSE_WIDTH = 2200;
//END GLOBAL CONSTANTS

//GLOBAL VARIABLES
long oldPosition = 999;
int index = -1;
int maxIndex = 4;
int pulseWidth = 1500;
int period[] = {20000, 8000, 4000};
String protocol[] = {"1. PWM 50Hz", "2. PWM 125Hz", "3. PWM 250Hz", "4. OneShot", "5. DShot"};
MENU_LEVEL menu = Home;
//END GLOBAL VARIABLES

void setup()
{
  lcd.begin(16, 2);              // Initalise the LCD for 16 characters and 2 rows
  lcd.clear();
  lcd.print("Multiprotocol");
  lcd.setCursor(0, 1);           // Move cursor to start of bottom line (0 is the index of the top line, and first character of each line)
  lcd.print("ESC/Servo Tester");
  analogWrite(BL, 255);

  enc.write(0);                  //  Reset the encoder's accumulated position
  delay(1000);

  pinMode(PWM, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(PB_DEC, INPUT_PULLUP);   //  Push buttons are ACTIVE LOW
  pinMode(PB_INC, INPUT_PULLUP);
  pinMode(PB_SLT, INPUT_PULLUP);
  pinMode(PB_BCK, INPUT_PULLUP);

  digitalWrite(LED_RED, HIGH);     //  Power ON indication
  digitalWrite(LED_BUILTIN, HIGH);

  // Start up sequence for push button LED's
  for (int led = LED1; led <= LED4; led++) {
    digitalWrite(led, HIGH);
    delay(500);
    digitalWrite(led, LOW);
  }

  // Enable phase and frequency correct PWM on Timer 1

  TCCR1A = 0;                              //  Clear TCCR A & B bits
  TCCR1B = 0;
  TCCR1A = (1 << COM1A1);                  //  Non-inverting output
  TCCR1B = (1 << WGM13)  | (1 << CS11);    //  Phase & Freq correct PWM, prescaler N = 8
  ICR1 = 20000;                            //  Freq = 50 Hz
  OCR1A = 0;                               //  PWM OFF. pulse width = 0
}

int checkEncoder(long newPosition) {
  
  int delta = 0;

  if (newPosition > oldPosition + 1) {
    delta--;
  }
  else if (newPosition < oldPosition - 1) {
    delta++;
  }

  return delta;
}

void loop()
{
  //  INPUT Handlers
  long newPosition = enc.read();
  int encoderDelta = checkEncoder(newPosition);
  boolean selectPressed = (digitalRead(PB_SLT) == ButtonDown);

  if (menu == Home) {

    index += encoderDelta;
    if (index < 0) {
      index = maxIndex;
    }
    else if (index > maxIndex) {
      index = 0;
    }

    if (oldPosition != newPosition) {
      lcd.clear();
      lcd.print("Select Protocol:");
      lcd.setCursor(0, 1);
      lcd.print(protocol[index]);
      oldPosition = newPosition;
    }

    if (selectPressed && index < 3 && digitalRead(PB_SLT) == ButtonUp) {
      digitalWrite(LED3, HIGH);
      menu = PulseWidth;
      pulseWidth = 1500;
      ICR1 = period[index];
      OCR1A = 1500;
      lcd.clear();
      lcd.print(protocol[index]);
      lcd.setCursor(0, 1);
      lcd.print("Width: 1500");
      delay(250);
      digitalWrite(LED3, LOW);
    }

  } //END HOME MENU

  else if (menu == PulseWidth) {

    if (oldPosition != newPosition) {
      pulseWidth = constrain(pulseWidth += encoderDelta, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
      lcd.clear();
      lcd.print(protocol[index]);
      lcd.setCursor(0, 1);
      lcd.print("Width:");
      lcd.setCursor(7, 1);
      lcd.print(pulseWidth);
      OCR1A = pulseWidth;
      oldPosition = newPosition;
    }

  } //END PULSE WIDTH MENU

}