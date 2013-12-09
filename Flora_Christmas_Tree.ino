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
    
    virtual void  Startup(int signal=1);
    virtual void  Update();
    
    const static int kDefaultWait;
    
    void  FlashStar( uint32_t color, int count, int wait = kDefaultWait );
    void  FlashLights( uint32_t color, int count, int wait = kDefaultWait );
    void  FlashLight( int index, uint32_t color, int count, int wait = kDefaultWait );

    void  SetStarColor( uint32_t color );
    void  SetLightColor( int index, uint32_t color );
    void  ClearLight( int index );
    
    void  SetAllLightsColor( uint32_t color );
    void  ClearAllLights();
  
    void  RainbowCycle( int wait = kDefaultWait );
    void  ColorWipe( uint32_t c, int wait = kDefaultWait);
    void  Rainbow( int  wait = kDefaultWait );

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
    int                                  _update;
};

class WhiteLightsChristmasTree : public ChristmasTree
{
  public:
    WhiteLightsChristmasTree( int numberOfLights = 1, int pin = 6 ) : ChristmasTree(numberOfLights,pin) {}
    void  Update();
    void  Startup( int );
};

class  RainbowLightsChristmasTree : public ChristmasTree
{
  public:
    RainbowLightsChristmasTree( int numberOfLights = 1, int pin = 6 ) : ChristmasTree(numberOfLights,pin) {}
    void  Update();
    void  Startup( int );
   private:
       uint8_t                            *_originalPixels;
};

