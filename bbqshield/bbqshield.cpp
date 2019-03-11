/*******************************************************************************
 * Neopixel fire/glowing effect
 *
 * Copyright (C) 2018-2019 Allen Wild <allenwild93@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "Arduino.h"
#include "DigitalIO.h"
#include "wiring_private.h"
#include "Neostrip.h"
#include "Timer.h"

#include "gradient.h"
#include "gradient_bluegreen.h"

// RNG
#include <stdlib.h>
#define RANDOM       lrand48
#define SRAND        srand48
#define RANDOM_SEED  0xAAC0FFEE

template<typename T>
static inline T random_range(T a, T b)
{
    return static_cast<T>((RANDOM() % (b - a)) + a);
}

// debug pin PA17 - pin 13, also the blue LED
//#define DEBUG_PORT 0
//#define DEBUG_PIN  17
#include "debug_macros.h"

// how many pixels in the strip?
#define STRIP_LENGTH    32

// 0: two buttons to manually step brightness
// 1: automatic brightness pulsing, one enable/disable button
#define AUTO_BRIGHTNESS 1

// starting brightness and adjustment step
#define DEF_BRIGHTNESS  100
#define BRIGHTNESS_STEP 20

// number of steps to fade between states, and delay between each
// FADE_STEPS should be 2^n+1 to make dividing by FADE_STEPS-1 fast
#define FADE_STEPS      257
#define STEP_DELAY_MS   10

// static objects
Neostrip<STRIP_LENGTH> ns(SPI, DEF_BRIGHTNESS);
DigitalOut blue_led(13);

// SPI1 object. SERCOM4, all pins are -1 so SPIClass::begin() doesn't
// screw with the pin muxing. We'll use pinPeripheral manually in setup()
SPIClass SPI1(&sercom4, -1, -1, -1, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_3);
Neostrip<STRIP_LENGTH> ns1(SPI1, DEF_BRIGHTNESS);
static_assert(GRADIENT_BLUEGREEN_SIZE >= GRADIENT_SIZE);

static void timer_isr(void) { blue_led = 0; }
Timer heartbeat_timer(TC5, timer_isr);
DECLARE_TIMER_HANDLER(TC5, heartbeat_timer)

// fade endpoint arrays (values are indicies to the gradient)
static uint8_t colors1[STRIP_LENGTH];
static uint8_t colors2[STRIP_LENGTH];

#if AUTO_BRIGHTNESS
// one button to enable/disable
static volatile bool disabled = false;
static void pb_onoff_isr(void) { disabled = !disabled; }
DigitalIn pb_onoff(8, INPUT_PULLUP);
#else
// brightness control variables
static volatile int brightness = DEF_BRIGHTNESS;
static volatile bool brightness_update = true;
DigitalIn pb_bright_up(8, INPUT_PULLUP);
DigitalIn pb_bright_down(9, INPUT_PULLUP);

// brightness control ISRs
static void brightness_up(void)   { brightness += BRIGHTNESS_STEP; brightness_update = true; }
static void brightness_down(void) { brightness -= BRIGHTNESS_STEP; brightness_update = true; }
static inline uint8_t clamp_brightness(void)
{
    __disable_irq();
    int b = brightness;
    if (b < 0)
    {
        b = brightness = 0;
    }
    else if (b > 255)
    {
        b = brightness = 255;
    }
    brightness_update = false;
    __enable_irq();
    return (uint8_t)b;
}
#endif // AUTO_BRIGHTNESS

void setup(void)
{
    // init debug pin, RNG, and brightness button IRQs
    DBGINIT();
    DBGHIGH();
    SRAND(RANDOM_SEED);
#if AUTO_BRIGHTNESS
    pb_onoff.add_interrupt(pb_onoff_isr, FALLING);
#else
    pb_bright_up.add_interrupt(brightness_up, FALLING);
    pb_bright_down.add_interrupt(brightness_down, FALLING);
#endif

    // init the SPI and move pin 13 back to GPIO for debug rather than SCLK
    SPI.begin();
    pinPeripheral(13, PIO_OUTPUT);

    // set up 2nd SPI and use pin 15 (PB08, which Arduino calls "analog" A1)
    // as SERCOM4[0] in SERCOM-ALT mode
    SPI1.begin();
    pinPeripheral(15, PIO_SERCOM_ALT);

    // init and clear neostrip
    ns.init(false);
    ns1.init(false);
    ns.write(false);
    ns1.write(false);

    // set up the heartbeat timer, flashes every main cycle through loop()
    heartbeat_timer.init();
    heartbeat_timer.set_us(20000);

    // set the fade endpoints and load the first frame into the strip's buffer
    for (size_t i = 0; i < STRIP_LENGTH; i++)
    {
        colors1[i] = RANDOM() % GRADIENT_SIZE;
        colors2[i] = RANDOM() % GRADIENT_SIZE;
        ns[i] = gradient_data[colors1[i]];
        ns1[i] = gradient_bluegreen_data[colors1[i]];
    }

    ns.wait_for_complete();
    ns1.wait_for_complete();
    DBGLOW();
}

void loop(void)
{
    static uint8_t *cstart = colors1;
    static uint8_t *cstop  = colors2;

#if AUTO_BRIGHTNESS
    static constexpr uint8_t bmin = 40;
    static constexpr uint8_t bmax = 125;
    static uint8_t bstart, bend, brightness = DEF_BRIGHTNESS;
    static uint32_t bstep = 0;
    static uint32_t bsteps = 0;
#endif

    blue_led = 1;
    heartbeat_timer.start();

    for (int i = 0; i < FADE_STEPS; i++)
    {
#if AUTO_BRIGHTNESS
        if (disabled)
        {
            // disable button pushed, clear strip
            ns.clear();
            ns1.clear();
            ns.write();
            ns1.write();
            // sleep until enable button toggles again
            while (disabled)
                __WFI();
        }

        if (bstep == bsteps)
        {
            // end of brightness fade, set new endpoint and time
            bstart = brightness;
            bend = random_range(bmin, bmax);
            bstep = 0;
            bsteps = random_range(20, 80);
        }
        else
        {
            // FIXME: it's really glitchy without the floating-point cast, the integer range is
            // probably too small and all the numbers here should be scaled up significantly.
            // Using floats is a lazy but functional workaround.
            brightness = bstart + ((1.0*bstep * (bend - bstart)) / (bsteps-1));
            bstep++;
        }
        ns.set_brightness(brightness);
        ns1.set_brightness(brightness);
#else
        if (brightness_update)
        {
            ns.set_brightness(clamp_brightness());
            ns1.set_brightness(clamp_brightness());
        }
#endif
        // write current frame
        ns.write();
        ns1.write();

        // prepare the next frame with a linear interpolation
        for (int j = 0; j < STRIP_LENGTH; j++)
        {
            uint8_t gc = cstart[j] + ((i * (cstop[j] - cstart[j])) / (FADE_STEPS-1));
            ns[j] = gradient_data[gc];
            ns1[j] = gradient_bluegreen_data[gc];
        }

        // wait for next frame. The last frame of the fade won't actually get
        // displayed until the next time through loop(), where it's the preloaded
        // first frame of the next fade.
        delay(STEP_DELAY_MS);
    }

    // randomize the starting points and then flip buffers
    for (int i = 0; i < STRIP_LENGTH; i++)
        cstart[i] = RANDOM() % GRADIENT_SIZE;

    if (cstart == colors1)
    {
        cstart = colors2;
        cstop = colors1;
    }
    else
    {
        cstart = colors1;
        cstop = colors2;
    }
}
