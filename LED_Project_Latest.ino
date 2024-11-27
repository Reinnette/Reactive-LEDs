// I2S driver
#include <driver/i2s.h>
#include "FastLED.h"

enum Modes {Static, AltClr, SwpClr, Reactive, Breathing, Stripe, CenterOut, OutCenter};
 
 //Configure these two values!!!
// INMP441 I2S pin assignment
#define I2S_WS               15
#define I2S_SD               32
#define I2S_SCK              14
#define I2S_SAMPLE_BIT_COUNT 32
#define SOUND_SAMPLE_RATE    8000
#define SOUND_CHANNEL_COUNT  1
#define I2S_PORT             I2S_NUM_0
//LED Setup
//!!REMINDER MUST GO INTO SETUP AND CHANGE LED
#define NUM_LEDS 144 //Max Num Leds in a single strip
const int numLedPins = 5; //Used to determine the total number of Led Strips
const int numStripes = 5;
Modes ledMode = Static; //Used to determine the DEFAULT Led Mode. See enum Modes for the list of modes
CRGB defaultColor1 = CRGB(0,247,255);
CRGB defaultColor2 = CRGB(252,11,3);
CRGB colorList[5] = {pink, blue, white, blue, pink};

//Default Set Colors
CRGB pink = CRGB(100,255,100);
CRGB blue = CRGB(50,50,255);
CRGB white = CRGB(255,255,255);

const int I2S_DMA_BUF_COUNT = 8;
const int I2S_DMA_BUF_LEN = 1024;

const int StreamBufferNumBytes = 512;
const int StreamBufferLen = StreamBufferNumBytes / 4;
int32_t StreamBuffer[StreamBufferLen];

// sound sample (16 bits) amplification
const int MaxAmplifyFactor = 20;
const int DefAmplifyFactor = 10;

esp_err_t i2s_install();
esp_err_t i2s_setpin();

CRGB ledArray[numLedPins][NUM_LEDS];
std::function<void(int, int)> special; //The method to call dependent on the set mode
bool isOdd = true; //Used in alternating colors down the strip
bool isDimming = true; //Used for Reactive and Breathing. Reactive will always set this to TRUE.
int amplifyFactor = DefAmplifyFactor;//10;

int test=0;
int base=0; //60-250hz
int lowMids=0; //250-500hz
int mids=0;//500-2000hz
int highMids=0;//2-4khz
int presence=0;//4-6khz
int brilliance=0; //6-16khz

void setup() {
  Serial.begin(115200);

  Serial.println("SETUP MIC ...");

  // set up I2S
  if (i2s_install() != ESP_OK) {
    Serial.println("XXX failed to install I2S");
    while (1) {;}
  }
  if (i2s_setpin() != ESP_OK) {
    Serial.println("XXX failed to set I2S pins");
    while (1) {;}
  }
  if (i2s_zero_dma_buffer(I2S_PORT) != ESP_OK) {
    Serial.println("XXX failed to zero I2S DMA buffer");
    while (1) {;}
  }
  if (i2s_start(I2S_PORT) != ESP_OK) {
    Serial.println("XXX failed to start I2S");
    while (1) {;}
  }

  Serial.println("... DONE SETUP MIC");

  //LED Setup
  //One Statement per led strip is needed
  //EXAMPLE
  //FastLED.addLeds<WS2812, LED_PIN, RGB>(ledArray[STRIP_NUMBER], NUM_LEDS);  // GRB ordering is typical
  // Only LED_PIN and STRIP_NUMBER need to be changed
  //LED_PIN - The pin for that led strip
  //STRIP_NUMBER - Starting at 0 the strip number, this will always increment by one
    FastLED.addLeds<WS2812, 2, RGB>(ledArray[0], NUM_LEDS);  // GRB ordering is typical
    FastLED.addLeds<WS2812, 4, RGB>(ledArray[1], NUM_LEDS);  // GRB ordering is typical
    FastLED.addLeds<WS2812, 5, RGB>(ledArray[2], NUM_LEDS);  // GRB ordering is typical
    FastLED.addLeds<WS2812, 12, RGB>(ledArray[3], NUM_LEDS);  // GRB ordering is typical
    FastLED.addLeds<WS2812, 13, RGB>(ledArray[4], NUM_LEDS);  // GRB ordering is typical
    ChangeBrightness(100);
}

