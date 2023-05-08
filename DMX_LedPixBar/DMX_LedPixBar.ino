// DMX to 10 RGB pixel bar controller using arduino nano
#include "FastLED.h"

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    3
#define CLK_PIN     5
#define LED_TYPE    P9813
#define COLOR_ORDER RGB
#define NUM_LEDS    10
#define BRIGHTNESS  255

CRGB leds[NUM_LEDS];

#include "TM1637.h" //7 segment decoder library
#define CLK 9 //TM1637 pins
#define DIO 6
TM1637 tm1637(CLK, DIO);

#include <Bounce2.h> //debouncer library
const uint8_t BUTTON_PINS[] = {2, 4, 7, 8};
#define NUM_BUTTONS sizeof(BUTTON_PINS)
Bounce * buttons = new Bounce[NUM_BUTTONS];

#include <DMXSerial.h>

uint16_t startChannel = 258;

enum modestate {
  mdmxaddr,
  maud,
  mtemp,
  mcolor,
  mbrt
};

bool cleared = false;
uint8_t bcd[8] = {0};
uint8_t displaymode = mdmxaddr;
uint8_t ledbrt = BRIGHTNESS;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(CRGB(250, 255, 245));
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();

  for (uint8_t i = 0; i < NUM_BUTTONS; i++)
  {
    buttons[i].attach(BUTTON_PINS[i] , INPUT_PULLUP); //setup the bounce instance for the current button
    buttons[i].interval(30); //interval in ms
  }

  tm1637.init();
  tm1637.set(3);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  showDisplay(startChannel);

  DMXSerial.init(DMXReceiver);
  digitalWrite(LED_BUILTIN, LOW);
}

void intbcd(uint16_t num, uint8_t arr[])
{
  uint16_t temp;

  for (uint8_t i = 0; i < 8; i++ )
    arr[i] = 0; //reset value

  for (uint8_t i = 8; num > 0; i-- )
  {
    temp = num / 10 ;
    arr[i - 1] = num - 10 * temp ;
    num = temp ;
  }
}

void showDisplay(uint16_t int_in)
{
  intbcd(int_in, bcd);
  for (uint8_t i = 0; i < 4; i++)
  {
    tm1637.display(i, bcd[4 + i]);
  }
}

void buttonAction(uint8_t btn)
{
  switch (btn)
  {
    case 0:
      if (startChannel == 512)
        startChannel = 0;
      else
        startChannel += 1;
      break;
    case 1:
      //enter
      break;
    case 2:
      displaymode++;
      break;
    case 3:
      if (startChannel == 0)
        startChannel = 512;
      else
        startChannel -= 1;
      break;
  }
}

void loop()
{

  for (uint8_t i = 0; i < NUM_BUTTONS; i++)
  {
    buttons[i].update();
    if (buttons[i].fell())
    {
      //buttonAction(i);
    }
  }

  unsigned long lastPacket = DMXSerial.noDataSince();
  if (lastPacket < 5000)
  {
    if (cleared == true)
    {
      cleared = false;
    }

    // read recent DMX values and set led levels
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
      for (uint8_t j = 0; j < 3; j++)
      {
        leds[i][j] = DMXSerial.read(startChannel + (i * 3) + j);
      }
    }
  }

  else
  {
    if (cleared == false)
    {
      cleared = true;
      FastLED.clear();
    }
  }
  
  EVERY_N_MILLISECONDS(50)
  {
    switch (displaymode)
    {
      case mdmxaddr:
        showDisplay(startChannel);
        break;
      case maud:
        break;
      case mtemp:
        break;
      case mcolor:
        break;
      case mbrt:
        break;
    }
  }

  EVERY_N_MILLISECONDS(25)
  {
    FastLED.show();
  }
}
