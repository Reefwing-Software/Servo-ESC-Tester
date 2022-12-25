// Copyright (c) 2022 David Such
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

/**********************
  @file    Servo_Shield.ino
  @brief   Firmware for the prototype Multiprotocol Servo UNO Shield Tester.
  @author  David Such
  Code:        David Such
  Version:     1.2
  Date:        05/05/20
  1.0 Original Release          26/03/20
  1.1 PWM 490 Hz protocol added 03/04/20
  1.2 OneShot125 protocol added 05/05/20
**********************/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD I2C address to 0x3F with 16 chars and a 2 line display
LiquidCrystal_I2C lcd(0x3F, 16, 2);

//  Create a custom LCD character
uint8_t arrow[8] = {  0x1, 0x1, 0x5, 0x9, 0x1f, 0x8, 0x4};

//PIN CONNECTIONS

//  POTENTIOMETER INPUT
const byte POT = A0;

//  PWM OUTPUT
const byte PWM = 9;

//  PUSH BUTTON INPUTS
const byte PB_DEC = 8;          // Decrement button
const byte PB_INC = 7;          // Increment button
const byte PB_SLT = 6;          // Select Button
const byte PB_BCK = 5;          // Back Button

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
const int ONESHOT125 = 4;
const int MIN_PULSE_WIDTH = 800;
const int MAX_PULSE_WIDTH = 2200;
const int MIN_ONESHOT_WIDTH = 125;
const int MAX_ONESHOT_WIDTH = 250;
const int UNDEFINED = 9999;
//END GLOBAL CONSTANTS

//GLOBAL VARIABLES
int oldPotValue = UNDEFINED;
int oldIndex = UNDEFINED;
int index = 0;
int maxIndex = 5;
int pulseWidth = 1500;
int period[] = {20000, 8000, 4000, 2040, 1000};
String protocol[] = {"1. PWM 50Hz", "2. PWM 125Hz", "3. PWM 250Hz", "4. PWM 490Hz", "5. OneShot125", "6. DShot150"};
MENU_LEVEL menu = Home;
//END GLOBAL VARIABLES

void setup() {
  lcd.begin();                    // Initialize the LCD
  lcd.backlight();                // Turn on the backlight
  lcd.print("Multiprotocol");
  lcd.setCursor(0, 1);            // Move cursor to start of bottom line (0 is the index of the top line, and first character of each line)
  lcd.print("ESC/Servo Tester");
  delay(4000);

  pinMode(PWM, OUTPUT);
  pinMode(PB_DEC, INPUT_PULLUP);   //  Push buttons are ACTIVE LOW
  pinMode(PB_INC, INPUT_PULLUP);
  pinMode(PB_SLT, INPUT_PULLUP);
  pinMode(PB_BCK, INPUT_PULLUP);

  // Enable phase and frequency correct PWM on Timer 1

  TCCR1A = 0;                              //  Clear TCCR A & B bits
  TCCR1B = 0;
  TCCR1A = (1 << COM1A1);                  //  Non-inverting output
  TCCR1B = (1 << WGM13)  | (1 << CS11);    //  Phase & Freq correct PWM, prescaler N = 8
  ICR1 = 20000;                            //  Default Freq = 50 Hz
  OCR1A = 0;
}

void loop() {
  //  INPUT Handlers
  int potValue = analogRead(POT);
  boolean refreshLCD = false;
  boolean incrementPressed = (digitalRead(PB_INC) == ButtonDown);
  boolean decrementPressed = (digitalRead(PB_DEC) == ButtonDown);
  boolean selectPressed = (digitalRead(PB_SLT) == ButtonDown);
  boolean backPressed = (digitalRead(PB_BCK) == ButtonDown);
  
  delay(50);                                // Debounce switches
  
  if (menu == Home) {
    
    OCR1A = 0;                              //  PWM OFF, pulse width = 0
    
    if (incrementPressed && digitalRead(PB_INC) == ButtonUp) {
      index++;
    }
    if (decrementPressed && digitalRead(PB_DEC) == ButtonUp) {
      index--;
    }
    
    if (index < 0) {
      index = maxIndex;
    }
    else if (index > maxIndex) {
      index = 0;
    }

    if (index != oldIndex) {
      lcd.clear();
      lcd.print("Select Protocol:");
      lcd.setCursor(0, 1);
      lcd.print(protocol[index]);
      oldIndex = index;
    }

    if (selectPressed && index < 4 && digitalRead(PB_SLT) == ButtonUp) {
      menu = PulseWidth;
      pulseWidth = 1500;
      ICR1 = period[index];
      OCR1A = 1500;
      lcd.clear();
      lcd.print(protocol[index]);
      lcd.setCursor(0, 1);
      lcd.print("Width: 1500");
      delay(250);
    }
    else if (selectPressed && index == ONESHOT125 && digitalRead(PB_SLT) == ButtonUp) {
      //  OneShot125 Selected
      menu = PulseWidth;
      pulseWidth = 188;
      ICR1 = period[index];
      OCR1A = 188;
      lcd.clear();
      lcd.print(protocol[index]);
      lcd.setCursor(0, 1);
      lcd.print("Width: 188");
      delay(250);
    }

  } //END HOME MENU
  
  else if (menu == PulseWidth) {

    if (backPressed && digitalRead(PB_BCK) == ButtonUp) {
      menu = Home;
      oldIndex = UNDEFINED;
    }
    else if (incrementPressed && digitalRead(PB_INC) == ButtonUp) {
      if (index == ONESHOT125) {
        pulseWidth = constrain(pulseWidth++, MIN_ONESHOT_WIDTH, MAX_ONESHOT_WIDTH);
      }
      else {
        pulseWidth = constrain(pulseWidth++, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
      }
      refreshLCD = true;
    }
    else if (decrementPressed && digitalRead(PB_DEC) == ButtonUp) {
      if (index == ONESHOT125) {
        pulseWidth = constrain(pulseWidth--, MIN_ONESHOT_WIDTH, MAX_ONESHOT_WIDTH);
      }
      else {
        pulseWidth = constrain(pulseWidth--, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
      }
      refreshLCD = true;
    }
    else if (oldPotValue != potValue) {
      if (index == ONESHOT125) {
        pulseWidth = map(potValue, 0, 1023,  MIN_ONESHOT_WIDTH, MAX_ONESHOT_WIDTH);
      }
      else {
        pulseWidth = map(potValue, 0, 1023,  MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
      }
      oldPotValue = potValue;
      refreshLCD = true;
    }

    if (refreshLCD) {
      lcd.clear();
      lcd.print(protocol[index]);
      lcd.setCursor(0, 1);
      lcd.print("Width:");
      lcd.setCursor(7, 1);
      lcd.print(pulseWidth);
      OCR1A = pulseWidth;
      delay(250);                 //  Remove LCD jitter
    }

  } //END PULSE WIDTH MENU 
}