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
After compiling and uploading the code, the speaker didn't produce any sound. However, upon "jiggling" the 5V wire, a flat tone can be heard. For some reason, pressing the button also changed the tone of the sound being played.

*1.5 hours spent*

upgraded to orpheus pico, took some time soldering and straightening the pins because we recieved horizontal mount header pins and needed to straighen them out (took like an hour)

rewired everything and moved from arduino IDE to PlatformIO in preparation of adding additional external files to the project.

<img width="450" height="700" alt="image" src="https://github.com/user-attachments/assets/17616fc7-21bf-4ae7-b838-f89dd452d873" />

spent like 30 minutes realizing why it wasn't working when migrating until I realized it wasn't uploading the code 💀.

Hooked TFT display and got the backlight up.

The TFT display doesn't seem to be working, even after switching from SPI0 to SPI1 (because of the amp). After a couple hours of debugging, I decided to get the autocorrect feature working in the serial monitor while we get the display situation sorted out. 

```cpp
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
  int32_t val = 0;                                

  if (pressed) {
    if (cnt % HALF_WAVE == 0) sample = -sample;
    val = sample;                                  // funni
    cnt++;
  }

  I2S.write((int32_t)val);   // left
  I2S.write((int32_t)val);   // right
}
```

*5 hours spent?*

### Morse Code:

we need a map of all the different letters the dots and dashes can encode to:

```cpp
struct {const char* code; char ch;} const Morse[] = {
  {".-",'A'},{"-...",'B'},{"-.-.",'C'},{"-..",'D'},{".",'E'},{"..-.",'F'},
  {"--.",'G'},{"....",'H'},{"..",'I'},{".---",'J'},{"-.-",'K'},{".-..",'L'},
  {"--",'M'},{"-.",'N'},{"---",'O'},{".--.",'P'},{"--.-",'Q'},{".-.",'R'},
  {"...",'S'},{"-",'T'},{"..-",'U'},{"...-",'V'},{".--",'W'},{"-..-",'X'},
  {"-.--",'Y'},{"--..",'Z'}
};
```

Then we need to actually detect button presses and classify them as dots or dashes (we will just continue polling the pin in loop() because I am lazy and do not want to use interrupts): 

```cpp
  if(down!=lastState){
    uint32_t dur = now-lastEdge; lastEdge=now;
    if(!down){                       // key released
      symBuf += (dur>150)?'-':'.';   
      // Serial.print("Pressed for (s) ");
      Serial.println(dur/1000.0, 3); 
      // Serial.print(" : “");
      lastUp = now;
    }
    lastState = down;
  }

  if(!down && symBuf.length() && (now-lastUp)>300){
    char letter='?';
    for(auto &p:Morse){
      if(symBuf == p.code){ letter=p.ch; break; }
    }
    wordBuf += letter;
    // Serial.print("Time Pressed (in seconds):");
    // Serial.print((now-lastUp)/1000.0, 3); // print time pressed in seconds
    Serial.println(wordBuf);          
    symBuf="";
  }
```

Now we have a buffer for all the inputs the user does! 

After about 2 hours we have the "autocorrect" feature. The user must have a data/words.txt file containing a list of capital words. Upload the FS image by going to PIO Home > Project Tasks > pico > Platform > Upload Filesystem Image

