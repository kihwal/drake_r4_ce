#include <si5351.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#define DRAKE_R4_BAND_LIMIT_LO 1260000000ULL
#define DRAKE_R4_BAND_LIMIT_HI 4060000000ULL
#define DRAKE_R4_500KHZ          50000000ULL

#define DRAKE_R4_INITIAL_FREQ  2510000000ULL
#define DRAKE_R4_OFFSET        1110000000ULL

#define DRAKE_R4_30M_SAFE_TX   2080000000ULL
#define DRAKE_R4_30M_NOMINAL   2010000000ULL

#define DRAKE_R4_UP_A                 PIN_F0
#define DRAKE_R4_DN_A                 PIN_F1
#define DRAKE_R4_UP_B                 PIN_F4
#define DRAKE_R4_DN_B                 PIN_F5
#define DRAKE_R4_SYNC_ATOB            PIN_F6
#define DRAKE_R4_SYNC_BTOA            PIN_F7

Si5351 si5351;
LiquidCrystal lcd(12, 11, 23, 4, 3, 2);
unsigned long long curr_freq[2];

/** return the enum corresponding to the channel number */
si5351_clock get_clk_num(int channel) {
  return channel == 0 ? SI5351_CLK0 : SI5351_CLK1;
}

void update_display() {
  // freq
  for (int i = 0; i < 2; i++) {
    // rx/tx frequency
    int freq = (int)((curr_freq[i] - DRAKE_R4_OFFSET)/10000000);
    int mhz = freq/10;
    lcd.setCursor(0, i);
    // lcd print is slow with the 4pin api. 
    // Construct the string and call print once.
    lcd.print((i==0) ? "A: " : "B: "); // channel header
    if (mhz < 10) {
      lcd.print(" "); // leading space, if necessary.
    }
    lcd.print(mhz);
    lcd.print(".");
    lcd.print(freq%10);
    lcd.print(" MHz ");
  }
}

void update_help_msg() {
  // band info. not for tx, etc.
}

/**
 * channel 0 for A, 1 for B
 * up 0 for down, 1 for up
 */
void update_frequency(int channel, int up) {
  if (up) {
    if (curr_freq[channel] >= DRAKE_R4_BAND_LIMIT_HI) {
      // wrap around
      curr_freq[channel] = DRAKE_R4_BAND_LIMIT_LO;
    } else {
      curr_freq[channel] += DRAKE_R4_500KHZ;
    }
  } else {
    if (curr_freq[channel] <= DRAKE_R4_BAND_LIMIT_LO) {
      // wrap around
      curr_freq[channel] = DRAKE_R4_BAND_LIMIT_HI;
    } else {
      curr_freq[channel] -= DRAKE_R4_500KHZ;
    }
  }
  si5351.set_freq(curr_freq[channel], 0ULL, get_clk_num(channel));
}

/** src 0 for channel A, 1 for B */
void sync_freq(int src) {
  if (src) { // B to A
    curr_freq[0] = curr_freq[1];
    si5351.set_freq(curr_freq[0], 0ULL, get_clk_num(0));
  } else { // A to B
    curr_freq[1] = curr_freq[0];
    si5351.set_freq(curr_freq[1], 0ULL, get_clk_num(1));
  }
}

void setup() {
  // Start serial and initialize the Si5351
  Serial.begin(57600);
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0);

  // Initialize the display
  lcd.begin(16, 2);
  lcd.print("Drake R4/T4X");
  lcd.setCursor(0,1);
  lcd.print("Synthesizer");

  // Setup the output
  si5351.output_enable(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK1, 1);
  si5351.output_enable(SI5351_CLK2, 0);
  for (int i = 0; i < 2; i++) {
    curr_freq[i] = DRAKE_R4_INITIAL_FREQ;
    si5351.set_freq(curr_freq[i], 0ULL, get_clk_num(i)); 
  }
  
  // setup the input
  pinMode(DRAKE_R4_UP_A, INPUT_PULLUP);
  pinMode(DRAKE_R4_DN_A, INPUT_PULLUP);
  pinMode(DRAKE_R4_UP_B, INPUT_PULLUP);
  pinMode(DRAKE_R4_DN_B, INPUT_PULLUP);
  pinMode(DRAKE_R4_SYNC_ATOB, INPUT_PULLUP);
  pinMode(DRAKE_R4_SYNC_BTOA, INPUT_PULLUP);
  delay(300);

  // overwrite the initial screen
  update_display();
}


void loop() {
  int up_a = digitalRead(DRAKE_R4_UP_A);
  int dn_a = digitalRead(DRAKE_R4_DN_A);
  int up_b = digitalRead(DRAKE_R4_UP_B);
  int dn_b = digitalRead(DRAKE_R4_DN_B);
  int sync_atob = digitalRead(DRAKE_R4_SYNC_ATOB);
  int sync_btoa = digitalRead(DRAKE_R4_SYNC_BTOA);

  // channel A
  if (!up_a && dn_a) { // up A
    update_frequency(0, 1);
  } else if (up_a && !dn_a) { // dn A
    update_frequency(0, 0);
  }
  
  // channel B
  if (!up_b && dn_b) { // up B
    update_frequency(1, 1);
  } else if (up_a && !dn_a) { // dn B
    update_frequency(1, 0);
  }
  
  // sync
  if (!sync_atob && sync_btoa) {
    sync_freq(0);
  } else if (sync_atob && !sync_btoa) {
    sync_freq(1);
  }
  update_display();
  delay(200);
}