class  RedAndGreenLightsChristmasTree : public ChristmasTree
{
  public:
    RedAndGreenLightsChristmasTree( int numberOfLights = 1, int pin = 6 ) : ChristmasTree(numberOfLights,pin) {}
    void  Update();
    void  Startup( int );
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

void CreateChristmasTree()
{
    // read from EEPROM to see what dispatch entry we are using
  uint8_t val = EEPROM.read(0);
  if ( val == 255 ) {  // initial state
    val = 0;
  }
    
  ChristmasTree *ct = NULL;
  switch ( kChristmasTreeRainbowLights ) {
    case kChristmasTreeWhiteLights : ct = new WhiteLightsChristmasTree(NUM_PIXELS,PIN); break;
    case kChristmasTreeRainbowLights : ct = new RainbowLightsChristmasTree(NUM_PIXELS,PIN); break;
    case kChristmasTreeRedAndGreenLights : ct = new RedAndGreenLightsChristmasTree(NUM_PIXELS,PIN); break;
  }

  val = (val+1) % kChristmasTreeCount;
  EEPROM.write(0,val);

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
    CreateChristmasTree();
}

//
//  Standard Arduino loop
//
void loop() {
    float time = millis();
    digitalWrite(LED,HIGH);

    if ( tree ) {
        tree->Update();
    }
    
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
void  ChristmasTree::SetStarColor( uint32_t color )
{
  SetLightColor( _strip.numPixels()-1, color);
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
    _strip.show();
  }
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


const int ChristmasTree::kDefaultWait = 100;

ChristmasTree::ChristmasTree( int numberOfLights, int pin ) : _strip(numberOfLights,pin,NEO_GRB + NEO_KHZ800), _maxAccel(0)
{

  if ( !_lsm.begin() ) {
    Serial.println("Unable to initialize the LSM303!");
  }
  
  
}

ChristmasTree::~ChristmasTree()
{
}

void ChristmasTree::Startup( int signal )
{
   _lsm.read();
   _previousAccel = GetCurrentAccel();
   randomSeed( long( _previousAccel.x + _previousAccel.y + _previousAccel.z) );
   
   _strip.begin();
   _strip.show();
 
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
    
    float x = abs(accel.x), y = abs(accel.y), z = abs(accel.z);
    
    Serial.print("Accel X:"); Serial.println(int(x));
    Serial.print("      Y:"); Serial.println(int(y));
    Serial.print("      Z:"); Serial.println(int(z));
  
}

void  WhiteLightsChristmasTree::Startup( int signal )
{
    ChristmasTree::Startup(signal);
  Serial.println("White");
}

void WhiteLightsChristmasTree::Update()
{
    ChristmasTree::Update();
    Adafruit_LSM303::lsm303AccelData accel = FilterAccelerometerData( GetPreviousAccel(), GetCurrentAccel(), 0.2 );
  Serial.println("White");

     int x = map(int(abs(accel.x)),0,int(GetMaxAccel()),1,8) << 5, 
    y = map(int(abs(accel.y)),0,int(GetMaxAccel()),1,8) << 5, 
    z = map(int(abs(accel.z)),0,int(GetMaxAccel()),1,8) << 5;

    Serial.print("x,y,z="); Serial.print(x); Serial.print(","); Serial.print(y); Serial.print(","); Serial.println(z);

    int components[] = {x,y,z};
    for ( int i=0;i<GetStrip().numPixels();++i ) {
      int c = components[i%3];
      uint32_t white = Adafruit_NeoPixel::Color(byte(c),byte(c),byte(c));
      
      GetStrip().setPixelColor(i,white);
    }
    GetStrip().show();
}

// -----------------------------------------------------------------------

void  RainbowLightsChristmasTree::Startup( int signal )
{
  ChristmasTree::Startup(signal);
 
   for ( int i=0;i<GetStrip().numPixels();++i ) {
     uint8_t r = random(127,256), g = random(127,256), b = random(127,256);
     GetStrip().setPixelColor(i,r,g,b);
   } 
   
  uint8_t *pixels = GetStrip().getPixels();
  _originalPixels = new uint8_t[3 * GetStrip().numPixels()];
  if ( _originalPixels ) {
    memcpy( _originalPixels, pixels, 3 * GetStrip().numPixels());
  }
  
  Serial.println("Rainbow");
}

void RainbowLightsChristmasTree::Update( )
{
    ChristmasTree::Update();
   Serial.println("Rainbow");
  
    Adafruit_LSM303::lsm303AccelData accel = FilterAccelerometerData( GetPreviousAccel(), GetCurrentAccel(), 0.2 );

    const int kLowMap = 0, kHighMap = 64;
    const int kMultiplier = 256 / (kHighMap - kLowMap);
    int adjust[] = { map(int(abs(accel.x)),0,int(GetMaxAccel()),kLowMap,kHighMap), map(int(abs(accel.y)),0,int(GetMaxAccel()),kLowMap,kHighMap), map(int(abs(accel.z)),0,int(GetMaxAccel()),kLowMap,kHighMap)};

    Serial.print("x,y,z="); Serial.print(adjust[0]); Serial.print(","); Serial.print(adjust[1]); Serial.print(","); Serial.println(adjust[2]);

    uint8_t *pixels = GetStrip().getPixels();
    for ( int i=0;i<GetStrip().numPixels();++i ) {
      int component = random(0,3);
      pixels[i+component] = constrain(_originalPixels[i+component] + (adjust[component] * kMultiplier) - 127,0,255);
      int t = adjust[0]; adjust[0] = adjust[1]; adjust[1] = adjust[2]; adjust[2] = t;
      GetStrip().show();
    }

}

// -----------------------------------------------------------------------

void  RedAndGreenLightsChristmasTree::Startup( int signal )
{
    ChristmasTree::Startup(signal);
    
   Serial.println("Red and Green");
   
    SetStarColor( kWhiteColor );
}

void RedAndGreenLightsChristmasTree::Update()
{
    ChristmasTree::Update();
   Serial.println("Red and Green");

    Adafruit_LSM303::lsm303AccelData accel = FilterAccelerometerData( GetPreviousAccel(), GetCurrentAccel(), 0.2 );
   
    int x = map(int(abs(accel.x)),0,int(GetMaxAccel()),1,8) << 5, 
    y = map(int(abs(accel.y)),0,int(GetMaxAccel()),1,8)  << 5, 
    z = map(int(abs(accel.z)),0,int(GetMaxAccel()),1,8) << 5;
    
    Serial.print("x,y,z="); Serial.print(x); Serial.print(","); Serial.print(y); Serial.print(","); Serial.println(z);
    
    for ( int i=0;i<GetStrip().numPixels()-1;++i ) {
      if ( i % 2 ) {
        GetStrip().setPixelColor(i,x,0,0);
      }
      else {
        GetStrip().setPixelColor(i,0,y,0);
      }
    }

    GetStrip().show();
    
}