```cpp
#include <I2S.h>
#include <LittleFS.h>

const int DIN_PIN = 12, BCLK_PIN = 10, LRCK_PIN = 11;  
const int BUT_PIN = 15;                                

const int SAMPLE_RATE = 16000, TONE_HZ = 600;
const int16_t AMP = 15000;
const int HALF_WAVE = SAMPLE_RATE / TONE_HZ;
I2S  i2s(OUTPUT);
int16_t sample = AMP; uint32_t scnt = 0;

struct {const char* code; char ch;} const Morse[] = {
  {".-",'A'},{"-...",'B'},{"-.-.",'C'},{"-..",'D'},{".",'E'},{"..-.",'F'},
  {"--.",'G'},{"....",'H'},{"..",'I'},{".---",'J'},{"-.-",'K'},{".-..",'L'},
  {"--",'M'},{"-.",'N'},{"---",'O'},{".--.",'P'},{"--.-",'Q'},{".-.",'R'},
  {"...",'S'},{"-",'T'},{"..-",'U'},{"...-",'V'},{".--",'W'},{"-..-",'X'},
  {"-.--",'Y'},{"--..",'Z'}
};

String symBuf, wordBuf;
File   dictFile;

void toneOut(bool on){
  int32_t v=0;
  if(on){ if(scnt%HALF_WAVE==0) sample=-sample; v=sample; scnt++; }
  i2s.write(v); i2s.write(v);
}

uint8_t levDist(const String& a, const String& b){
  const uint8_t n=a.length(), m=b.length();
  uint8_t v0[m+1], v1[m+1];
  for(uint8_t j=0;j<=m;j++) v0[j]=j;
  for(uint8_t i=0;i<n;i++){
    v1[0]=i+1;
    for(uint8_t j=0;j<m;j++){
      uint8_t cost=(a[i]==b[j])?0:1;
      v1[j+1]=min(min(v1[j]+1,v0[j+1]+1),v0[j]+cost);
    }
    memcpy(v0,v1,m+1);
  }
  return v0[m];
}

String autocorrectWord(const String& w){
  if(!dictFile) return w;        // no file? give up

  dictFile.seek(0);
  uint8_t best=255, second=255;
  String bestWord, secondWord;
  while(dictFile.available()){
    String d = dictFile.readStringUntil('\n');
    d.trim(); d.toUpperCase();
    if(d.length()==0) continue;
    uint8_t dist = levDist(w,d);
    if(dist<best){
      second=best; secondWord=bestWord;
      best=dist;   bestWord=d;
    }else if(dist<second){
      second=dist; secondWord=d;
    }
  }
  if(bestWord == w && secondWord.length()) return secondWord;
  return bestWord.length() ? bestWord : w;
}

/* ---------- SETUP ---------- */
void setup(){
  Serial.begin(115200);
  while(!Serial) {};          

  // LittleFS
  if(!LittleFS.begin()){
    Serial.println("LittleFS init failed – no autocorrect dictionary!");
  }else{
    dictFile = LittleFS.open("/words.txt","r");
    if(!dictFile) Serial.println("words.txt not found! Autocorrect disabled.");
  }

  // audio
  i2s.setBCLK(BCLK_PIN); i2s.setDATA(DIN_PIN);
  i2s.begin(SAMPLE_RATE);

  pinMode(BUT_PIN, INPUT_PULLUP);
  Serial.println("Ready. Key your Morse:");
}

void loop(){
  bool down = !digitalRead(BUT_PIN); toneOut(down);

  static uint32_t lastEdge=0, lastUp=0; static bool lastState=false;
  uint32_t now = millis();

  if(down!=lastState){
    uint32_t dur = now-lastEdge; lastEdge=now;
    if(!down){                       // key released
      symBuf += (dur>150)?'-':'.';   
      // Serial.print("Pressed for (s) ");
      Serial.println(dur/1000.0, 3); 
      // Serial.print(" : “");
      lastUp = now;
    }
    lastState = down;
  }

  if(!down && symBuf.length() && (now-lastUp)>300){
    char letter='?';
    for(auto &p:Morse){
      if(symBuf == p.code){ letter=p.ch; break; }
    }
    wordBuf += letter;
    // Serial.print("Time Pressed (in seconds):");
    // Serial.print((now-lastUp)/1000.0, 3); 
    Serial.println(wordBuf);          
    symBuf="";
  }

  /* word gap (>.35 s of silence) */
  if(!down && wordBuf.length() && (now-lastUp)>350){
    Serial.println("AUTOCORRECTING…");
    String replace = autocorrectWord(wordBuf);
    Serial.println(replace);          
    Serial.println();  
    wordBuf="";
  }
}
```


## Day 3

Team Member: Evan

I've created a simple 3d model case that can be printed in two parts. It consists of the speaker cutout, button cutout, and display cutout. I've also created a very large hole for the usb c cable so it should be able to fit no matter what.


<img width="480" height="514" alt="Screenshot from 2025-07-13 12-02-48" src="https://github.com/user-attachments/assets/220af131-0165-41c2-8ba1-1f6961c46a68" />

<img width="416" height="688" alt="Screenshot from 2025-07-13 13-07-10" src="https://github.com/user-attachments/assets/156ed290-8d44-4091-a2f3-3c6fdbc28b80" />
