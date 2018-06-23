#include "Arduino.h"
#include "DigitalIO.h"
#include "Neostrip.h"
#include "wiring_private.h"

#include "huetable.h"

#define AIN_BRIGHTNESS 9
#define STRIP_LENGTH 8

DigitalOut blue_led(13, 1);        // Blue "stat" LED on pin 13
Neostrip<STRIP_LENGTH> ns(SPI);

const int N_COLORS = 6;
const Color colors[] = { RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA };

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

void loop(void)
{
    static const size_t dh = 360 / STRIP_LENGTH;
    static size_t basehue = 0;

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
    ns.set_brightness(b);
    ns.write(false); // don't wait for tranfer complete (long delay soon)

    unsigned long m1, m2;
    m1 = micros();
    for (size_t i = 0; i < STRIP_LENGTH; i++)
    {
        Color c;
        //m1 = micros();
        //hue_to_color(c, (dh * i) - basehue);
        int index = (dh * i) - basehue;
        if (index >= 360)
            index -= 360;
        if (index < 0)
            index += 360;
        c.i = huetable[index];
        //m2 = micros();
        ns.set_color(i, c);

        if (++basehue >= 360)
            basehue = 0;
    }
    m2 = micros();
    //SerialUSB.printf("%ld\n", m2-m1);

    delay(50);
}

#if 0
// Converts HSV to RGB with the given hue, assuming
// maximum saturation and value
static void hue_to_color(Color& c, float h)
{
    // lots of floating point magic from the internet and scratching my head
    float r, g, b;
    if (h > 360.0f)
        h -= 360.0f;
    if (h < 0.0f)
        h += 360.0f;
    int i = (int)(h / 60.0f);
    float f = (h / 60.0f) - i;
    float q = 1.0f - f;

    switch (i % 6)
    {
        case 0: r = 1; g = f; b = 0; break;
        case 1: r = q; g = 1; b = 0; break;
        case 2: r = 0; g = 1; b = f; break;
        case 3: r = 0; g = q; b = 1; break;
        case 4: r = f; g = 0; b = 1; break;
        case 5: r = 1; g = 0; b = q; break;
        default: r = 0; g = 0; b = 0; break;
    }

    // scale to integers and save in the color
    c.b.red   = (uint8_t)(r * 255.0f);
    c.b.green = (uint8_t)(g * 255.0f);
    c.b.blue  = (uint8_t)(b * 255.0f);
}
#endif
