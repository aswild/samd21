/*******************************************************************************
 * BBQ Leona Shield LED Control
 *
 * Copyright (C) 2018 Allen Wild <allenwild93@gmail.com>
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
#include "gradient.h"

// RNG
#include <stdlib.h>
#define RANDOM       lrand48
#define SRAND        srand48
#define RANDOM_SEED  0xAAC0FFEE

// debug pin PA17 - pin 13, also the blue LED
#define DEBUG_PORT 0
#define DEBUG_PIN  17
#include "debug_macros.h"

// how many pixels in the strip?
#define STRIP_LENGTH    90

// starting brightness and adjustment step
#define DEF_BRIGHTNESS  215
#define BRIGHTNESS_STEP 20

// number of steps to fade between states, and delay between each
// FADE_STEPS should be 2^n+1 to make dividing by FADE_STEPS-1 fast
#define FADE_STEPS      257
#define STEP_DELAY_MS   10

// static objects
Neostrip<STRIP_LENGTH> ns(SPI, DEF_BRIGHTNESS);
DigitalIn pb_bright_up(8, INPUT_PULLUP);
DigitalIn pb_bright_down(9, INPUT_PULLUP);

// fade endpoint arrays (values are indicies to the gradient)
static uint8_t colors1[STRIP_LENGTH];
static uint8_t colors2[STRIP_LENGTH];

// brightness control variables
static volatile int brightness = DEF_BRIGHTNESS;
static volatile bool brightness_update = true;

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

void setup(void)
{
    // init debug pin, RNG, and brightness button IRQs
    DBGINIT();
    DBGHIGH();
    SRAND(RANDOM_SEED);
    pb_bright_up.add_interrupt(brightness_up, FALLING);
    pb_bright_down.add_interrupt(brightness_down, FALLING);

    // init the SPI and move pin 13 back to GPIO for debug rather than SCLK
    SPI.begin();
    pinPeripheral(13, PIO_OUTPUT);

    // init and clear neostrip
    ns.init(false);
    ns.write(false);

    // set the fade endpoints and load the first frame into the strip's buffer
    for (size_t i = 0; i < STRIP_LENGTH; i++)
    {
        colors1[i] = RANDOM() % GRADIENT_SIZE;
        colors2[i] = RANDOM() % GRADIENT_SIZE;
        ns.set_color(i, gradient_data[colors1[i]]);
    }

    ns.wait_for_complete();
    DBGLOW();
}

void loop(void)
{
    static uint8_t *cstart = colors1;
    static uint8_t *cstop  = colors2;

    DBGLOW();
    for (int i = 0; i < FADE_STEPS; i++)
    {
        // write current frame
        if (brightness_update)
            ns.set_brightness(clamp_brightness());
        ns.write(false);

        // prepare the next frame with a linear interpolation
        for (int j = 0; j < STRIP_LENGTH; j++)
        {
            uint8_t gc = cstart[j] + ((i * (cstop[j] - cstart[j])) / (FADE_STEPS-1));
            ns.set_color(j, gradient_data[gc]);
        }

        // wait for next frame. The last frame of the fade won't actually get
        // displayed until the next time through loop(), where it's the preloaded
        // first frame of the next fade.
        delay(STEP_DELAY_MS);
    }
    DBGHIGH();

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
