// Copyright (c) 2022 David Such
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

/**********************
  @file    DShot_SPI.ino
  @brief   Testing DShot packet transmission using SPI.
  @author  David Such
  Code:        David Such
  Version:     1.0
  Date:        19/05/20
  1.0 Original Release          19/05/20
**********************/

#include <SPI.h>

//  System defined UNO SPI PINS
//  SS:   pin 10
//  MOSI: pin 11
//  MISO: pin 12
//  SCK:  pin 13

//  DShot starting test frame = throttle (11 bits) + telemetry request (1 bit)
//    0: 10 SPI bits ON, 17 SPI bits OFF = 0x07FE 0000
//    1: 20 SPI bits ON, 7 SPI bits OFF  = 0x07FF FF80
uint16_t frame = 0;
uint16_t throttle = 0b10101010101; // 0x555 or 1365 Decimal (68% throttle)
uint32_t DSHOT0 = 0x07FE0000;
uint32_t DSHOT1 = 0x07FFFF80;
bool telemetry = 0;


void setup() {
  //  Define PIN modes & Disable SPI
  pinMode(SS, OUTPUT);
  digitalWrite (SS, HIGH);

  // Start the SPI library
  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

  // Append telemetry to throttle
  if (valid_throttle(throttle)) {
    frame = (throttle << 1) | telemetry;
  }
}

void loop() {
  //  With a fixed frame we could just do this once in setup
  int csum = checksum(frame);
  int mask = 0b10000000;

  // append checksum to test_frame
  frame = (frame << 4) | csum;

  //  Slave Select - Active LOW
  digitalWrite (SS, LOW);
  for (byte i = 0; i < 15; i++) {
    if (frame & mask) {
      dShot_1();
    }
    else {
      dShot_0();
    }
    mask = mask >> 1;
  }
  digitalWrite (SS, HIGH);
  
  delay(200);
}

//  DShot Functions
//  For Arduino UNO (16MHz CPU),
//    0: 10 SPI bits ON, 17 SPI bits OFF = 0x07FE 0000
//    1: 20 SPI bits ON, 7 SPI bits OFF  = 0x07FF FF80
//  Total 27 SPI bits for every DShot bit

void dShot_0() {
  //SPI.transfer16(0x07FE);
  //SPI.transfer16(0x0000);
  SPI.transfer(&DSHOT0, 4);
}

void dShot_1() {
  //SPI.transfer16(0x07FF);
  //SPI.transfer16(0xFF80);
  SPI.transfer(&DSHOT1, 4);
}

bool valid_throttle(uint16_t throttle) {
  //  Note throttle values < 47 are special commands.
  //  See notes in article.
  if (throttle > 0 && throttle < 2048) {
    return true;
  }

  return false;
}

int checksum(uint16_t packet) {
  // compute checksum
  int csum = 0;
  int csum_data = packet;
  
  for (int i = 0; i < 3; i++) {
    csum ^= csum_data; // xor data by nibbles
    csum_data >>= 4;
  }
  
  csum &= 0xf;

  return csum;
}