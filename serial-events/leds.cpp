#include "Arduino.h"
#include "Neostrip.h"
#include "wiring_private.h"

static constexpr size_t STRIP_LEN        = 8;
static constexpr int    STRIP_BRIGHTNESS = 80;

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

#if 1
void leds_set_state(int state)
{
    Color c = BLACK;
    switch (state)
    {
        case 1: c = BLUE;       break;
        case 2: c = RED;        break;
        case 3: c = MAGENTA;    break;
        case 4: c = GREEN;      break;
    }
    ns.set_all_colors(c);
    ns.write();
}
#else
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
            for (size_t i = 0; i < STRIP_LEN/2; i++)
                ns[i] = RED;
            break;
        case 2:
            // second half blue
            for (size_t i = STRIP_LEN/2; i < STRIP_LEN; i++)
                ns[i] = BLUE;
            break;
        case 3:
            // both of the above
            for (size_t i = 0; i < STRIP_LEN/2; i++)
                ns[i] = RED;
            for (size_t i = STRIP_LEN/2; i < STRIP_LEN; i++)
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
#endif