void loop() {
  SetMode();
  if(ledMode == Reactive){//If Mode is Reactive then we need to get the frequency values first
    GetSoundValues();
  }
  
  //NOTE: We need to loop through each LED first to ensure that each Led on each strip is updated at the same time
  for(int i = 0; i<NUM_LEDS; i++) { //Loop Through each Led
      for(int count = 0; count < numLedPins; count++) {//Loop through each Led Strip
        (special)(i, count);
      }
      if(ledMode != Reactive && ledMode != Breathing)
        delay(100);
  } 
  
  isOdd = !isOdd;

  //ONLY IF Led Mode is Reactive do we want to update the leds that are not being used to dim them out
  //Since the Loop function is operating faster than wanted only dim an led once every millisecond
    EVERY_N_MILLISECONDS(1){
      if(ledMode != Reactive && ledMode != Breathing)
        return;

      if(ledMode == Reactive)
        isDimming = true;

      for(int currentLed = 0; currentLed<NUM_LEDS; currentLed++) {
        for(int ledStrip = 0; ledStrip<numLedPins; ledStrip++) {

          int intensity = 2;
          CRGB targetVal;

          //Get the Current Led Value
          int rVal = ledArray[ledStrip][currentLed].r;
          int gVal = ledArray[ledStrip][currentLed].g;
          int bVal = ledArray[ledStrip][currentLed].b;

          if(isDimming){
            targetVal = CRGB(0,0,0);

            if(rVal < 255 / 2) intensity = 2;
            else intensity = 3;
            rVal = rVal-intensity > 0 ? rVal-intensity : 0;
            
            
            if(gVal < 255 / 2) intensity = 2;
            else intensity = 3;
            gVal = gVal-intensity > 0 ? gVal-intensity : 0;

            
            if(bVal < 255 / 2) intensity = 2;
            else intensity = 3;
            bVal = bVal-intensity > 0 ? bVal-intensity : 0;
          }
          else{
            targetVal = isOdd ? defaultColor1 : defaultColor2;

            if(targetVal.r < 255 / 2) intensity = 2;
            else intensity = 3;
            rVal = rVal+intensity < targetVal.r ? rVal+intensity : targetVal.r;
            
            
            if(targetVal.g < 255 / 2) intensity = 2;
            else intensity = 3;
            gVal = gVal+intensity < targetVal.g ? gVal+intensity : targetVal.g;

            
            if(targetVal.b < 255 / 2) intensity = 2;
            else intensity = 3;
            bVal = bVal+intensity < targetVal.b ? bVal+intensity : targetVal.b;
          }

          ledArray[ledStrip][currentLed]=CRGB(rVal,gVal,bVal);

          if(ledArray[ledStrip][currentLed] == targetVal)
            isDimming = !isDimming;
        }
      }
  }

  FastLED.show();
}

esp_err_t i2s_install() {
  uint32_t mode = I2S_MODE_MASTER | I2S_MODE_RX;
#if I2S_SCK == I2S_PIN_NO_CHANGE
    mode |= I2S_MODE_PDM;
#endif    
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(mode/*I2S_MODE_MASTER | I2S_MODE_RX*/),
    .sample_rate = SOUND_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BIT_COUNT),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = I2S_DMA_BUF_COUNT/*8*/,
    .dma_buf_len = I2S_DMA_BUF_LEN/*1024*/,
    .use_apll = false
  };
  return i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}
 
esp_err_t i2s_setpin() {
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,   
    .data_out_num = -1/*-1*/,
    .data_in_num = I2S_SD
  };
  return i2s_set_pin(I2S_PORT, &pin_config);
}

void StaticColor(int i, int count){
  ledArray[count][i] = defaultColor1;
  FastLED.show();
}

void AlternatingColors(int i, int count){
  if(isOdd) {
    ledArray[count][i] = i%2 ? defaultColor1 : defaultColor2;
  } else {
    ledArray[count][i] = i%2 ? defaultColor2 : defaultColor1;
  }
  FastLED.show();
}

void SwappingColors(int currentLed, int ledStrip){
  if(isOdd) {
    ledArray[ledStrip][currentLed] = defaultColor1;
  } else {
    ledArray[ledStrip][currentLed] = defaultColor2;
  }
  FastLED.show();
}

