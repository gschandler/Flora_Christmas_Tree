#include <EEPROM.h>

#include <Wire.h>
#include <Adafruit_LSM303.h>
#include <Adafruit_NeoPixel.h>

#define  PIN  6
#define  NUM_PIXELS  8

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_LSM303  lsm;

uint32_t  kRedColor = Adafruit_NeoPixel::Color(255,0,0);
uint32_t  kGreenColor = Adafruit_NeoPixel::Color(0,255,0);
uint32_t  kBlueColor = Adafruit_NeoPixel::Color(0,0,255);
uint32_t  kWhiteColor = Adafruit_NeoPixel::Color(255,255,255);
uint32_t  kBlackColor = Adafruit_NeoPixel::Color(0,0,0);
uint32_t  kMagentaColor = Adafruit_NeoPixel::Color(255,0,255);
uint32_t  kYellowColor = Adafruit_NeoPixel::Color(255,255,0);
uint32_t  kCyanColor = Adafruit_NeoPixel::Color(0,255,255);

#define  LAST_PIXEL  (strip.numPixels()-1)
#define  ACCEL_CLAMP255(v)  (ClampValue(abs(v), max_accel, 255.0))

void colorWipe(uint32_t, uint8_t);
void rainbowCycle(uint8_t);
void rainbow(uint8_t);
uint32_t Wheel(byte);

typedef void (*DispatchFunc)(void);

DispatchFunc  dispatchTable[] = {
  &WhiteLights,
  &RainbowLights,
  &RedAndGreenLights
};

typedef enum {
  kLoopTest = 0,
  kLoopRun
} LoopState;

LoopState loopState = kLoopTest;

int  dispatchIndex = 0;
const int  kMaxDispatchIndex = (sizeof(dispatchTable)/sizeof(DispatchFunc)) - 1;
void  SetupDispatchIndex()
{
  // read from EEPROM to see what dispatch entry we are using
  uint8_t val = EEPROM.read(0);
  if ( val == 255 ) {  // initial state
    val = 0;
  }

  dispatchIndex = val;
  
  if ( ++val > kMaxDispatchIndex ) val = 0;
  EEPROM.write(0,val);

}

void setup() {
  // set up serial for debugging
    Serial.begin(9600);

  // begin NeoPixel stuff
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  // start up LM303
  if ( !lsm.begin() ) {
    Serial.println("Unable to initialize the LSM303!");
  }

  SetupDispatchIndex();
  
  loopState = kLoopTest;
}

float max_accel = 0;

void loop() {
  switch ( loopState ) {
    case kLoopTest: {
       // signal begining of start up (flash 5 times), test the pixels, than signal end of test (flash 10 times)
      FlashNeoPixel(kWhiteColor,LAST_PIXEL,5,100);
      TestNeoPixels();
      FlashDispatchIndexInLastPixel();
      loopState = kLoopRun;
   }
    break;
    
    case kLoopRun : {
      // load accelerometer data and continue to refine the maximum acceleration global
      lsm.read();
      max_accel = max(max_accel,max(lsm.accelData.x,max(lsm.accelData.y,lsm.accelData.z)));
      
      DispatchFunc func = dispatchTable[dispatchIndex];
      if ( func ) {
        func();
      }  
    }
    break;
  }
  
}

//
//
//
void  FlashDispatchIndexInLastPixel()
{
  FlashNeoPixel(kWhiteColor,LAST_PIXEL,5,100);
  delay(200);
  FlashNeoPixel(kYellowColor,LAST_PIXEL,dispatchIndex+1,200);
  delay(200);
  FlashNeoPixel(kWhiteColor,LAST_PIXEL,5,100);
  
  Serial.print("Running program index: "); Serial.println(dispatchIndex);

}

//
//
//
void  TestNeoPixels()
{  
    colorWipe(kRedColor,20);
    colorWipe(kGreenColor,20);
    colorWipe(kBlueColor,20);
    rainbow(10);
    rainbowCycle(10);

    SetAllPixelColor(kBlackColor);
}

//
//
//
void  SetAllPixelColor( uint32_t c )
{
    for ( int i=0; i<strip.numPixels(); ++i ) {
      strip.setPixelColor(i,c);
    }
    strip.show();
}

//
//
//
void  FlashNeoPixels( uint32_t color, int count )
{
    if ( color == 0 ) return;
    if ( count == 0 ) return;
    
    uint32_t  c = 0;
    for ( int loop = 0; loop<count*2; ++loop ) {
      SetAllPixelColor(c);
      delay(100);
      c = (c==0) ? color : 0;
    }
    
    SetAllPixelColor(0);
}

//
//
//
void  FlashNeoPixel( uint32_t color, int index, int count, int interval ) {
  if ( index < 0 ) index = 0;
  else if (index >= strip.numPixels()) index = strip.numPixels() - 1;
  
  if ( count < 0 ) return;
  
  if ( color == 0 ) return;
  
  uint32_t c = 0;
  for ( int i=0; i<count*2; ++i ) {
    strip.setPixelColor(index,c);
    strip.show();
    c = (c==0) ? color : 0;
    delay(interval);
  }
  // turn it off at the end
  strip.setPixelColor(index,kBlackColor);
  strip.show();
}

float  ClampValue( float value, float maxValue, float clampValue)
{
  return (value/maxValue) * clampValue;
}


//
//
//
void  WhiteLights()
{
    Adafruit_LSM303::lsm303AccelData accel = lsm.accelData;
    float components[] = {ACCEL_CLAMP255(accel.x), ACCEL_CLAMP255(accel.y), ACCEL_CLAMP255(accel.z)};
    for ( int i=0;i<strip.numPixels();++i ) {
      float c = components[i%3];
      uint32_t white = Adafruit_NeoPixel::Color(byte(c),byte(c),byte(c));
      
      strip.setPixelColor(i,white);
    }
    strip.show();
}

//
//
//
void  RainbowLights()
{
    Adafruit_LSM303::lsm303AccelData accel = lsm.accelData;
    float r = ClampValue(abs(accel.x),max_accel,255.0), g = ClampValue(abs(accel.y),max_accel,255.0), b = ClampValue(abs(accel.z),max_accel,255.0);
    
    for ( int i=0;i<strip.numPixels();++i ) {
      strip.setPixelColor(i,Adafruit_NeoPixel::Color(byte(r),byte(g),byte(b)));
      float t = r; r = g; g = b; b = t;
    }
    strip.show();
}

//
//
//
void  RedAndGreenLights()
{
    Adafruit_LSM303::lsm303AccelData accel = lsm.accelData;
    float r = ClampValue(abs(accel.x),max_accel,255.0), g = ClampValue(abs(accel.y),max_accel,255.0);
    
    for ( int i=0;i<strip.numPixels()-1;++i ) {
      uint32_t color = (i%2) ? Adafruit_NeoPixel::Color(0,byte(g),0) : Adafruit_NeoPixel::Color(byte(r),0,0);
      strip.setPixelColor(i,color);
    }
    strip.setPixelColor(LAST_PIXEL,Adafruit_NeoPixel::Color(byte((r+g)/2.0),byte((r+g)/2.0),byte((r+g)/2.0)));
    strip.show();
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return Adafruit_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return Adafruit_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return Adafruit_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

