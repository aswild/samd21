#include "Arduino.h"
#include "DigitalIO.h"
#include "Neostrip.h"
#include "wiring_private.h"

#define TIMING_DEBUG 0

#define AIN_BRIGHTNESS 9
#define STRIP_LENGTH 8

DigitalOut blue_led(13, 1);        // Blue "stat" LED on pin 13
Neostrip<STRIP_LENGTH> ns(SPI);

void setup(void)
{
    ns.init();
    //ns.set_all_colors(WHITE);
    ns.clear();
    ns.write();

    analogReadResolution(8);

    // pin 13 is shared between SCK and the LED, I don't need SCK so
    // move the mux back to PORT output
    pinPeripheral(13, PIO_OUTPUT);
    blue_led = 1;

#if 0
    while (!SerialUSB)
        delay(10);
    delay(10);
    SerialUSB.print("NeoPixel Demo!\n");
    //ns.dump_rawcolors(SerialUSB);
#endif

    blue_led = 0;
}

static const int N_STEPS = 256 * 6;
static Color get_color(int n)
{
    Color c = { .i = 0 };

    while (n >= N_STEPS)
        n -= N_STEPS;
    while (n < 0)
        n += N_STEPS;

    uint8_t stage = n / 256;
    uint8_t step = n % 256;

    switch (stage)
    {
        case 0:
            // red -> yellow
            c.b.red = 0xff;
            c.b.green = step;
            break;

        case 1:
            // yellow -> green
            c.b.red = 0xff - step;
            c.b.green = 0xff;
            break;

        case 2:
            // green -> cyan
            c.b.green = 0xff;
            c.b.blue = step;
            break;

        case 3:
            // cyan -> blue
            c.b.green = 0xff - step;
            c.b.blue = 0xff;
            break;

        case 4:
            // blue -> magenta
            c.b.red = step;
            c.b.blue = 0xff;
            break;

        case 5:
            // magenta -> red
            c.b.blue = 0xff - step;
            c.b.red = 0xff;
            break;
    }

    return c;
}

void loop(void)
{
    static const int dh = N_STEPS / STRIP_LENGTH;
    static int basehue = 0;

#if 0
    if (!SerialUSB)
    {
        blue_led = 1;
        ns.clear();
        ns.write();
        while (!SerialUSB)
            delay(10);
        blue_led = 0;
    }
#endif

    uint8_t b = analogRead(AIN_BRIGHTNESS);
    if (b == 1)
        b = 0;
    ns.set_brightness(b);
    ns.write(false); // don't wait for tranfer complete (long delay soon)

#if TIMING_DEBUG
    unsigned long m1, m2;
    m1 = micros();
#endif
    for (int i = 0; i < STRIP_LENGTH; i++)
    {
        ns.set_color(i, get_color((dh * i) - basehue));

        if (++basehue >= N_STEPS)
            basehue = 0;
    }
#if TIMING_DEBUG
    m2 = micros();
    SerialUSB.printf("%ld\n", m2-m1);
#endif

    delay(17);
}
