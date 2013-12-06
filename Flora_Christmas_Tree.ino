#include <EEPROM.h>

#include <Wire.h>
#include <Adafruit_LSM303.h>
#include <Adafruit_NeoPixel.h>

#define  PIN  6
#define  LED  7
#define  NUM_PIXELS  8

//
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_LSM303  lsm;

// convenience definitions
uint32_t  kRedColor = Adafruit_NeoPixel::Color(255,0,0);
uint32_t  kGreenColor = Adafruit_NeoPixel::Color(0,255,0);
uint32_t  kBlueColor = Adafruit_NeoPixel::Color(0,0,255);
uint32_t  kWhiteColor = Adafruit_NeoPixel::Color(255,255,255);
uint32_t  kBlackColor = Adafruit_NeoPixel::Color(0,0,0);
uint32_t  kMagentaColor = Adafruit_NeoPixel::Color(255,0,255);
uint32_t  kYellowColor = Adafruit_NeoPixel::Color(255,255,0);
uint32_t  kCyanColor = Adafruit_NeoPixel::Color(0,255,255);

// some macros that help out
#define  LAST_PIXEL  (strip.numPixels()-1)
#define  ACCEL_CLAMP255(v)  (ClampValue(abs(v), max_accel, 255.0))

// Dispatch table for function pointers to make changing our behavior easy
typedef void (*DispatchFunc)(void);

DispatchFunc  dispatchTable[] = {
  &WhiteLights,
  &RainbowLights,
  &RedAndGreenLights
};

int  currentDispatchFuncIndex = 0;
const int kDispatchFuncCount = sizeof(dispatchTable)/sizeof(DispatchFunc);
const int  kMaxDispatchIndex = kDispatchFuncCount - 1;

//
//
//
void  InitializeDispatchTable()
{
  // read from EEPROM to see what dispatch entry we are using
  uint8_t val = EEPROM.read(0);
  if ( val == 255 ) {  // initial state
    val = 0;
  }
  
  if ( ++val > kMaxDispatchIndex ) val = 0;
  
  currentDispatchFuncIndex = val;
  
  EEPROM.write(0,val);

}

//
//  Standard Arduino setup
//
void setup() {
  pinMode(LED,OUTPUT);
  
  // set up serial for debugging
    Serial.begin(9600);
  
  // start up LM303
  InitializeAccelerometer();
  
   // begin NeoPixel stuff
  InitializeNeoPixels();
 
  InitializeDispatchTable();

  
   // signal begining of start up (flash 5 times), test the pixels, than signal end of test (flash 10 times)
  FlashNeoPixel(kWhiteColor,LAST_PIXEL,5,100);
  TestNeoPixels();
  FlashDispatchIndexInLastPixel();
}

float max_accel = 0;

//
//  Standard Arduino loop
//
void loop() {
    float time = millis();
    digitalWrite(LED,HIGH);

    ReadAccelerometer();
    
    dispatchTable[currentDispatchFuncIndex]();
    
    float interval = max(0.0,200 - (millis() - time));
    delay(interval);
    
    digitalWrite(LED,LOW);
    
    delay(200);
}

//
//
//
Adafruit_LSM303::lsm303AccelData  lastAccelData;

//
//
//
Adafruit_LSM303::lsm303AccelData  FilterAccelerometerData( const Adafruit_LSM303::lsm303AccelData &past, const Adafruit_LSM303::lsm303AccelData &present, float alpha )
{
  Adafruit_LSM303::lsm303AccelData data;
  data.x = alpha * past.x + (1.0 - alpha) * present.x;
  data.y = alpha * past.y + (1.0 - alpha) * present.y;
  data.z = alpha * past.z + (1.0 - alpha) * present.z;
  
  return data;
}

//
//
//
void  InitializeAccelerometer()
{
  if ( !lsm.begin() ) {
    Serial.println("Unable to initialize the LSM303!");
  }
  lsm.read();
  lastAccelData = lsm.accelData;
}

//
//
//
void  ReadAccelerometer()
{
    lastAccelData = lsm.accelData;
    
    // load accelerometer data and continue to refine the maximum acceleration global
    lsm.read();
    max_accel = max(max_accel,max(lsm.accelData.x,max(lsm.accelData.y,lsm.accelData.z)));
}


//
//
//
void  InitializeNeoPixels()
{
  // begin NeoPixel stuff
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}


//
//
//
void  FlashDispatchIndexInLastPixel()
{
  FlashNeoPixel(kWhiteColor,LAST_PIXEL,5,100);
  delay(200);
  
  // flash our "star" the index of the current dispatch function
  FlashNeoPixel(kYellowColor,LAST_PIXEL,currentDispatchFuncIndex,200);

  delay(200);
  FlashNeoPixel(kWhiteColor,LAST_PIXEL,5,100);
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
  
    Adafruit_LSM303::lsm303AccelData accel = FilterAccelerometerData( lastAccelData, lsm.accelData, 0.2 );
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
    Adafruit_LSM303::lsm303AccelData accel = FilterAccelerometerData( lastAccelData, lsm.accelData, 0.2 );
    float r = ACCEL_CLAMP255(accel.x), g = ACCEL_CLAMP255(accel.y), b = ACCEL_CLAMP255(accel.z);
    
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
    Adafruit_LSM303::lsm303AccelData accel = FilterAccelerometerData( lastAccelData, lsm.accelData, 0.2 );
    float r = ACCEL_CLAMP255(accel.x), g = ACCEL_CLAMP255(accel.y);
    
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

