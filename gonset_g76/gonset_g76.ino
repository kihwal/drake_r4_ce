/**
 * RX/TX VFO for Gonset G76 by K9SUL
 * 
 * For Teensy pin definitions, see
 * /Applications/Teensyduino.app/Contents/Java/
 *  hardware/teensy/avr/cores/teensy/core_pins.h
 *
 * == Hardware components ==
 * Teensy 2.0 board
 * Adafrout si5351a board
 * Two push button switches
 * 16x2 LCD
 * Rotary encoder
 * 
 * == Encoder and si5351a PLL ==
 * PIN_D0 SCL - si5351a control
 * PIN_D1 SDA - si5351a control
 * PIN_D2 - encoder A
 * PIN_D3 - encoder B
 * 
 * == LCD : 14 pin interface ==
 * LCD RS, En, D4, D5, D6, D7 are connected to Teensy 2.0 pins 0, 1, 2, 3, 4, 22.
 * LCD R/W is grounded.
 * LCD pin 3 is for contrast.
 *
 * == Other I/O ==
 * PIN_B6 (15) encoder button for tuning step
 * PIN_B5 (14) T/R signal. Turn off TX VFO while receiving.
 * PIN_B4 (13) button Band DOWN
 * PIN_D7 (12) button Band UP
 *
 * === Required signals ===
 * The Gonset G76 has two distinct VFOs for reception and transmission.
 * Typical of an AM equipment of the era, the TX frequency is obtained
 * by multiplying a 3.5MHz or 7MHz fundamental signal. The RX VFO is
 * a HFO supplying a signal that is 2065 kHz higher than the reception 
 * frequency.
 * 
 * Band    TX VFO                  RX VFO (F + 2065)
 * ---------------------------------------------
 * 80m   3.5 - 4.000 MHz   F     5.565 -  6.065 MHz
 * 40m   7.0 - 7.300 MHz   F     9.065 -  9.365 MHz
 * 20m   7.0 - 7.175 MHz (F/2)  16.015 - 16.415 MHz
 * 15m   7.0 - 7.150 MHz (F/3)  23.065 - 23.515 MHz
 * 10m   7.0 - 7.425 MHz (F/4)  30.065 - 31.765 MHz
 *  6m   8.344 - 9.0 MHz (F/6)  52.065 - 56.065 MHz
 *  
 * The TX VFO signal is fed to the xtal socket. 
 * The RX VFO signal is fed to the pin 9 of V5 through a coupling cap.
 * Disconnect the existing LC circuit.  The existing RX tuning will 
 * act as preselector tuning.
 *
 * The TX VFO signal is diabled when receiving. When PIN_B6 is grounded, 
 * the control will recognize it as transmit mode. If open, it will enter 
 * the receive mode and the TX VFO will be disabled.
 * 
 */
#include <Encoder.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <si5351.h>


#define G76_STEP_10HZ      10
#define G76_STEP_100HZ    100
#define G76_STEP_1KHZ    1000
#define G76_RX_IF_OFFSET 206500000 // 2065khz

#define G76_ENC_DIV         4  //works for the common detented encoder.

#define G76_BAND_80         0
#define G76_BAND_40         1
#define G76_BAND_20         2
#define G76_BAND_15         3
#define G76_BAND_10         4

uint64_t base_freq[5] = { 3547000, 7180000, 14286000, 21285000, 29000000 };
uint16_t cur_step = G76_STEP_1KHZ;
uint16_t cur_band = G76_BAND_40;

boolean isTx = false;

// PIN_D0 (SCL) and PIN_D1 (SDA) are used for si5351a control
Si5351 si5351;

// Interrupt pins are used for encoders for the best result
Encoder tuning_enc(PIN_D2, PIN_D3);

// LCD control is easier with the digital pins.
// pin 0, 1, 2, 3, 4, 5, 22
LiquidCrystal lcd(PIN_B0, PIN_B1, PIN_B2, PIN_B3, PIN_B7, PIN_D4);

uint64_t freq_disp = base_freq[cur_band]; // Power on default.

