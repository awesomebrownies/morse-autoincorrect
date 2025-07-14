## Day 1
Our main objective was to create a circuit that utilized the speakers. Our first hassle was the infinitely long lines of the undercity. Many scavengers scrounged the streets towards the hardware sanctuary.

*2 hours theorizing the perfect idea*

*Unfortunately* the large sign out front said **SINGLE FILE LINE**

This delayed the inevitable adventure, as our frantic petrifying speed made the components we need fly off back on the shelves. 

*1.5 hours spent in line*

## Day 2
To create the mechanism, we constructed off a audio amplifier out of towers of solder, each pin at a time. Once finished the swarming spaghetti monster of breadboards consumed our work. The spaghetti was weak, and easily shifted. More importantly, the communication channels within, our **firmware**, had a fatal flaw. Evan worked on the first mechanism. *the button*. This was no ordinary button. As it refused to work despite every effort, neither towards staff member or any helping hand.

*2-3 hours exterminating the bugs within the code*

The MCU being used for the project is the SEEED XIAO RP2040. We want to interface it with the MAX98357A amp.
 
Reference the documentation on the [MCU](https://wiki.seeedstudio.com/XIAO-RP2040/) and the [amp](https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts).

<img width="500" height="400" alt="image" src="https://github.com/user-attachments/assets/8392701d-06e8-4235-8c27-bd56bf0f2ac4" />

Assign GPIO pins 2, 3, 4 to BCLK, LRC, and DIN respectively. The RP2040 driver enforces that LRC is connected to GPIO#+1 of the BCLK pin. For example, if BCLK -> GPIO 0, then LRC -> GPIO 1. Don't ask me why it enforces that. 

GAIN can be left floating. SD can be left floating to output stereo average. 

Schematic:

<img width="714" height="450" alt="image" src="https://github.com/user-attachments/assets/1a2fd819-3c31-4e72-bbe0-4842414bad66" />

Below is the firmware uploaded (modified from an example sketch from the Arduino IDE). This is only to test if the sound and buttons are working. The button uses an internal pullup resistor on GPIO5. 

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

After compiling and uploading the code, the speaker didn't produce any sound. However, upon "jiggling" the 5V wire, a flat tone can be heard (YAY). For whatever reason, pressing the button unintentially changes the tone of the sound. 

*0.5 hours spent*

We want to add a TFT display such that the user can see what is being translated. However, since the current MCU doesn't have enough GPIO pins, we upgraded to a Orpheus Pico. We took some time soldering and straightening the pins because we recieved horizontal mount header pins and needed to straighen them out for the breadboard (took like an hour) (it was painful). 

After that, we rewired everything and moved from Arduino IDE to PlatformIO in preparation of adding external files to the project (and because VSCode is cooler).

We hooked up the TFT display and got the backlight on, referencing this [guide](https://www.instructables.com/Rasberry-Pi-Zero-W-With-Arduino-TfT-ili9341/).

<img width="450" height="700" alt="image" src="https://github.com/user-attachments/assets/17616fc7-21bf-4ae7-b838-f89dd452d873" />

We spent like 30 minutes confused on why my firmware from PlatformIO wasn't uploading to the pico. The reason was because we didn't specify the upload protocol (picotool).

The TFT display doesn't seem to be working, even after switching from SPI0 to SPI1 (because of the amp). After a couple hours of head-scratching, we decided to get the autocorrect feature working in the serial monitor first before moving on to the TFT issue. 

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
 
Above is just code to ensure we correctly migrated button/sound code to the Orpheus Pico. 

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

Then we need to actually detect button presses and classify them as dots or dashes (we will just continue polling the pin in loop() because I (Vinson) am lazy and do not want to use interrupts): 

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
    // Serial.print((now-lastUp)/1000.0, 3); 
    Serial.println(wordBuf);          
    symBuf="";
  }
```

We spent like 30 minutes tuning/adjusting the timing of the button presses to match up to standard of Morse Code.  

Now we have a buffer for all the inputs the user does! 

After about 2 hours we made the "autocorrect" feature. The user must have a data/words.txt file containing a list of capital words. Upload the FS image by going to PIO Home > Project Tasks > pico > Platform > Upload Filesystem Image. 

### Levenshtein edit-distance

The Levenshtein edit-distance scores how "different" a word is from another based on inserts/deletes/substitutes. The code scans every word in words.txt and keeps track of the best and second lowest scores. 

```cpp
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
```

Two byte arrays (v0 & v1) are used to run the routine quickly. 

Below is the code so far: 
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

While using the translator, we found that the timing was insanely fast, so we adjusted the wait times for some of the events. 

The TFT display was replaced with an OLED screen that uses I2C, going from 8 pins -> 4 pins. According to the pico pinout, there are many options for the I2C bus, so we chose GPIOs 8 and 9. Vinson must have still been half asleep while wiring, because he spent a good two hours figuring out why it didn't work (the SDA and SCL pins were swapped).

<img width="500" height="450" alt="image" src="https://github.com/user-attachments/assets/0ca63e9e-c596-4a39-b93c-df049d8da1d6" />

Then we just connect VDD to 3v3 and VSS to GND!

For the OLED, we'll be using a SSD1306 library kindly provided by Adafruit. 

```cpp
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
                          
const int SDA_PIN = 8,  SCL_PIN = 9;                  

Adafruit_SSD1306 oled(128, 32, &Wire, -1);            

void oledPrint(const String& live, const String& msg){
  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.setTextSize(1);           
  oled.print(live);              
  oled.setCursor(0,32);
  oled.setTextSize(1);
  oled.println(msg);             // wraps
  oled.display();
}

void setup(){
  // OLED
  Wire.setSDA(SDA_PIN); Wire.setSCL(SCL_PIN); Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextColor(SSD1306_WHITE);
  oledPrint("READY","");
}
```

After an hour or so, we made a reset function (hold 3s). One issue we ran into is that the program treated that press as a long press and assigned it to a "T". 

```cpp
// RESET STUF
bool holdActive=false; uint32_t holdStart=0;
const uint32_t HOLD_MS=3000;

bool     lastState=false;
uint32_t lastEdge=0, lastUp=0;

  // IN LOOP
  if(holdActive && now-holdStart>=HOLD_MS){
    symBuf=wordBuf=message=serialBuf="";
    oledPrint("RESET"); Serial.println("*** SCREEN RESET ***");
    while(digitalRead(BUT_PIN)==LOW);
    lastState=false;
    lastEdge = lastUp = millis();               
    holdActive=false; return;                   
  }
```

We also made a way to input text and autoincorrect in the serial monitor, just so we don't have to repeatedly spam the button to test features. This was later removed.

Vinson vibe-coded the scrolling. Basically it keeps shifting a window of 10 characters left so the user can see everything they inputted. 

```cpp
String sentence   = "";  
size_t scrollPos  = 0;
uint32_t lastStep = 0;
const uint32_t STEP_MS = 200;               

inline void draw(){
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0,0); 
  oled.println(topLine.substring(0,TOP_CH));
  if(sentence.length()==0){ oled.println("          "); }
  else{
    String win = sentence.substring(scrollPos) + sentence.substring(0,scrollPos);
    oled.println(win.substring(0,TOP_CH));
  }
  oled.display();
}

  // Scrolling (IN LOOP)
  if(sentence.length() && millis()-lastStep>=STEP_MS){
    lastStep=millis();
    scrollPos = (scrollPos + 1) % sentence.length();
    draw();
  }
```

For some reason, vibe coding changed a bunch of the code (ex. word randomness, text size, etc.) We corrected them in about half an hour.

<img width="500" height="650" alt="image" src="https://github.com/user-attachments/assets/72494e21-e2c8-4afa-a59e-a4699a2819b8" />

Schematic:

<img width="540" height="333" alt="image" src="https://github.com/user-attachments/assets/854bf532-825f-4be4-928f-e4deb13e4f89" />

We created a simple 3D model case that can be printed in two parts. It consists of the speaker cutout, button cutout, and display cutout. We also created a very large hole for the USB-C cable so it should be able to fit no matter what.

<img width="480" height="514" alt="Screenshot from 2025-07-13 12-02-48" src="https://github.com/user-attachments/assets/220af131-0165-41c2-8ba1-1f6961c46a68" />

<img width="416" height="688" alt="Screenshot from 2025-07-13 13-07-10" src="https://github.com/user-attachments/assets/156ed290-8d44-4091-a2f3-3c6fdbc28b80" />
