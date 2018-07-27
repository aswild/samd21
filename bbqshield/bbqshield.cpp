/*******************************************************************************
 * BBQ Shield LED Control
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

#define DEBUG_PORT 0
#define DEBUG_PIN  17

#include <stdlib.h>
#include "Arduino.h"
#include "DigitalIO.h"
#include "wiring_private.h"
#include "debug_macros.h"

#include "Neostrip.h"
#include "gradient.h"

#define RANDOM_SEED  0xa1e600
#define STRIP_LENGTH 32
#define BRIGHTNESS   ((30 * 255) / 100)

DigitalOut blue_led(13);
Neostrip<STRIP_LENGTH> ns(SPI, BRIGHTNESS);

DigitalIn button1(7, INPUT_PULLUP);
DigitalIn button2(8, INPUT_PULLUP);

static uint8_t colors[GRADIENT_SIZE];

static volatile int brightness = BRIGHTNESS;
static void brightness_up(void)
{
    brightness += 15;
}
static void brightness_down(void)
{
    brightness -= 15;
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

static inline uint8_t gradient_clamp(int index)
{
    if (index < 0)
        return 0;
    if (index > GRADIENT_SIZE-1)
        return GRADIENT_SIZE-1;
    return index;
}

static void set_colors(void)
{
    for (size_t i = 0; i < STRIP_LENGTH; i++)
        ns.set_color(i, gradient_data[colors[i]]);
}

void setup(void)
{
    blue_led = 1;
    button1.add_interrupt(brightness_up, FALLING);
    button2.add_interrupt(brightness_down, FALLING);
    srand(RANDOM_SEED);

    ns.init();
    DBGINIT();
    pinPeripheral(13, PIO_OUTPUT);

    for (size_t i = 0; i < STRIP_LENGTH; i++)
    {
        colors[i] = random() % GRADIENT_SIZE;
    }
    set_colors();
    ns.write();

    blue_led = 0;
}

void loop(void)
{
    ns.set_brightness(clamp_brightness());
    ns.write(false); // don't wait for tranfer complete (long delay soon)

    DBGHIGH();
    for (size_t i = 0; i < STRIP_LENGTH; i++)
    {
#define RANDOM_RANGE 41
        int delta = (random() % RANDOM_RANGE) - (RANDOM_RANGE / 2);
        colors[i] = gradient_clamp(colors[i] + delta);
    }
    set_colors();
    DBGLOW();

    delay(100);
}