void setup() {
  lcd.begin(16, 2);
  pinMode(PIN_B6, INPUT_PULLUP); // enc button
  pinMode(PIN_B5, INPUT_PULLUP); // t/r (tx on ground)
  pinMode(PIN_B4, INPUT_PULLUP); // down
  pinMode(PIN_D7, INPUT_PULLUP); // up

  // Initialize the clock generator
  if (!si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0)) {
    lcd.print("Init error");
    while(1);
  }

  // Splash
  lcd.print("Gonset G76 VFO");
  delay(2000);

  // Display initial state
  lcd.clear();
  update_freq(true);
  lcd.setCursor(0,1);
  lcd.print("STEP: 1KHz");

  // set the initial osc output enable bits.
  isTx = false;
  switch_tr(isTx);
  si5351.output_enable(SI5351_CLK2, 0);
  si5351.output_enable(SI5351_CLK0, 1); // rx always on
}

int clicks = 0;
void loop() {
  // step button
  if (!digitalRead(PIN_B6)) {
    lcd.setCursor(0,1);
    switch(cur_step) {
      case G76_STEP_10HZ:
        cur_step = G76_STEP_100HZ;
        lcd.print("STEP: 100Hz");
        break;
      case G76_STEP_100HZ:
        cur_step = G76_STEP_1KHZ;
        lcd.print("STEP: 1KHz ");
        break;
      case G76_STEP_1KHZ:
        cur_step = G76_STEP_10HZ;
        lcd.print("STEP: 10Hz ");
        break;
    }
    lcd.setCursor(0,1);
    delay(300);
  }

  // band up
  if (!digitalRead(PIN_D7)) {
    // save the current band freq
    base_freq[cur_band] = freq_disp;
    if (cur_band == G76_BAND_10) {
      cur_band = G76_BAND_80;
    } else {
      cur_band++;
    }
    freq_disp = base_freq[cur_band];
    update_freq(true);
    delay(300);
  }

  // band down
  if (!digitalRead(PIN_B4)) {
    // save the current band freq
    base_freq[cur_band] = freq_disp;
    if (cur_band == G76_BAND_80) {
      cur_band = G76_BAND_10;
    } else {
      cur_band--;
    }
    freq_disp = base_freq[cur_band];
    update_freq(true);
    delay(300);
  }

  // T/R sensing. Debounce not needed for this.
  if (!digitalRead(PIN_B5)) {
    // we are here because the pin was grounded for TX
    if (!isTx) {
      // switched to TX
      isTx = true;
      switch_tr(isTx);
    }
    // alread in tx mode
  } else {
    // In RX mode.
    if (isTx) {
      // switched to RX
      isTx = false;
      switch_tr(isTx);
    }
  }

  // update if the encoder rotated sufficiently
  update_freq(false);

  
}

void switch_tr(boolean transmit) {
  // CLK0 : RX, CLK1 : TX, CLK2 : unused
  lcd.setCursor(14,1);
  if (transmit) {
    si5351.output_enable(SI5351_CLK1, 1);
    lcd.print("TX");
  } else {
    si5351.output_enable(SI5351_CLK1, 0);
    lcd.print("RX");
  }
}
  
void update_freq(boolean force) {
  int32_t freq_ctl = (int32_t)(tuning_enc.read()/G76_ENC_DIV);
  if (force || freq_ctl != 0) {
    freq_disp -= (cur_step * freq_ctl);
    // reset encoder
    tuning_enc.write(0);
    set_frequency(freq_disp); // set output frequency

    // update display
    lcd.setCursor(0,0);
    char buff[16];
    //sprintf(buff,"%10lu Hz", freq_disp>>10);
    //lcd.print(buff);
    int khz = (freq_disp/1000)%1000;
    int mhz = freq_disp/1000000;
    int hz = freq_disp%1000;
    sprintf(buff, "%4d.%03d.%03d Hz", mhz, khz, hz);
    lcd.print(buff);
  }
}

/*
 * Sets the output frequency.
 */
void set_frequency(uint64_t f) {
  uint64_t freq = f*100;
  // RX HFO freq : f + 2065 kHz
  si5351.set_freq(freq + G76_RX_IF_OFFSET, SI5351_CLK0);

  // TX freq
  switch(cur_band) {
    case G76_BAND_80:
    case G76_BAND_40:
      break;
    case G76_BAND_20:
      freq /= 2;
      break;
    case G76_BAND_15:
      freq /= 3;
      break;
    case G76_BAND_10:
      freq /= 4;
      break;
  }
  si5351.set_freq(freq, SI5351_CLK1);
}
