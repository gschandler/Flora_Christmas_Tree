
#include <EEPROM.h>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>    // required for Adafruit_LSM303
#include <Adafruit_LSM303.h>

#define  DEBUG !0

#ifndef DEBUG
#define DEBUG 0
#endif

#define  NEO_PIXEL_STRIP_PIN  6
#define  NUM_PIXELS  8
#define  LED_BUILTIN  7

// convenience definitions
uint32_t  kRedColor = Adafruit_NeoPixel::Color(255,0,0);
uint32_t  kGreenColor = Adafruit_NeoPixel::Color(0,255,0);
uint32_t  kBlueColor = Adafruit_NeoPixel::Color(0,0,255);
uint32_t  kWhiteColor = Adafruit_NeoPixel::Color(255,255,255);
uint32_t  kBlackColor = Adafruit_NeoPixel::Color(0,0,0);
uint32_t  kMagentaColor = Adafruit_NeoPixel::Color(255,0,255);
uint32_t  kYellowColor = Adafruit_NeoPixel::Color(255,255,0);
uint32_t  kCyanColor = Adafruit_NeoPixel::Color(0,255,255);

/*
 *
 * Globals
 *
 */
Adafruit_NeoPixel  pixels(NUM_PIXELS,NEO_PIXEL_STRIP_PIN);



/*
 *
 * Accelerometer
 *
 */
class Accelerometer
{
public:
  Accelerometer();
  
  boolean   begin();
  void     update();
  
  Adafruit_LSM303::lsm303AccelData previous() const  { return _previousAccel; }
  Adafruit_LSM303::lsm303AccelData current(float lowPassAlpha = 1.0) const;
  float  maximum() const { return _maximumAccel; }
  
  Adafruit_LSM303::lsm303AccelData  currentLowPass(float alpha = 0.25) const;
  
private:
  float                              _maximumAccel;
  Adafruit_LSM303                    _lsm303;
  Adafruit_LSM303::lsm303AccelData  _previousAccel;
};  

Accelerometer  accelerometer;



// some macros that help out
#define  ACCEL256(v,mv)    map(long(abs(v)),0,long(abs(mv)),0,256)

// Dispatch table for function pointers to make changing our behavior easy
typedef uint32_t (*LightProgram)(int index);
LightProgram  programs[] = {
   &whiteLight,
   &rainbowLight,
   &redGreenLight
};
const int kNumberOfLightPrograms = sizeof(programs)/sizeof(LightProgram);

/*
 *
 *
 *
 */
static int lightProgram = 0;
void  selectProgram()
{
  // read from EEPROM to see what lightProgram we are running
  uint8_t val = EEPROM.read(0);
  if ( val == 255 ) {  // initial state
    val = 0;
  }
  lightProgram = val;
  val = (val+1) % kNumberOfLightPrograms;
  EEPROM.write(0,val);
}

/*
 *
 *
 *
 */
int  currentProgram()
{
  return lightProgram;
}

/*
 *
 *  Initialized the NeoPixels
 *
 */
void  initNeoPixels()
{
  pixels.begin();
  pixels.show();
}

/*
 *
 * Initialize the LSM303
 *
 */
void initLSM303()
{
  if ( !accelerometer.begin() ) {
#if DEBUG
    Serial.println("Unable to initialize the LSM303!");
#endif
  }
}

/*
 *
 *  Standard Arduino setup
 *
 */
void setup() {
#if DEBUG
  // set up serial for debugging
    Serial.begin(9600);
#endif

#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN,OUTPUT);
#endif
  
  initLSM303();
  selectProgram();
  initNeoPixels();
  lightshow();
}

//
//
//
#define  MIN_LIGHT_VALUE  20
#define  MAX_LIGHT_VALUE  150
uint8_t  randomColorValue()
{
  uint8_t   low = MIN_LIGHT_VALUE;
  uint8_t  high = MAX_LIGHT_VALUE;
  
  float components[3] = { accelerometer.current().x, accelerometer.current().y, accelerometer.current().z };
  
  uint8_t a = map(components[random(0,3)],0,accelerometer.maximum(),0,256);
  uint8_t b = 256 - a;
  
  low = min(a,b);
  high = max(a,b);

  return byte(random(low,high));
}

uint32_t  whiteLight( int index )
{
  uint8_t c = randomColorValue(); 
  uint32_t color = Adafruit_NeoPixel::Color(c,c,c);
  return color;
}

uint32_t  rainbowLight( int index )
{
  uint32_t color = kWhiteColor;
  if ( index < pixels.numPixels()-1 ) {
    color = Adafruit_NeoPixel::Color(randomColorValue(),randomColorValue(),randomColorValue());
  }
  return color;
}

uint32_t  redGreenLight( int index )
{
  uint32_t  color = kWhiteColor;
  if ( index < pixels.numPixels()-1 ) {
    if ( random(2) ) {
      color = Adafruit_NeoPixel::Color(0,randomColorValue(),0);
    }
    else {
      color = Adafruit_NeoPixel::Color(randomColorValue(),0,0);
    }
  }
  else {
    color = whiteLight(index);
  }
  return color;
}


/*
 *
 *  Standard Arduino loop
 *
 */
