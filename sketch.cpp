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

    while (!SerialUSB);
    SerialUSB.println("NeoPixel Demo!");
    //ns.dump_rawcolors(SerialUSB);

    blue_led = 0;
}

void loop()
{
    static int i = 0;

    if (!SerialUSB)
    {
        blue_led = 1;
        ns.clear();
        ns.write();
        while (!SerialUSB);
        blue_led = 0;
    }

    uint8_t b = analogRead(AIN_BRIGHTNESS);

    SerialUSB.printf("set brightness %u, color %d\n", b, i);
    ns.set_brightness(b);
    ns.set_all_colors(colors[i]);
    //ns.dump_rawcolors(SerialUSB);
    ns.write(false); // don't wait for tranfer complete (long delay soon)

    i = (i + 1) % N_COLORS;
    delay(1000);
}