void GetSoundValues(){
  // read I2S data and place in data buffer
  size_t bytesRead = 0;
  esp_err_t result = i2s_read(I2S_PORT, &StreamBuffer, StreamBufferNumBytes, &bytesRead, portMAX_DELAY);

  //Get the mean of each frequency band
  for(int i = 0; i<bytesRead/4; i++) {
    //if(i>StreamBufferLen) {break;}

    int frequency = StreamBuffer[i] / (I2S_SAMPLE_BIT_COUNT * SOUND_SAMPLE_RATE * SOUND_CHANNEL_COUNT);
    if(frequency > 60 && frequency < 250)
    {
        base = (base + (frequency)) / 2 + 1;
    }
    else if(frequency > 250 && frequency < 500)//lows
    {
        lowMids = (lowMids + (frequency)) / 2 + 1;
    }
    else if(frequency > 500 && frequency < 2000)//mids
    {
        mids = (mids + (frequency)) / 2 + 1;
    }
    else if(frequency > 2000 && frequency < 4000)//highs
    {
        highMids = (highMids + (frequency-2000)) / 2;
    }
    else if(frequency > 4000 && frequency < 6000)//highs
    {
        presence = (presence + (frequency-4000)) / 2;
    }
    else if(frequency > 6000 && frequency < 16000)//highs
    {
        brilliance = (brilliance + (frequency-6000)) / 2;
    }
    else
    {
        test = (test + frequency) / 2;
    }

    StreamBuffer[i] = 0;
  }
}

void SoundReactive(int currentLed, int ledStrip){
  //FOR EACH LED Set its color value
  int rValColor = 255;
  int gValColor = 255;
  int bValColor = 255;
  
  rValColor = ((base * 1000) / 250) * 255 / 1000;
  float rValLeds = ((base * 1000) / 250) * NUM_LEDS / 1000;

  gValColor = ((lowMids * 1000) / 500) * 255 / 1000;
  float gValLeds = ((lowMids * 1000) / 500) * NUM_LEDS / 1000;

  bValColor = ((mids * 1000) / 2000) * 255 / 1000;
  float bValLeds = ((mids * 1000) / 2000) * NUM_LEDS / 1000;

  
  rValColor = currentLed <= rValLeds ? rValColor : ledArray[ledStrip][currentLed].g;
  gValColor = currentLed <= gValLeds ? gValColor : ledArray[ledStrip][currentLed].r;
  bValColor = currentLed <= bValLeds ? bValColor : ledArray[ledStrip][currentLed].b;
  ledArray[ledStrip][currentLed]=CRGB(gValColor,rValColor,bValColor);//GRB
  ledArray[ledStrip][currentLed].maximizeBrightness((rValColor + gValColor + bValColor) / 3);
}

void BreathingEffect(int currentLed, int ledStrip){
  return;
}

void Strips(int currentLed, int ledStrip){
  int div = NUM_LEDS / colorList.Size(); 
  ledArray[ledStrip][currentLed]=colorList[floor(currentLed / div)];
}

void CenterOutEffect(int currentLed, int ledStrip){
  if(currentLed > NUM_LEDS / 2)
    return;

  bool isCenter = ceil(NUM_LEDS % 2);
  //If there is a center Led
  if(isCenter){
      ledArray[ledStrip][NUM_LEDS / 2 + currentLed]=colorList[floor(currentLed / div)];
      ledArray[ledStrip][NUM_LEDS / 2 - currentLed]=colorList[floor(currentLed / div)];
  }
  else{
    ledArray[ledStrip][ceil(NUM_LEDS / 2) + currentLed]=colorList[floor(currentLed / div)];
    ledArray[ledStrip][floor(NUM_LEDS / 2) + currentLed]=colorList[floor(currentLed / div)];
  }
}

void OutCenterEffect(int currentLed, int ledStrip){
  if(currentLed > NUM_LEDS / 2)
    return;

  ledArray[ledStrip][currentLed]=defaultColor1;
  ledArray[ledStrip][NUM_LEDS - currentLed]=defaultColor2;
}

void SetMode(){
  switch(ledMode){
    case Static:
      special = StaticColor;
      break;
    case AltClr:
      special = AlternatingColors;
      break;
    case SwpClr:
      special = SwappingColors;
      break;
    case Reactive:
      special = SoundReactive;
    case Breathing:
      special = BreathingEffect;
    case Stripe:
      special = Stripes;
    case CenterOut:
      special = CenterOutEffect;
    case OutCenter:
      special = OutCenterEffect;
  }
}

void ChangeBrightness(int value){
  FastLED.setBrightness(value);
}