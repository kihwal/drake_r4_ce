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
#define DRAKE_TR3_CAR_CF            900155000ULL  // CF of the KVG USB filter
#define DRAKE_TR3_CAR_SB_OFFSET        135000ULL  // 1.35kc
#define DRAKE_TR3_CAR_CW_OFFSET             0ULL  // on CW

// Band LO frequencies. Calibrated for the error and offset
// For 40m-10m, dial does not need calibrated. 3.5 will be 2.7kc off.
#define DRAKE_TR3_LO_7MHZ           2149997100ULL  // 21.5 MHz
#define DRAKE_TR3_LO_21MHZ          3550263800ULL  // 35.5 MHz +2.7kc
#define DRAKE_TR3_LO_28_0MHZ        4250262600ULL  // 42.5 MHz +2.7kc
#define DRAKE_TR3_LO_28_5MHZ        4300262500ULL  // 43.0 MHz +2.7kc
#define DRAKE_TR3_LO_29_1MHZ        4360262400ULL  // 43.6 MHz +2.7kc
#define DRAKE_TR3_LO_OFF           15100000000ULL  // 151MHz.
// rather than turning on and off one output, setting it to a frequency
// seems more stable. Mixing products with 150MHz shouldn't cause any
// problem in this trx.

// Input pins for the band selection
// If none of these are set, it's on 20m or 80m.
#define DRAKE_TR3_SEL_7MHZ          PIN_F0
#define DRAKE_TR3_SEL_21MHZ         PIN_F1
#define DRAKE_TR3_SEL_28_0MHZ       PIN_F4
#define DRAKE_TR3_SEL_28_5MHZ       PIN_F5
#define DRAKE_TR3_SEL_29_1MHZ       PIN_F6

// sideband selection
#define DRAKE_TR3_SEL_SSB_X         PIN_F7
#define DRAKE_TR3_SEL_CW_TX         PIN_B5

Si5351 si5351;

unsigned long long current_carr = 0ULL;
unsigned long long current_band_lo = DRAKE_TR3_LO_OFF;

void set_carrier(unsigned long long freq) {
  if (current_carr == freq) {
    return;
  }
  si5351.set_freq(freq, 0ULL, SI5351_CLK0);
  current_carr = freq;
}

void set_band_lo(unsigned long long freq) {
  if (current_band_lo == freq) {
    return;
  }
  si5351.set_freq(freq, 0ULL, SI5351_CLK1);
  current_band_lo = freq;
}

void setup() {
  // initialize the Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0);

  // Setup the output
  si5351.output_enable(SI5351_CLK0, 1); // carrier
  si5351.output_enable(SI5351_CLK1, 1); // band lo
  si5351.output_enable(SI5351_CLK2, 0);

  // Setup the input pins
  pinMode(DRAKE_TR3_SEL_7MHZ, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_21MHZ, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_28_0MHZ, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_28_5MHZ, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_29_1MHZ, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_SSB_X, INPUT_PULLUP);
  pinMode(DRAKE_TR3_SEL_CW_TX, INPUT_PULLUP);

  // initial setup
  set_carrier(DRAKE_TR3_CAR_CF + DRAKE_TR3_CAR_CW_OFFSET);
  set_band_lo(DRAKE_TR3_LO_OFF);
}


void loop() {
  int sel_carr_x = !digitalRead(DRAKE_TR3_SEL_SSB_X);
  // Normally grounded. Floating(high) on CW.
  int sel_cw_tx = digitalRead(DRAKE_TR3_SEL_CW_TX);

  int sel_band_7mhz = !digitalRead(DRAKE_TR3_SEL_7MHZ);
  int sel_band_21mhz = !digitalRead(DRAKE_TR3_SEL_21MHZ);
  int sel_band_28_0mhz = !digitalRead(DRAKE_TR3_SEL_28_0MHZ);
  int sel_band_28_5mhz = !digitalRead(DRAKE_TR3_SEL_28_5MHZ);
  int sel_band_29_1mhz = !digitalRead(DRAKE_TR3_SEL_29_1MHZ);

  // Set the carrier freq
  if (sel_cw_tx) {
    set_carrier(DRAKE_TR3_CAR_CF + DRAKE_TR3_CAR_CW_OFFSET);
  } else if (sel_carr_x) {
    set_carrier(DRAKE_TR3_CAR_CF + DRAKE_TR3_CAR_SB_OFFSET);
  } else {
    set_carrier(DRAKE_TR3_CAR_CF - DRAKE_TR3_CAR_SB_OFFSET);
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
  delay(100);
}
