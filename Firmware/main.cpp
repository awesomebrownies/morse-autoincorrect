#include <I2S.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

const int DIN_PIN = 12, BCLK_PIN = 10, LRCK_PIN = 11; 
const int BUT_PIN = 15;
const int SDA_PIN = 8,  SCL_PIN = 9; 

const int SAMPLE_RATE = 16000, TONE_HZ = 1000;
const int16_t AMP = 15000;
const int HALF_WAVE = SAMPLE_RATE / TONE_HZ;

I2S i2s(OUTPUT);

int16_t sample=AMP; uint32_t scnt=0;
inline void toneOut(bool on){
  int32_t v=0;
  if(on){ if(scnt%HALF_WAVE==0) sample=-sample; v=sample; scnt++; }
  i2s.write(v); i2s.write(v);
}

Adafruit_SSD1306 oled(128,32,&Wire,-1);       
const uint8_t CHAR_W  = 12;                  
const uint8_t TOP_CH  = 128/CHAR_W;           
String topLine = "";

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

File dictFile;
uint8_t levDist(const String& a,const String& b){
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

struct {const char* code; char ch;} const Morse[] PROGMEM = {
  {".-",'A'},{"-...",'B'},{"-.-.",'C'},{"-..",'D'},{".",'E'},{"..-.",'F'},
  {"--.",'G'},{"....",'H'},{"..",'I'},{".---",'J'},{"-.-",'K'},{".-..",'L'},
  {"--",'M'},{"-.",'N'},{"---",'O'},{".--.",'P'},{"--.-",'Q'},{".-.",'R'},
  {"...",'S'},{"-",'T'},{"..-",'U'},{"...-",'V'},{".--",'W'},{"-..-",'X'},
  {"-.--",'Y'},{"--..",'Z'}
};

String symBuf, wordBuf, serialBuf;

bool hold=false; uint32_t holdStart=0; const uint32_t HOLD_MS=3000;
bool lastState=false; uint32_t lastEdge=0, lastUp=0;

void setup(){
  Serial.begin(115200); while(!Serial && millis()<3000);
  LittleFS.begin(); dictFile = LittleFS.open("/words.txt","r");

  Wire.setSDA(SDA_PIN); Wire.setSCL(SCL_PIN); Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC,0x3C); oled.setTextColor(SSD1306_WHITE);

  i2s.setBCLK(BCLK_PIN); i2s.setDATA(DIN_PIN); i2s.begin(SAMPLE_RATE);
  pinMode(BUT_PIN, INPUT_PULLUP);

  sentence = ""; scrollPos=0; draw();

  oled.setCursor(0,0);  oled.println("AUTO");       
  oled.setCursor(0,16); oled.println("INCORRECT"); 
  oled.display();
}

void loop(){

  bool down=!digitalRead(BUT_PIN); toneOut(down); uint32_t now=millis();

  if(down && !hold){ hold=true; holdStart=now; }
  if(!down && hold){ hold=false; }

  // Reset
  if(hold && now-holdStart>=HOLD_MS){
    symBuf=wordBuf=sentence=serialBuf=""; scrollPos=0; topLine="RESET"; draw();
    while(digitalRead(BUT_PIN)==LOW); hold=false; lastEdge=lastUp=millis(); lastState=false; return;
  }
  // Map to dots/dashes
  if(down!=lastState){
    uint32_t dur=now-lastEdge; lastEdge=now;
    if(!down){ symBuf+=(dur>150)?'-':'.'; lastUp=now; topLine=symBuf; draw();}
    lastState=down;
  }
  // Find word match
  if(!down && symBuf.length() && (now-lastUp)>350){
    char letter='?';
    for(auto& p:Morse)
      if(symBuf==String((__FlashStringHelper*)p.code)){ letter=p.ch; break; }
    wordBuf+=letter; topLine=wordBuf; draw(); symBuf="";
  }
  // Autocorrect and add to sentence
  if(!down && wordBuf.length() && (now-lastUp)>2000){
    String repl=autocorrectWord(wordBuf);
    sentence+=repl+" "; wordBuf=""; topLine=""; scrollPos=0;
  }
  // Scrolling
  if(sentence.length() && millis()-lastStep>=STEP_MS){
    lastStep=millis();
    scrollPos = (scrollPos + 1) % sentence.length();
    draw();
  }
}
