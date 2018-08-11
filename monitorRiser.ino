#include <Adafruit_NeoPixel.h>
#include <math.h>

#define DIAL_N_PIN 2
#define DIAL_P_PIN 3
#define NEXT_PIN   5
#define BUTTON_PIN 6
#define PIXEL_PIN  7

#define PIXEL_COUNT 30

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

enum VIS_T { COLORS, COLORS_RAND, RAIN, FADE, FADE_RAND, RAINBOW };
enum COLORS_T { R, G, B, RG, RB, GB, LIGHT_G, LIGHT_B, LIGHT_RB, RGB, BLACK };
enum BUTTON_T { OFF, ON, ON_DIAL, STRIP_ON_OFF };

enum VIS_T vis = RAIN;
enum COLORS_T color = R;
enum BUTTON_T button_prev = OFF, button_curr;
uint32_t button_on_time = 0;

uint32_t sparkles[PIXEL_COUNT];
uint32_t colorMap[20];
int16_t brightness = 50;
unsigned long displayTime = 0, randTime, randTimeOut = 1000;
uint32_t idx = 0;
uint8_t fadeSpeed = 3;
uint8_t fadeVal = 250;
bool fadeDir, rainbowDir;
bool inRandShowIncDec = false;
uint8_t numSparkles = 4;
bool strip_on = true;

