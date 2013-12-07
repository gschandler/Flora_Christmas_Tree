#include <EEPROM.h>

#include <Wire.h>
#include <Adafruit_LSM303.h>
#include <Adafruit_NeoPixel.h>

#define  PIN  6
#define  LED  7
#define  NUM_PIXELS  8

class ChristmasTree
{
  public:
    ChristmasTree( int numberOfLignts = 1, int pin = 6 );
    virtual ~ChristmasTree();
    
    void    Startup(int signal=1);
    virtual void  Update();
    
    void  FlashStar( uint32_t color, int count, int wait = 50 );
    void  FlashLights( uint32_t color, int count, int wait = 50 );
    void  FlashLight( int index, uint32_t color, int count, int wait = 50 );

    void  SetLightColor( int index, uint32_t color );
    void  ClearLight( int index );
    
    void  SetAllLightsColor( uint32_t color );
    void  ClearAllLights();
  
    void  RainbowCycle( int wait = 50 );
    void  ColorWipe( uint32_t c, int wait = 50);
    void  Rainbow( int  wait = 50 );

    int  GetStarIndex() const  { return _strip.numPixels()-1; }
    
  protected:
    Adafruit_NeoPixel &  GetStrip()   { return _strip; }

    uint32_t Wheel(byte WheelPos);
    
    float    GetMaxAccel() const   { return _maxAccel; }
    Adafruit_LSM303::lsm303AccelData  GetPreviousAccel() const { return _previousAccel; }
    Adafruit_LSM303::lsm303AccelData  GetCurrentAccel() const { return _lsm.accelData; }
    
  private:
    Adafruit_NeoPixel                    _strip;
    Adafruit_LSM303::lsm303AccelData    _previousAccel;
    Adafruit_LSM303                      _lsm;
    float                                _maxAccel;
};

class WhiteLightsChristmasTree : public ChristmasTree
{
  public:
    WhiteLightsChristmasTree( int numberOfLights = 1, int pin = 6 ) : ChristmasTree(numberOfLights,pin) {}
    void  Update();
};

class  RainbowLightsChristmasTree : public ChristmasTree
{
  public:
    RainbowLightsChristmasTree( int numberOfLights = 1, int pin = 6 ) : ChristmasTree(numberOfLights,pin) {}
    void  Update();
};

class  RedAndGreenLightsChristmasTree : public ChristmasTree
{
  public:
    RedAndGreenLightsChristmasTree( int numberOfLights = 1, int pin = 6 ) : ChristmasTree(numberOfLights,pin) {}
    void  Update();
};

//
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
//Adafruit_LSM303  lsm;

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
#define  ACCEL_CLAMP255(v,mv)  (ClampValue(abs(v), mv, 255.0))

// Dispatch table for function pointers to make changing our behavior easy


enum {
  kChristmasTreeWhiteLights = 0,
  kChristmasTreeRainbowLights,
  kChristmasTreeRedAndGreenLights,
  
  kChristmasTreeCount
};

static ChristmasTree *tree = NULL;

void ChristmasTreeFactory()
{
    // read from EEPROM to see what dispatch entry we are using
  uint8_t val = EEPROM.read(0);
  if ( val == 255 ) {  // initial state
    val = 0;
  }
  
  if ( ++val >= kChristmasTreeCount ) val = 0;
  EEPROM.write(0,val);
  
  ChristmasTree *ct = NULL;
  switch ( val ) {
    case kChristmasTreeWhiteLights : ct = new WhiteLightsChristmasTree(NUM_PIXELS,PIN); break;
    case kChristmasTreeRainbowLights : ct = new RainbowLightsChristmasTree(NUM_PIXELS,PIN); break;
    case kChristmasTreeRedAndGreenLights : ct = new RedAndGreenLightsChristmasTree(NUM_PIXELS,PIN); break;
  }

  if ( ct ) {
    tree = ct;
    tree->Startup(val+1);
  }
}


//
//  Standard Arduino setup
//

void setup() {
  pinMode(LED,OUTPUT);
  
  // set up serial for debugging
    Serial.begin(9600);
//    tree = ChristmasTreeFactory();
}

//
//  Standard Arduino loop
//
void loop() {
    float time = millis();
    digitalWrite(LED,HIGH);

    tree->Update();
    
    float interval = max(0.0,200 - (millis() - time));
    delay(interval);
    
    digitalWrite(LED,LOW);
    
    delay(200);
}

//
//
//

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
void  ChristmasTree::FlashLights( uint32_t color, int count, int interval )
{
    if ( color == 0 ) return;
    if ( count == 0 ) return;
    
    uint32_t  c = 0;
    for ( int loop = 0; loop<count*2; ++loop ) {
      SetAllLightsColor(c);
      delay(interval);
      c = (c==0) ? color : 0;
    }
    
    ClearAllLights();
}

//
//
//
void  ChristmasTree::FlashLight( int index, uint32_t color, int count, int interval ) {

  if ( index < 0 ) index = 0;
  else if (index >= _strip.numPixels()) index = _strip.numPixels() - 1;
  
  if ( count < 0 ) return;
  if ( color == 0 ) return;
  
  uint32_t c = 0;
  for ( int i=0; i<count*2; ++i ) {
    _strip.setPixelColor(index,c);
    _strip.show();
    c = (c==0) ? color : 0;
    delay(interval);
  }
  
  // turn it off at the end
  ClearLight(index);
}

