#include "si5351.h"
#include "Wire.h"
#include <LiquidCrystal.h>


Si5351 si5351;
LiquidCrystal lcd(12, 11, 23, 4, 3, 2);
unsigned long long curr_freq;

void setup()
{
  // Start serial and initialize the Si5351
  Serial.begin(57600);
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0);

  // Set CLK0 to output 14 MHz with a fixed PLL frequency
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  lcd.begin(16, 2);
  lcd.print("K9SUL synth");

  si5351.output_enable(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK1, 0);
  si5351.output_enable(SI5351_CLK2, 0);

  curr_freq = 1110000000ULL;
  set_display_freq();
  
  pinMode(PIN_F0, INPUT_PULLUP);
  pinMode(PIN_F1, INPUT_PULLUP);
  pinMode(PIN_F4, INPUT_PULLUP);
}

void set_display_freq() {
  si5351.set_freq(curr_freq, 0ULL, SI5351_CLK0);
  lcd.setCursor(0,1);
  int freq_khz = (int)(curr_freq/100000);
  lcd.print(freq_khz/1000);
  lcd.print(".");
  lcd.print(freq_khz%1000);
}

void loop()
{
  if (!digitalRead(PIN_F0)) {
    if (si5351.clk0_freq != 1610000000ULL) {
      curr_freq = 1610000000ULL;
      set_display_freq();
    }
  } else if (!digitalRead(PIN_F1)) {
    if (si5351.clk0_freq != 2110000000ULL) {
      curr_freq = 2110000000ULL;
      set_display_freq();
    }
  } else if (!digitalRead(PIN_F4)) {
    if (si5351.clk0_freq != 1110000000ULL) {
      curr_freq = 1110000000ULL;
      set_display_freq();
    }
  }
  delay(100);
}
