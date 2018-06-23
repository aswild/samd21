#include "Arduino.h"
#include "DigitalIO.h"
#include "Neostrip.h"
#include "wiring_private.h"

#define AIN_BRIGHTNESS 9

DigitalOut blue_led(13, 1);        // Blue "stat" LED on pin 13

Neostrip<8> ns(SPI);

const int N_COLORS = 6;
const Color colors[] = { RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA };

void setup()
{
    ns.init();
    ns.clear();
    ns.write();

    analogReadResolution(8);

    // pin 13 is shared between SCK and the LED, I don't need SCK so
    // move the mux back to PORT output
    pinPeripheral(13, PIO_OUTPUT);
    blue_led = 1;

    while (!SerialUSB)
        delay(10);
    delay(10);
    SerialUSB.print("NeoPixel Demo!\n");
    //ns.dump_rawcolors(SerialUSB);

    blue_led = 0;
}

void loop()
{
    static const unsigned long looptime_ms = 10;
    static const size_t colorchange_count = 1000 / looptime_ms;
    static size_t ci = 0;
    static size_t loopcount = colorchange_count-1;

    if (!SerialUSB)
    {
        blue_led = 1;
        ns.clear();
        ns.write();
        while (!SerialUSB)
            delay(10);
        blue_led = 0;
    }

    uint8_t b = analogRead(AIN_BRIGHTNESS);
    ns.set_brightness(b);
    ns.write(false); // don't wait for tranfer complete (long delay soon)

    if (++loopcount == colorchange_count)
    {
        SerialUSB.printf("set brightness %u, color %d\n", b, ci);
        ns.set_all_colors(colors[ci]);
        ci = (ci + 1) % N_COLORS;
        loopcount = 0;
    }

    delay(looptime_ms);
}