void  ChristmasTree::FlashStar( uint32_t color, int count, int interval )
{
    FlashLight(_strip.numPixels()-1,color,count,interval);
}

//
//
//
void  ChristmasTree::SetLightColor( int index, uint32_t color )
{
    if (index<0) index = 0;
    if (index>=_strip.numPixels()) index = _strip.numPixels()-1;
    
    _strip.setPixelColor(index,color);
    _strip.show();
}

//
//
//
void  ChristmasTree::ClearLight( int index )
{
    SetLightColor(index,kBlackColor);
}

//
//
//
void  ChristmasTree::SetAllLightsColor( uint32_t color )
{
  for (int i=0;i<_strip.numPixels();++i) {
    _strip.setPixelColor(i,color);
  }
  _strip.show();
}

//
//
//
void  ChristmasTree::ClearAllLights()
{
  SetAllLightsColor(kBlackColor);
}


float  ClampValue( float value, float maxValue, float clampValue)
{
  return (value/maxValue) * clampValue;
}


// Fill the dots one after the other with a color
void ChristmasTree::ColorWipe(uint32_t c, int wait) {
  for(uint16_t i=0; i<_strip.numPixels(); i++) {
      _strip.setPixelColor(i, c);
      _strip.show();
      delay(wait);
  }
}

void ChristmasTree::Rainbow(int wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<_strip.numPixels(); i++) {
      _strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    _strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void ChristmasTree::RainbowCycle(int wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< _strip.numPixels(); i++) {
      _strip.setPixelColor(i, Wheel(((i * 256 / _strip.numPixels()) + j) & 255));
    }
    _strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t ChristmasTree::Wheel(byte WheelPos) {
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

ChristmasTree::ChristmasTree( int numberOfLights, int pin ) : _strip(numberOfLights,pin,NEO_GRB + NEO_KHZ800), _maxAccel(0)
{

  if ( !_lsm.begin() ) {
    Serial.println("Unable to initialize the LSM303!");
  }
  
  _lsm.read();
  _previousAccel = GetCurrentAccel();
  
  _strip.begin();
  _strip.show();
}

ChristmasTree::~ChristmasTree()
{
}

void ChristmasTree::Startup( int signal )
{
  
    FlashStar( kWhiteColor,5);
    
    ColorWipe(kRedColor,20);
    ColorWipe(kGreenColor,20);
    ColorWipe(kBlueColor,20);
    Rainbow(10);
    RainbowCycle(10);

    ClearAllLights();

    FlashStar( kWhiteColor,5 );
    delay(100);
    FlashStar( kYellowColor,signal);
    delay(100);
    FlashStar( kWhiteColor, 1 );
    
}

void  ChristmasTree::Update()
{
    _previousAccel = GetCurrentAccel();
    _lsm.read();
    
    Adafruit_LSM303::lsm303AccelData  accel = GetCurrentAccel();
    _maxAccel = max(_maxAccel,max(accel.x,max(accel.y,accel.z)));
}

void WhiteLightsChristmasTree::Update()
{
    ChristmasTree::Update();
    Adafruit_LSM303::lsm303AccelData accel = FilterAccelerometerData( GetPreviousAccel(), GetCurrentAccel(), 0.2 );
    float components[] = {ACCEL_CLAMP255(accel.x,GetMaxAccel()), ACCEL_CLAMP255(accel.y,GetMaxAccel()), ACCEL_CLAMP255(accel.z,GetMaxAccel())};
    for ( int i=0;i<GetStrip().numPixels();++i ) {
      float c = components[i%3];
      uint32_t white = Adafruit_NeoPixel::Color(byte(c),byte(c),byte(c));
      
      GetStrip().setPixelColor(i,white);
    }
    GetStrip().show();
}

void RainbowLightsChristmasTree::Update( )
{
    ChristmasTree::Update();
    
    Adafruit_LSM303::lsm303AccelData accel = FilterAccelerometerData( GetPreviousAccel(), GetCurrentAccel(), 0.2 );
    float r = ACCEL_CLAMP255(accel.x,GetMaxAccel()), g = ACCEL_CLAMP255(accel.y,GetMaxAccel()), b = ACCEL_CLAMP255(accel.z,GetMaxAccel());
    
    for ( int i=0;i<GetStrip().numPixels();++i ) {
      GetStrip().setPixelColor(i,Adafruit_NeoPixel::Color(byte(r),byte(g),byte(b)));
      float t = r; r = g; g = b; b = t;
    }
    GetStrip().show();

}

void RedAndGreenLightsChristmasTree::Update()
{
    ChristmasTree::Update();

    Adafruit_LSM303::lsm303AccelData accel = FilterAccelerometerData( GetPreviousAccel(), GetCurrentAccel(), 0.2 );
    float r = ACCEL_CLAMP255(accel.x,GetMaxAccel()), g = ACCEL_CLAMP255(accel.y,GetMaxAccel());
    
    for ( int i=0;i<GetStrip().numPixels()-1;++i ) {
      uint32_t color = (i%2) ? Adafruit_NeoPixel::Color(0,byte(g),0) : Adafruit_NeoPixel::Color(byte(r),0,0);
      GetStrip().setPixelColor(i,color);
    }

    GetStrip().setPixelColor(GetStrip().numPixels()-1,Adafruit_NeoPixel::Color(byte((r+g)/2.0),byte((r+g)/2.0),byte((r+g)/2.0)));
    GetStrip().show();
    
}