void setup() {
  Serial.begin(9600);
  while (!Serial) { }

  pinMode(DIAL_N_PIN, INPUT);
  pinMode(DIAL_P_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(NEXT_PIN,   OUTPUT);
  pinMode(PIXEL_PIN,  OUTPUT);
  
  strip.setBrightness(brightness);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  colorMap[(int)R] =        0xFA0000;
  colorMap[(int)G] =        0x00FA00;
  colorMap[(int)B] =        0x0000FA;
  colorMap[(int)RG] =       0xFAFA00;
  colorMap[(int)RB] =       0xFA00FA;
  colorMap[(int)GB] =       0x00FAFA;
  colorMap[(int)LIGHT_G] =  0x66FA44;
  colorMap[(int)LIGHT_B] =  0x7777FA;
  colorMap[(int)LIGHT_RB] = 0xBB66BB;
  colorMap[(int)RGB] =      0xFAFAFA;
  colorMap[(int)BLACK] =    0x000000;
}

void loop()
{
  unsigned long diffDisplayTime;
  unsigned long currTime = millis();
  int8_t dialVal = 0; // -1 for left, 1 for right, 0 for none
  bool changeVis = false;

  //Serial.println(lastLHigh + (lastRHigh << 1));

  // update dialIncDec
  dialVal = readDial();

  // possibly draw the display
  diffDisplayTime = currTime - displayTime;
  if (diffDisplayTime > 30 || diffDisplayTime < 0)
  {
    if (strip_on) {
      drawVis();
    }
    displayTime = currTime;
    idx++;
  }
  
  // update button values, possibly including dial values for brightness
  if (digitalRead(BUTTON_PIN)) {
    if (button_prev != OFF) {
      if (strip_on && dialCheckBrightness(dialVal)) {
        dialVal = 0;
        button_curr = ON_DIAL;
      } else if (button_prev != ON_DIAL) {
        if (currTime - button_on_time > 5000) {
          strip_on = turnStripOnOff(strip_on);
          while (digitalRead(BUTTON_PIN)) { }
          delay(10); // debounce time
          button_curr = STRIP_ON_OFF;
        }
      }
    }
    if (button_prev != ON_DIAL && button_curr != ON_DIAL && button_curr != STRIP_ON_OFF) {
      button_curr = ON;
    }
    if (button_prev == OFF) {
      button_on_time = currTime;
      delay(10); // debounce time
    }
  } else {
    switch (button_prev) {
    case ON:
      changeVis = strip_on && true;
    case STRIP_ON_OFF:
    case ON_DIAL:
      delay(10); // debounce time
      break;
    }
    button_curr = OFF;
  }
  button_prev = button_curr;

  // change the visualization
  if (strip_on && changeVis) {
    if (vis == RAINBOW)
      vis = 0;
    else
      vis = vis + 1;
    switch (vis) {
    case COLORS:
      Serial.println("COLORS");
      break;
    case COLORS_RAND:
      Serial.println("COLORS_RAND");
      break;
    case FADE:
      Serial.println("FADE");
      break;
    case FADE_RAND:
      Serial.println("FADE_RAND");
      break;
    case RAINBOW:
      Serial.println("RAINBOW");
      break;
    }
    specificVisInit();
  }

  // update dial values
  if (strip_on) {
    dialVal = dialCheckSpecificVisIncDec(dialVal);
  }
}

/**
 * Alters the on/off state of the strip, including a cute little visualization to
 * tell you which state you have just entered.
 * 
 * @param strip_on The current state of the strip (true for on, false for off).
 * @return !strip_on
 */
bool turnStripOnOff(bool strip_on) {
  if (strip_on) {
    colorWipe(0, 10);
  } else {
    colorWipe(0x00555555, 10);
  }
  return !strip_on;
}

/**
 * Updates the brightness.
 * If dialval is negative then brightness is decreased.
 * If dialval is positive then brightness is increased.
 * 
 * @return true if the brightness was changed
 */
int dialCheckBrightness(int dialVal)
{
  if (dialVal != 0) {
    int incVal = 5;
    if ((brightness >= 40 && dialVal > 0) || (brightness > 40 && dialVal < 0))
      incVal = 10;
    if ((brightness >= 80 && dialVal > 0) || (brightness > 80 && dialVal < 0))
      incVal = 20;
    if (dialVal < 0) brightness -= incVal;
    if (dialVal > 0) brightness += incVal;
    brightness = min(max(brightness, 5), 250);
    strip.setBrightness(brightness);
    return true;
  }

  return false;
}

int dialCheckSpecificVisIncDec(int dialVal)
{
  if (dialVal != 0)
  {
    if (dialVal < 0) specificVisDecrement();
    if (dialVal > 0) specificVisIncrement();
    dialVal = 0;
  }
  
  return dialVal;
}

void specificVisInit()
{
  int i, delayTime = 100;
  
  switch (vis)
  {
  case FADE:
    fadeVal = 250;
    break;
  case FADE_RAND:
  case COLORS_RAND:
    for (i = 0; i < 4; i++) {
      chooseRandColor();
      colorWipe(colorMap[(int)color], 2);
      strip.show();
      delay(delayTime);
      delayTime *= 1.5;
    }
    randTime = millis();
    break;
  case RAIN:
  case RAINBOW:
  case COLORS:
    // nothing to do
    break;
  }
}

void specificVisDecrement()
{
  switch (vis)
  {
  case COLORS:
  case FADE:
    if ((int)color == 0)
      color = BLACK - 1;
    else
      color = color - 1;
    break;
  case RAIN:
    numSparkles = max(numSparkles - 1, 0);
    break;
  case COLORS_RAND:
  case FADE_RAND:
    randTimeOut -= (randTimeOut <= 200) ? 0 : (log(randTimeOut) / log(2) * 110 - 740);
    randTimeOut = max(randTimeOut, 200);
    randShowIncDec();
    break;
  case RAINBOW:
    rainbowDir = !rainbowDir;
    break;
  }
}

void specificVisIncrement()
{
  switch (vis)
  {
  case COLORS:
  case FADE:
    if (color == BLACK - 1)
      color = 0;
    else
      color = color + 1;
    break;
  case RAIN:
    numSparkles = min(numSparkles + 1, PIXEL_COUNT);
    break;
  case COLORS_RAND:
  case FADE_RAND:
    randTimeOut += (randTimeOut >= 60000) ? 0 : (log(randTimeOut) / log(2) * 110 - 740);
    randTimeOut = min(randTimeOut, 60000);
    randShowIncDec();
    break;
  case RAINBOW:
    rainbowDir = !rainbowDir;
    break;
  }
}

void drawVis()
{
  long diffRandTime = millis() - randTime;
  
  switch (vis)
  {
  case COLORS_RAND:
    if (diffRandTime > randTimeOut || diffRandTime < 0) {
      chooseRandColor();
      randTime = millis();
    }
  case COLORS:
    colorWipe(colorMap[(int)color], 0);
    break;
  case RAIN:
    rain();
    break;
  case FADE_RAND:
    if (fadeVal <= fadeSpeed && fadeDir && (diffRandTime > randTimeOut || diffRandTime < 0)) {
      chooseRandColor();
      randTime = millis();
    }
  case FADE:
    fade();
    break;
  case RAINBOW:
    rainbowCycle();
    break;
  }
  strip.show();
}

void chooseRandColor()
{
  enum COLORS_T currColor = color;
  
  while ((int)currColor == (int)color)
  {
    color = random(BLACK-1);
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    if (wait > 0)
      strip.show();
    delay(wait);
  }
}

void fade()
{
  uint32_t rVal = 0, gVal = 0, bVal = 0, colorVal = 0, colorValFade = 0;
  
  // update fade
  if (fadeDir) // increasing
  {
    if (250 - fadeVal < fadeSpeed)
    {
      fadeVal = 250;
      fadeDir = false;
    }
    else
    {
      fadeVal += fadeSpeed;
    }
  }
  else // decreasing
  {
    if (fadeVal < fadeSpeed)
    {
      fadeVal = 0;
      fadeDir = true;
    }
    else
    {
      fadeVal -= fadeSpeed;
    }
  }

  // draw fade
  colorVal = colorMap[(int)color] & 0x00FFFFFF;
  rVal = (colorVal & 0xFF0000) >> 16;
  gVal = (colorVal & 0x00FF00) >> 8;
  bVal = colorVal & 0x0000FF;
  rVal = min(max((double)rVal / 250.0 * (double)fadeVal, 0), 250);
  gVal = min(max((double)gVal / 250.0 * (double)fadeVal, 0), 250);
  bVal = min(max((double)bVal / 250.0 * (double)fadeVal, 0), 250);
  rVal = (rVal << 16) & 0x00FF0000;
  gVal = (gVal <<  8) & 0x0000FF00;
  bVal = bVal & 0x000000FF;
  colorValFade = rVal | gVal | bVal;
  colorWipe(colorValFade, 0);
}

void rain()
{
  sparkle(0x00000032, 0x00969696, 100, numSparkles, 2);
}

/** Creates a faded sparkling animation.
 * @param baseColor The color for non-sparkles.
 * @param sparkleColorLow The brightest possible sparkle color.
 * @param singleChannelRange The smallest span in the R, G, or B channel between baseColor and sparkleColorLow.
 * @param numSparkles The number of pixels to have sparkle at a time.
 * @param subVal The value to subtract from current sparkle pixels.
 */
void sparkle(uint32_t baseColor, uint32_t sparkleColorLow, uint32_t singleChannelRange, uint8_t numSparkles, uint8_t subVal)
{
  uint32_t rangeRnd;
  uint8_t i, pixel, cnt = 0;
  uint8_t *pixPtr, *basePtr;

  // how many sparkles do we have?
  for (pixel = 0; pixel < PIXEL_COUNT; pixel++)
  {
    cnt += (sparkles[pixel] > 0) ? 1 : 0;
  }

  // create new sparkles!
  while (cnt < numSparkles)
  {
    pixel = random(PIXEL_COUNT);
    if (sparkles[pixel] == 0)
    {
      rangeRnd = random(singleChannelRange);
      sparkles[pixel] = ( (sparkleColorLow & 0x00FF0000) + (rangeRnd << 16) |
                          (sparkleColorLow & 0x0000FF00) + (rangeRnd <<  8) |
                          (sparkleColorLow & 0x000000FF) +         rangeRnd );
      cnt++;
    }
  }

  // draw the sparkles
  for (pixel = 0; pixel < PIXEL_COUNT; pixel++)
  {
    if (sparkles[pixel] > 0)
    {
      pixPtr = (uint8_t*)(sparkles + pixel);
      basePtr = (uint8_t*)&baseColor;
      for (i = 0; i < 4; i++)
      {
        if (pixPtr[i] > 0)
        {
          if (pixPtr[i] > subVal)
            pixPtr[i] -= subVal;
          else
            pixPtr[i] = 0;
        }
        if (pixPtr[i] < basePtr[i])
          pixPtr[i] = basePtr[i];
      }
      strip.setPixelColor(pixel, *((uint32_t*)pixPtr) );
      if (sparkles[pixel] == baseColor)
      {
        sparkles[pixel] = 0;
      }
    }
    else
    {
      strip.setPixelColor(pixel, baseColor);
    }
  }
}

void rainbowCycle() {
  int i;

  for (i = 0; i < PIXEL_COUNT; i++)
  {
    strip.setPixelColor(i, Wheel(idx + i * 4));
  }

  strip.show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(uint8_t WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3,0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3,0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0,0);
}

void randShowIncDec()
{
  int indicatorLED;
  unsigned long delayTime;
  int dialVal;
  bool timedOut = false;
  volatile int i = 0, j = 1;
  int oldRandShowIncDecIndicatorLED = -1;

  // special check to avoid lots of recursion
  if (inRandShowIncDec)
    return;
  inRandShowIncDec = true;

  // continue until the user stops updating the value
  while (!timedOut)
  {
    // get new variable values
    delayTime = millis();
    indicatorLED = min(max((double)randTimeOut / 60000.0 * PIXEL_COUNT, 0), PIXEL_COUNT-1);
    Serial.println(randTimeOut);

    // update lighted LED on strip?
    if (oldRandShowIncDecIndicatorLED != indicatorLED)
    {
      colorWipe(0, 0);
      strip.setPixelColor(indicatorLED, 0xFAFAFA);
      strip.setBrightness(250);
      strip.show();
      strip.setBrightness(brightness);
      oldRandShowIncDecIndicatorLED = indicatorLED;
    }

    // delay loop and check for new inc/dec
    timedOut = true;
    while (millis() - delayTime < 500)
    {
      dialVal = readDial();
      if (dialVal != 0)
      {
        dialCheckSpecificVisIncDec(dialVal);
        timedOut = false;
        break;
      }
  
      // Waste time in a pointless loop because this pointless loop does not disable interrupts
      for (i = 0; i < 1000; i++)
        j *= 2;
    }
  }
  
  randTime = millis();
  oldRandShowIncDecIndicatorLED = -1;
  inRandShowIncDec = false;
}

/**
 * Reads the value off of the rotaryEncoderBuffer project pins:
 * DIAL_N_PIN, DIAL_P_PIN
 * 
 * If a signal is available then the "ready" signal on NEXT_PIN is brought high
 * until the signal is dropped, and then the "ready" signal is dropped low for at least 1ms.
 * 
 * @return -1 for a left rotation, 1 for a right rotation, 0 for no rotation.
 */
int8_t readDial()
{
  int8_t dialVal = 0;
  
  // check for signal available
  if (digitalRead(DIAL_N_PIN) == LOW || digitalRead(DIAL_P_PIN) == LOW)
    return 0;
  //delay(300);

  // ask for a signal
  digitalWrite(NEXT_PIN, HIGH);
  while (digitalRead(DIAL_N_PIN) && digitalRead(DIAL_P_PIN)) { }
  delay(1);

  // get the new value
  if (digitalRead(DIAL_N_PIN))
    dialVal = -1;
  else if (digitalRead(DIAL_P_PIN))
    dialVal = 1;
  else
    Serial.println("weird");

  // drop the request signal
  digitalWrite(NEXT_PIN, LOW);
  delay(1);
  
  if (dialVal == -1) Serial.println("left");
  if (dialVal == 1) Serial.println("right");

  return dialVal;
}