void loop() {
#ifdef LED_BUILTIN
    float time = millis();
    digitalWrite(LED_BUILTIN,HIGH);
#endif

    accelerometer.update();
    
#if DEBUG
  Serial.println("Values <raw->cooked>:");
  Adafruit_LSM303::lsm303AccelData rawData = accelerometer.current(1.0);
  Adafruit_LSM303::lsm303AccelData cookedData = accelerometer.current();
  Serial.print("x= "); Serial.print("<"); Serial.print(rawData.x); Serial.print(" -> "); Serial.print(cookedData.x); Serial.println(">");
  Serial.print("y= "); Serial.print("<"); Serial.print(rawData.y); Serial.print(" -> "); Serial.print(cookedData.y); Serial.println(">");  
  Serial.print("z= "); Serial.print("<"); Serial.print(rawData.z); Serial.print(" -> "); Serial.print(cookedData.z); Serial.println(">");
#endif
  
    for ( int i=0;i<pixels.numPixels();++i ) {
      uint32_t color = programs[currentProgram()](i);
      pixels.setPixelColor(i,color);
      pixels.show();
    }
    
#ifdef LED_BUILTIN
    float interval = max(0.0,200 - (millis() - time));
    delay(interval);
    digitalWrite(LED_BUILTIN,LOW);
#endif    

    delay(200);
}

//
//
//
void  lightshow()
{
    colorWipe(kRedColor,20);
    colorWipe(kGreenColor,20);
    colorWipe(kBlueColor,20);
    rainbow(10);
    rainbowCycle(10);

    clearAllLights();
    
    int lastPixel = pixels.numPixels()-1;
    flashLight( lastPixel, kWhiteColor, 5, 10 );
    delay(100);
    flashLight( lastPixel, kYellowColor,currentProgram()+1,10);
    delay(100);
    flashLight( lastPixel, kWhiteColor, 1,10 );
}

void  flashLights( uint32_t color, int count, int interval )
{
    if ( color == 0 ) return;
    if ( count == 0 ) return;
    
    uint32_t  c = 0;
    for ( int loop = 0; loop<count*2; ++loop ) {
      setAllLightsColor(c);
      delay(interval);
      c = (c==0) ? color : 0;
    }
    
    clearAllLights();
}

//
//
//
void  flashLight( int index, uint32_t color, int count, int interval ) {

  if ( index < 0 ) index = 0;
  else if (index >= pixels.numPixels()) index = pixels.numPixels() - 1;
  
  if ( count < 0 ) return;
  if ( color == 0 ) return;
  
  uint32_t c = 0;
  for ( int i=0; i<count*2; ++i ) {
    pixels.setPixelColor(index,c);
    pixels.show();
    c = (c==0) ? color : 0;
    delay(interval);
  }
  
  // turn it off at the end
  clearLight(index);
}

//
//
//
void  setTopLightColor( uint32_t color )
{
  setLightColor( pixels.numPixels()-1, color);
}

//
//
//
void  setLightColor( int index, uint32_t color )
{
    if (index<0) index = 0;
    if (index>=pixels.numPixels()) index = pixels.numPixels()-1;
    
    pixels.setPixelColor(index,color);
    pixels.show();
}

//
//
//
void  clearLight( int index )
{
    setLightColor(index,0);
}

//
//
//
void  setAllLightsColor( uint32_t color )
{
  for (int i=0;i<pixels.numPixels();++i) {
    pixels.setPixelColor(i,color);
    pixels.show();
  }
}

//
//
//
void  clearAllLights()
{
  setAllLightsColor(0);
}


// Fill the dots one after the other with a color
void colorWipe(uint32_t c, int wait) {
  for(uint16_t i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, c);
      pixels.show();
      delay(wait);
  }
}

void rainbow(int wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, wheel((i+j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(int wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< pixels.numPixels(); i++) {
      pixels.setPixelColor(i, wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t wheel(byte WheelPos) {
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

/*
 *
 * Accelerometer
 *
 */
Accelerometer::Accelerometer() : _maximumAccel(0)
{
  _previousAccel.x = _previousAccel.y = _previousAccel.z = 0.0;
}

Adafruit_LSM303::lsm303AccelData  Accelerometer::current(float lowPassAlpha) const
{
  float alpha = constrain(lowPassAlpha,0.0,1.0);
  
  Adafruit_LSM303::lsm303AccelData data = _lsm303.accelData;
  data.x = alpha * data.x + (1.0 - alpha) * _previousAccel.x;
  data.y = alpha * data.y + (1.0 - alpha) * _previousAccel.y;
  data.z = alpha * data.z + (1.0 - alpha) * _previousAccel.z;
  
  return data;
}

boolean Accelerometer::begin()
{
  boolean result = _lsm303.begin(); 
  if ( result ) {
    _lsm303.read(); 
    _previousAccel = _lsm303.accelData;
    _maximumAccel = max(abs(_lsm303.accelData.x),max(abs(_lsm303.accelData.y),abs(_lsm303.accelData.z)));
  }
  return result;
}

void Accelerometer::update()
{
  _previousAccel = current(0.25);
  _lsm303.read(); 
  _maximumAccel = max(_maximumAccel,max(abs(_lsm303.accelData.x),max(abs(_lsm303.accelData.y),abs(_lsm303.accelData.z))));
}

