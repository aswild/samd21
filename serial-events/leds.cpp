#include "Arduino.h"
#include "Neostrip.h"
#include "wiring_private.h"

static constexpr size_t STRIP_LEN        = 32;
static constexpr int    STRIP_BRIGHTNESS = 100;

Neostrip<STRIP_LEN> ns(SPI, STRIP_BRIGHTNESS);

void leds_init(void)
{
    ns.init();
    pinPeripheral(13, PIO_OUTPUT);
    ns.write();
}

void leds_set_brightness(uint8_t b)
{
    ns.set_brightness(b);
    ns.write();
}

void leds_set_state(int state)
{
    ns.clear();
    switch (state)
    {
        case 0:
            // blank
            break;
        case 1:
            // first half red
            for (size_t i = 0; i < 16; i++)
                ns[i] = RED;
            break;
        case 2:
            // second half blue
            for (size_t i = 16; i < 32; i++)
                ns[i] = BLUE;
            break;
        case 3:
            // both of the above
            for (size_t i = 0; i < 16; i++)
                ns[i] = RED;
            for (size_t i = 16; i < 32; i++)
                ns[i] = BLUE;
            break;
        case 4:
            // all magenta
            ns.set_all_colors(MAGENTA);
            break;
        default:
            return;
    }
    ns.write();
}
