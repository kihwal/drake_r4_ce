/**
 * Copyright (C) 2015 Kihwal Lee K9SUL
 *
 * Band LO signal generator for Drake R-4/T-4X series.
 * It uses two outputs of si5351, so it can drive two sets of TX and RX or
 * a TX and a RX separately. The convinient sync buttons allow quickly
 * synchronizing the two frequencies.
 *
 * Some bands are not recommended for transmitting.
 * 30m band TX is allowed with 20.8 MHz LO.
 */
#include <si5351.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#define DRAKE_R4_BAND_LIMIT_LO 1260000000ULL
#define DRAKE_R4_BAND_LIMIT_HI 4060000000ULL
#define DRAKE_R4_500KHZ          50000000ULL

#define DRAKE_R4_INITIAL_FREQ  2510000000ULL
#define DRAKE_R4_OFFSET        1110000000ULL

#define DRAKE_R4_30M_SAFE_TX   2080000000ULL
#define DRAKE_R4_30M_NOMINAL   2110000000ULL

#define DRAKE_R4_UP_A                 PIN_F0
#define DRAKE_R4_DN_A                 PIN_F1
#define DRAKE_R4_UP_B                 PIN_F4
#define DRAKE_R4_DN_B                 PIN_F5
#define DRAKE_R4_SYNC_ATOB            PIN_F6
#define DRAKE_R4_SYNC_BTOA            PIN_F7

//#define __DEBUG 

Si5351 si5351;
LiquidCrystal lcd(12, 11, 23, 4, 3, 2);
unsigned long long curr_freq[2];

/** return the enum corresponding to the channel number */
si5351_clock get_clk(int channel) {
  return channel == 0 ? SI5351_CLK0 : SI5351_CLK1;
}

/** get label for the band
 * freq is in the 100khz unit. e.g. 216 for 21.6MHz
 */
char* get_label(int freq) {
  switch(freq) {
    case 126:
      return " >1.80";
    case 151:
      return " <4.35";
    case 156:
      return " <4.70";
    case 201:
      return " <9.35";
    case 206:
      return " <9.75";
    case 211: // 10mhz no rx
      return ">10.25";
    case 131:
    case 136:
    case 161: // 5mhz
    case 166:
    case 216:
    case 221:
    case 226:
      return " no TX";
    default:
      return "      ";
  }
}

void update_display() {
  // freq
  for (int i = 0; i < 2; i++) {
    // rx/tx frequency
    int freq = (int)((curr_freq[i] - DRAKE_R4_OFFSET)/10000000);
    int mhz = freq/10;
    lcd.setCursor(0, i);
    lcd.print((i==0) ? "A " : "B "); // channel header
    if (mhz < 10) {
      lcd.print(" "); // leading space, if necessary.
    }
    lcd.print(mhz);
    lcd.print(".");
    lcd.print(freq%10);
    lcd.print(" MHz");
    lcd.print(get_label(freq+111));
  }
}


/**
 * channel 0 for A, 1 for B
 * up 0 for down, 1 for up
 */
void update_frequency(int channel, int up) {
#ifdef __DEBUG
  for (int i=0; i<2; i++) {
    Serial.write("BEFORE:");
    Serial.println((int)(curr_freq[i]/10000000));
  }
#endif
  if (up) {
    if (curr_freq[channel] >= DRAKE_R4_BAND_LIMIT_HI) {
      // wrap around
      curr_freq[channel] = DRAKE_R4_BAND_LIMIT_LO;
    } else if(curr_freq[channel] ==
      (DRAKE_R4_30M_NOMINAL-DRAKE_R4_500KHZ)) {
      // switch to the tx enabled 30m
      curr_freq[channel] = DRAKE_R4_30M_SAFE_TX;
    } else if(curr_freq[channel] == DRAKE_R4_30M_SAFE_TX) {
      // switch to the tx enabled 30m
      curr_freq[channel] = DRAKE_R4_30M_NOMINAL;
    } else {
      curr_freq[channel] += DRAKE_R4_500KHZ;
    }
  } else {
    if (curr_freq[channel] <= DRAKE_R4_BAND_LIMIT_LO) {
      // wrap around
      curr_freq[channel] = DRAKE_R4_BAND_LIMIT_HI;
    } else if(curr_freq[channel] == DRAKE_R4_30M_NOMINAL) {
      // switch to the tx enabled 30m
      curr_freq[channel] = DRAKE_R4_30M_SAFE_TX;
    } else if(curr_freq[channel] == DRAKE_R4_30M_SAFE_TX) {
      // switch to the tx enabled 30m
      curr_freq[channel] = DRAKE_R4_30M_NOMINAL-DRAKE_R4_500KHZ;
    } else {
      curr_freq[channel] -= DRAKE_R4_500KHZ;
    }
  }
#ifdef __DEBUG
  for (int i=0; i<2; i++) {
    Serial.write("AFTER:");
    Serial.println((int)(curr_freq[i]/10000000));
  }
#endif
  si5351.set_freq(curr_freq[channel], 0ULL, get_clk(channel));
#ifdef __DEBUG
  Serial.write("device:");
  Serial.print((int)(si5351.clk0_freq/10000000));
  Serial.write(",");
  Serial.println((int)(si5351.clk1_freq/10000000));
#endif
}

