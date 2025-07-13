## Day 1
Log #1: Our main objective was to create a circuit that utilized the speakers. Our first hassle was the infinitely long lines of the undercity. Many scavengers scrounged the streets towards the hardware sanctuary.

*2 hours theorizing the perfect idea*

*Unfortunately* the large sign out front said **SINGLE FILE LINE**

This delayed the inevitable adventure, as our frantic petrifying speed made the components we need fly off back on the shelves. 

*1.5 hours spent in line*

## Day 2

Log #2: To create the mechanism, we constructed off a audio amplifier out of towers of solder, each pin at a time. Once finished the swarming spaghetti monster of breadboards consumed our work. The spaghetti was weak, and easily shifted. More importantly, the communication channels within, our **firmware**, had a fatal flaw. Evan worked on the first mechanism. *the button*. This was no ordinary button. As it refused to work despite every effort, neither towards staff member or any helping hand.

*4 hours exterminating the bugs within the code*

Log #3: The MCU being used for the project is the SEEED XIAO RP2040. We want to interface it with the MAX98357A amp.

Procedure: 
Reference the documentation on the [MCU](https://wiki.seeedstudio.com/XIAO-RP2040/) and the [amp](https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts).

Assign GPIO pins 2, 3, 4 to BCLK, LRC, and DIN respectively. The RP2040 driver enforces that LRC is connected to GPIO#+1 of the BCLK pin. 

GAIN can be left floating or connected to GND. SD can be left floating to output stereo average. 

Below is the firmware uploaded (modified from an example code).


```cpp
#include <I2S.h>

const int bclkPin = D2;   // BCLK
const int lrckPin = D3;   // LRC (WS)
const int dinPin  = D4;   // Data (SD)
const int butPin = D5;

const int frequency = 440; 
const int amplitude = 5000; 
const int sampleRate = 16000;

const int halfWavelength = (sampleRate / frequency); // half wavelength of square wave

int16_t sample = amplitude; // current sample value
int count = 0;

I2S i2s(OUTPUT);

void setup() {
  Serial.begin(115200);

  i2s.setBCLK(bclkPin);
  i2s.setDATA(dinPin);
  i2s.setBitsPerSample(16);

  if (!i2s.begin(44100)) {
    Serial.println("Failed to initialize I2S!");
    while (1);
  }
  pinMode(butPin, INPUT_PULLUP);
}

void loop() {

  if (count % halfWavelength == 0) {
    sample = -1 * sample;
  }

  i2s.write(sample);
  i2s.write(sample);

  count++;

  // test button
  if (digitalRead(butPin)) {
    Serial.println("HI");
    Serial.println(count);
  }
}
```
After compiling and uploading the code, the speaker didn't produce any sound. However, upon "jiggling" the 5V wire, a flat tone can be heard. 

*1.5 hours spent*

upgraded to orpheus pico, took some time soldering and straightening the pins because we recieved horizontal mount header pins and needed to straighen them out (took like an hour)

rewired everything and moved from arduino IDE to PlatformIO in preparation of adding additional external files to the project.

spent like 30 minutes realizing why it wasn't working when migrating until I realized it wasn't uploading the code 💀.

Hooked TFT display and got the backlight up. 

```cpp
#include <I2S.h>
#include <TFT_eSPI.h> // Graphics and font library for ILI9341 driver chip
#include <SPI.h>

#define TFT_GREY 0x5AEB // New colour

// ---------- pin assignments ----------
const int DIN_PIN  = 12;   // SD
const int BCLK_PIN = 10;   // BCLK
const int LRCK_PIN = 11;   // WS (BCLK+1 by default)
const int BUT_PIN  = 15;   // active‑low push‑button

// ---------- tone parameters ----------
const int SAMPLE_RATE = 16000;      // Hz
const int TONE_HZ     = 600;        // A4
const int16_t AMP     = 15000;      // ±15 000 ≈ –3 dBFS
const int HALF_WAVE   = SAMPLE_RATE / TONE_HZ;   // samples per half‑cycle

// ---------- globals ----------
int16_t  sample = AMP;
uint32_t cnt    = 0;

I2S I2S(OUTPUT);
TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);

  I2S.setBCLK(BCLK_PIN);
  I2S.setDATA(DIN_PIN);

  if (!I2S.begin(44100)) {
    Serial.println("I2S init failed");
    while (true) delay(1);
  }

  // tft.init();
  // tft.setRotation(2);

  pinMode(BUT_PIN, INPUT_PULLUP);
}

void loop() {

  // tft.fillScreen(TFT_GREY);

  const bool pressed = !digitalRead(BUT_PIN);      // active‑low
  int32_t val = 0;                                 // default: silence

  if (pressed) {
    // advance square‑wave only while audible
    if (cnt % HALF_WAVE == 0) sample = -sample;
    val = sample;                                  // play the tone
    cnt++;
  }

  // feed I²S (cast removes the ambiguity)
  I2S.write((int32_t)val);   // left
  I2S.write((int32_t)val);   // right
}
```
