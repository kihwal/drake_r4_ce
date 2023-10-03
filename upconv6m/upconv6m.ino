/*
 * An upconverter LO for 6m rigs such as SWAN 250.
 * Copyright (C) 2023 by Kihwal Lee, K9SUL
 * 
 * Since the 250 is USB-only, an inverting LO signal is needed for LSB reception.
 * When an inverting LO signal is used, the dial tunes backward. The IF tuning
 * frequency range is assumed to be 50.0-50.5MHz, with the exception of 10m, which
 * is 50.0-52.7MHz.
 * 
 *      USB LO   LSB LO
 * 1.5   48.5      52   (2.0 - 1.5)
 * 3.5   46.5      54   (4.0 - 3.5)
 * 7     43        57.5 (7.5 - 7.0)
 * 10    40        60   (10.0 - 9.5)
 * 14    36        64.5 (14.5 - 14.0)
 * 15    35        65   (15.0 - 14.5)
 * 18    32        68.5 (18.5 - 18.0)
 * 21    29        71.5 (21.5 - 21.0)
 * 24.5  25.5      75   (25.0 - 24.5)
 * 28    22        78.5 (28.5 - 28.0)
 * 
 * The band display is https://www.adafruit.com/product/881, which is interfaced
 * via i2c, address 0x70.
 * The si5351a board is also interfaced via i2c, address 0x60.
 * These are connected to the processor's i2c bus.
 * 
 * Three additional buttons are used. Change the definition of the pins. I am using
 * a Teensy 2.0, pin 13, 14 and 15.
 * 
 * PIN_B6 (15) band up
 * PIN_B5 (14) band down
 * PIN_B4 (13) LSB/USB
 */

// Buttons. Teensy 2.0
#define UCONV_BAND_UP  PIN_B6
#define UCONV_BAND_DN  PIN_B5
#define UCONV_SIDEBAND PIN_B4
#define UCONV_MODE_USB 0
#define UCONV_MODE_LSB 1
#define UCONV_NUM_BANDS 10

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <si5351.h>
#include <string.h>

Adafruit_7segment matrix = Adafruit_7segment();
Si5351 si5351;

// Digit 0 is used for U or L.
// Digit 1, 3 & 4 are for the band freq. Digit 2, colon, is not used
// Band definitions. There are 10 bands.
uint8_t digit1[UCONV_NUM_BANDS] = { 0, 0, 0, 1, 1, 1, 1, 2, 2, 2 };
uint8_t digit3[UCONV_NUM_BANDS] = { 1, 3, 7, 0, 4, 5, 8, 1, 4, 8 };
uint8_t digit4[UCONV_NUM_BANDS] = { 5, 5, 0, 0, 0, 0, 0, 0, 5, 0 };
// usb LO freq
uint64_t lo_freq_usb[UCONV_NUM_BANDS] =
  { 4850000000, 4650000000, 4300000000, 4000000000, 3600000000,
    3500000000, 3200000000, 2900000000, 2550000000, 2200000000 };

uint64_t lo_freq_lsb[UCONV_NUM_BANDS] = 
  { 5200000000, 5400000000, 5750000000, 6000000000, 6450000000,
    6500000000, 6850000000, 7150000000, 7500000000, 7850000000 };
uint8_t curr_band = 4; // 20m
uint8_t curr_mode = UCONV_MODE_USB;



void setup() {
  matrix.begin(0x70);

  // Initialize the clock generator
  if (!si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0)) {
    matrix.println("err1");
    matrix.writeDisplay();
    while(1); // cannot proceed
  }
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);

  si5351.output_enable(SI5351_CLK0, 1);
  
  // buttons
  pinMode(UCONV_BAND_UP,  INPUT_PULLUP);
  pinMode(UCONV_BAND_DN,  INPUT_PULLUP);
  pinMode(UCONV_SIDEBAND, INPUT_PULLUP);

  // splash
  scroll_led((char*)"UPCONVERTER 50 by K9SUL");
  update_freq();
  update_led();
}

void loop() {
  // band up
  if (!digitalRead(UCONV_BAND_UP)) {
    curr_band = (curr_band + 1) % UCONV_NUM_BANDS;
    update_freq();
    update_led();
    delay(300);
  }
  
  // band down
  if (!digitalRead(UCONV_BAND_DN)) {
    if (curr_band == 0) {
      curr_band = UCONV_NUM_BANDS;
    }
    curr_band--;
    update_freq();
    update_led();
    delay(300);
  }

  // mode
  if (!digitalRead(UCONV_SIDEBAND)) {
    curr_mode = (curr_mode == UCONV_MODE_USB) ? UCONV_MODE_LSB : UCONV_MODE_USB;
    update_freq();
    update_led();
    delay(300);
  }

}

// Update the mode and band on LED
void update_led() {
  matrix.writeDigitAscii(0, (curr_mode == UCONV_MODE_USB) ? 'U' : 'L', false);
  // do not display the leading zero.
  if (digit1[curr_band] == 0) {
    matrix.writeDigitAscii(1, ' ', false);
  } else {
    matrix.writeDigitNum(1, digit1[curr_band], false);
  }
  matrix.writeDigitNum(3, digit3[curr_band], true);
  matrix.writeDigitNum(4, digit4[curr_band], false);
  matrix.writeDisplay();
}

void update_freq() {
  if (curr_mode == UCONV_MODE_USB) {
    si5351.set_freq(lo_freq_usb[curr_band], SI5351_CLK0);
  } else {
    si5351.set_freq(lo_freq_lsb[curr_band], SI5351_CLK0);
  }
}
// Scroll a message. Used for the initial spalsh screen
void scroll_led(char* input) {
  char disp[4] = { ' ', ' ', ' ', ' ' };
  int len = strlen(input);
  for (int i = 0; i < len + 4; i++) {
    // shift the content
    disp[0] = disp[1];
    disp[1] = disp[2];
    disp[2] = disp[3];

    // add a new char
    if (i < len) {
      disp[3] = input[i];
    } else {
      disp[3] = ' '; // flush out
    }
    matrix.writeDigitAscii(0, disp[0], false);
    matrix.writeDigitAscii(1, disp[1], false);
    matrix.writeDigitAscii(3, disp[2], false);
    matrix.writeDigitAscii(4, disp[3], false);
    matrix.writeDisplay();
    delay(250);
  }
}