void show_spalsh() {
  lcd.clear();
  lcd.print("Drake R4/T4X");
  lcd.setCursor(0,1);
  lcd.print("Synthesizer 1.0");
}

/** src 0 for channel A, 1 for B */
void sync_freq(int src) {
  if (src) { // B to A
    curr_freq[0] = curr_freq[1];
    si5351.set_freq(curr_freq[0], 0ULL, get_clk(0));
  } else { // A to B
    curr_freq[1] = curr_freq[0];
    si5351.set_freq(curr_freq[1], 0ULL, get_clk(1));
  }
}

void setup() {
  // Start serial and initialize the Si5351
#ifdef __DEBUG
  Serial.begin(57600);
#endif
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0);

  // Initialize the display
  lcd.begin(16, 2);
  show_spalsh();
  
  // Setup the output
  si5351.output_enable(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK1, 1);
  si5351.output_enable(SI5351_CLK2, 0);
  for (int i = 0; i < 2; i++) {
    curr_freq[i] = DRAKE_R4_INITIAL_FREQ;
    si5351.set_freq(curr_freq[i], 0ULL, get_clk(i)); 
  }
  
  // setup the input
  pinMode(DRAKE_R4_UP_A, INPUT_PULLUP);
  pinMode(DRAKE_R4_DN_A, INPUT_PULLUP);
  pinMode(DRAKE_R4_UP_B, INPUT_PULLUP);
  pinMode(DRAKE_R4_DN_B, INPUT_PULLUP);
  pinMode(DRAKE_R4_SYNC_ATOB, INPUT_PULLUP);
  pinMode(DRAKE_R4_SYNC_BTOA, INPUT_PULLUP);
  delay(2000);

  // overwrite the initial screen
  lcd.clear();
  update_display();
}


void loop() {
  int up_a = digitalRead(DRAKE_R4_UP_A);
  int dn_a = digitalRead(DRAKE_R4_DN_A);
  int up_b = digitalRead(DRAKE_R4_UP_B);
  int dn_b = digitalRead(DRAKE_R4_DN_B);
  int sync_atob = digitalRead(DRAKE_R4_SYNC_ATOB);
  int sync_btoa = digitalRead(DRAKE_R4_SYNC_BTOA);
  int changed = 0;
  // channel A
  if (!up_a && dn_a) { // up A
    update_frequency(0, 1);
    changed =1;
#ifdef __DEBUG
    Serial.println("UP A");
#endif
  } else if (up_a && !dn_a) { // dn A
    update_frequency(0, 0);
    changed =1;
#ifdef __DEBUG
    Serial.println("DN A");
#endif
  }
  
  // channel B
  if (!up_b && dn_b) { // up B
    update_frequency(1, 1);
    changed =1;
#ifdef __DEBUG
    Serial.println("UP B");
#endif
  } else if (up_b && !dn_b) { // dn B
    update_frequency(1, 0);
    changed =1;
#ifdef __DEBUG
    Serial.println("DN B");
#endif
  }
  
  // sync
  if (!sync_atob && sync_btoa) {
    sync_freq(0);
    changed =1;
  } else if (sync_atob && !sync_btoa) {
    sync_freq(1);
    changed =1;
  } else if (!sync_atob && !sync_btoa) {
    show_spalsh();
    delay(2000);
    lcd.clear();
    changed =1;
  }
  if (changed) {
    update_display();
  }
  delay(200);
}
