#include "Arduino.h"
#include "DigitalIO.h"
#include "Neostrip.h"
#include "wiring_private.h"

#define TIMING_DEBUG 0

#define AIN_BRIGHTNESS 9
#define STRIP_LENGTH 8
#define BRIGHTNESS_STEP 2

DigitalOut blue_led(13, 1);        // Blue "stat" LED on pin 13
Neostrip<STRIP_LENGTH> ns(SPI);

DigitalIn rpg_a(6);
DigitalIn rpg_b(7);
DigitalIn rpg_pb(9);

static volatile uint8_t rpg_state;
static volatile int brightness = (10 * 255) / 100;
static volatile int direction = 1;

static inline void read_rpg(void)
{
    rpg_state = (rpg_a << 1) | rpg_b;
}

static void rpg_isr(void)
{
    __disable_irq();
    uint8_t old_state = rpg_state;
    read_rpg();
    if ((old_state ^ (rpg_state<<1)) & 2)
    {
        brightness += BRIGHTNESS_STEP;
    }
    else
    {
        brightness -= BRIGHTNESS_STEP;
    }
    __enable_irq();
}

static void rpg_pb_isr(void)
{
    direction *= -1;
}

static inline uint8_t clamp_brightness(void)
{
    __disable_irq();
    int b = brightness;
    if (b < 0)
    {
        b = brightness = 0;
    }
    if (b > 255)
    {
        b = brightness = 255;
    }
    __enable_irq();
    return (uint8_t)b;
}

void setup(void)
{
    blue_led = 1;

    read_rpg();

    rpg_a.add_interrupt(rpg_isr, CHANGE);
    rpg_a.set_interrupt_filter(true);
    rpg_b.add_interrupt(rpg_isr, CHANGE);
    rpg_b.set_interrupt_filter(true);

    rpg_pb.mode(INPUT_PULLUP);
    rpg_pb.add_interrupt(rpg_pb_isr, FALLING);

    ns.init();
    ns.clear();
    ns.write();

#if 0
    while (!SerialUSB)
        delay(10);
    delay(10);
    SerialUSB.print("NeoPixel Demo!\n");
    //ns.dump_rawcolors(SerialUSB);
#endif

    // pin 13 is shared between SCK and the LED, I don't need SCK so
    // move the mux back to PORT output
    pinPeripheral(13, PIO_OUTPUT);
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

    ns.set_brightness(clamp_brightness());
    //SerialUSB.printf("brightness %d, rpg %u\n", brightness, rpg_state);
    ns.write(false); // don't wait for tranfer complete (long delay soon)

#if TIMING_DEBUG
    unsigned long m1, m2;
    m1 = micros();
#endif
    for (int i = 0; i < STRIP_LENGTH; i++)
    {
        ns.set_color(i, get_color((dh * i) - basehue));

        basehue += 1 * direction;
        if (basehue >= N_STEPS)
            basehue = 0;
        else if (basehue < 0)
            basehue = N_STEPS-1;
    }
#if TIMING_DEBUG
    m2 = micros();
    SerialUSB.printf("%ld\n", m2-m1);
#endif

    delay(17);
}
