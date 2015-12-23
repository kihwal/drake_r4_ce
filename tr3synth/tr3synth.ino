/**
 * Copyright (C) 2015 Kihwal Lee K9SUL
 *
 * Band LO and carrier signal generator for the Drake TR-3/4.
 *
 */

//#define __DEBUG

#include <si5351.h>
#include <Wire.h>

// Carrier frequencies
#define DRAKE_TR3_CAR_CF            900155000ULL  // KVG LSB filter
//#define DRAKE_TR3_CAR_CF          899780000ULL  // KVG LSB filter

#define DRAKE_TR3_CAR_SB_OFFSET        130000ULL  // 1.55kHz + 150Hz
#define DRAKE_TR3_CAR_CW_OFFSET             0ULL  // CF on CW

// Band LO frequencies
#define DRAKE_TR3_LO_3MHZ           1800000000ULL  // 21.5 MHz

#define DRAKE_TR3_LO_7MHZ           2150000000ULL  // 21.5 MHz
#define DRAKE_TR3_LO_21MHZ          3550000000ULL  // 35.5 MHz
#define DRAKE_TR3_LO_28_0MHZ        4250000000ULL  // 42.5 MHz
#define DRAKE_TR3_LO_28_5MHZ        4300000000ULL  // 43.0 MHz
#define DRAKE_TR3_LO_29_1MHZ        4360000000ULL  // 43.6 MHz
#define DRAKE_TR3_LO_OFF           15000000000ULL

// Input pins for the band selection
// If none of these are set, it's on 20m or 80m, so the LO should shut down.
#define DRAKE_TR3_SEL_7MHZ          PIN_F0
#define DRAKE_TR3_SEL_21MHZ         PIN_F1
#define DRAKE_TR3_SEL_28_0MHZ       PIN_F4
#define DRAKE_TR3_SEL_28_5MHZ       PIN_F5
#define DRAKE_TR3_SEL_29_1MHZ       PIN_F6

// sideband selection
#define DRAKE_TR3_SEL_SSB_X         PIN_F7
#define DRAKE_TR3_SEL_CW_TX         PIN_B6

Si5351 si5351;

unsigned long long current_offset = -1ULL;
unsigned long long current_band_lo = DRAKE_TR3_LO_OFF;

void set_carrier(unsigned long long offset) {
  if (current_offset == offset) {
    return;
  }
  si5351.set_freq(DRAKE_TR3_CAR_CF + (offset), 0ULL, SI5351_CLK0);
  current_offset = offset;
}

void set_band_lo(unsigned long long freq) {
  // current_offset must be already set
  unsigned long long target_freq = freq; //-(current_offset);
  if (current_band_lo == target_freq) {
    return;
  }

/*
  // new freq specified.
  if (freq == DRAKE_TR3_LO_OFF) {
    // turn off the band lo
    si5351.output_enable(SI5351_CLK1, 0);
  } else if (current_band_lo <= DRAKE_TR3_LO_OFF) {
    // need to turn on the output, if it was off before
    si5351.output_enable(SI5351_CLK1, 1);
  }
*/
  si5351.set_freq(target_freq, 0ULL, SI5351_CLK1);
  current_band_lo = target_freq;
}

void setup() {
  // Start serial and initialize the Si5351
#ifdef __DEBUG
  Serial.begin(57600);
#endif
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0);

  // Setup the output
  si5351.output_enable(SI5351_CLK0, 1); // carrier is always on
  si5351.output_enable(SI5351_CLK1, 1); // turn off band lo initially
  si5351.output_enable(SI5351_CLK2, 0);

  // Setup the input pins
  pinMode(DRAKE_TR3_SEL_7MHZ, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_21MHZ, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_28_0MHZ, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_28_5MHZ, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_29_1MHZ, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_SSB_X, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_CW_TX, INPUT_PULLUP);
}


void loop() {
  // It must be possible to read a word and to bitwise check
  // but we will settle for this since efficiency is not important here.
  int sel_carr_x = !digitalRead(DRAKE_TR3_SEL_SSB_X);
  int sel_cw_tx = !digitalRead(DRAKE_TR3_SEL_CW_TX);

  int sel_band_7mhz = !digitalRead(DRAKE_TR3_SEL_7MHZ);
  int sel_band_21mhz = !digitalRead(DRAKE_TR3_SEL_21MHZ);
  int sel_band_28_0mhz = !digitalRead(DRAKE_TR3_SEL_28_0MHZ);
  int sel_band_28_5mhz = !digitalRead(DRAKE_TR3_SEL_28_5MHZ);
  int sel_band_29_1mhz = !digitalRead(DRAKE_TR3_SEL_29_1MHZ);

  // Set the carrier freq
  if (sel_cw_tx) {
    set_carrier(DRAKE_TR3_CAR_CW_OFFSET);
  } else if (sel_carr_x) {
    set_carrier(DRAKE_TR3_CAR_SB_OFFSET);
  } else {
    set_carrier(-DRAKE_TR3_CAR_SB_OFFSET);
  }

  // Set the band lo freq
  if (sel_band_7mhz) {
    set_band_lo(DRAKE_TR3_LO_7MHZ);
  } else if (sel_band_21mhz) {
    set_band_lo(DRAKE_TR3_LO_21MHZ);
  } else if (sel_band_28_0mhz) {
    set_band_lo(DRAKE_TR3_LO_28_0MHZ);
  } else if (sel_band_28_5mhz) {
    set_band_lo(DRAKE_TR3_LO_28_5MHZ);
  } else if (sel_band_29_1mhz) {
    set_band_lo(DRAKE_TR3_LO_29_1MHZ);
  } else {
    set_band_lo(DRAKE_TR3_LO_OFF);
  }

//si5351.output_enable(SI5351_CLK1, 1);
//si5351.set_freq(DRAKE_TR3_LO_7MHZ, 0ULL, SI5351_CLK1);


  delay(100);
}
