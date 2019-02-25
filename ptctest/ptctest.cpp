#include "Arduino.h"
#include "Adafruit_FreeTouch.h"
#include "DigitalIO.h"

#define DEBUG_PORT 0
#define DEBUG_PIN  14
#include "debug_macros.h"

Adafruit_FreeTouch touch(A0, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);
DigitalOut led(13);

static constexpr uint16_t threshold = 0x03f0;

void setup(void)
{
    DBGINIT();
    touch.begin();
    DBGLOW();
}

void loop(void)
{
    static bool last = false;
    uint16_t val;

    DBGHIGH();
    val = touch.measure();
    DBGLOW();

    bool current = val > threshold;
    if (last != current)
    {
        led = last = current;
    }

#if 0
    SerialUSB.printf("%04x\n", val);
    delay(500);
#else
    delay(10);
#endif
}
