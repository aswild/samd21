/*******************************************************************************
 * SAMD21 test program for NeoPixels
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
#include "Neostrip.h"
#include "MPR121.h"
#include "Wire.h"
#include "wiring_private.h"

#define STRIP_LENGTH 8
#define BRIGHTNESS_STEP 2

#define KEYPAD_SDA_PIN 4
#define KEYPAD_SCL_PIN 3
#define KEYPAD_IRQ_PIN 2

Neostrip<STRIP_LENGTH> ns(SPI);
TwoWire i2c(&sercom2, KEYPAD_SDA_PIN, KEYPAD_SCL_PIN);
MPR121 keypad(i2c);

DigitalOut blue_led(13, HIGH);
DigitalIn keypad_irq(KEYPAD_IRQ_PIN, INPUT_PULLUP);
DigitalIn rpg_a(6);
DigitalIn rpg_b(7);
DigitalIn rpg_pb(9, INPUT_PULLUP);

static volatile uint8_t rpg_state;
static volatile int brightness = (10 * 255) / 100;
static volatile bool invert = false;
static volatile bool keypad_event = false;

static const int direction = 1;

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
    //direction *= -1;
    invert = !invert;
}

static void keypad_isr(void)
{
    keypad_event = true;
    blue_led = 1;
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

    rpg_pb.add_interrupt(rpg_pb_isr, FALLING);
    keypad_irq.add_interrupt(keypad_isr, FALLING);

    ns.init();
    // pin 13 is shared between SCK and the LED, I don't need SCK so
    // switch back to PORT output
    pinPeripheral(13, PIO_OUTPUT);
    ns.clear();
    ns.write();

    // override pinmux settings from variant.cpp
    i2c.begin();
    pinPeripheral(KEYPAD_SDA_PIN, PIO_SERCOM_ALT);
    pinPeripheral(KEYPAD_SCL_PIN, PIO_SERCOM_ALT);
    keypad.init(false);

    blue_led = 0;
}

static const int N_STEPS = 256 * 6;
static Color get_color(int n)
{
    while (n >= N_STEPS)
        n -= N_STEPS;
    while (n < 0)
        n += N_STEPS;

    Color c = { .i = 0 };
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
    static uint16_t keypad_data = 0;

    if (keypad_event)
    {
        keypad_data = keypad.readTouchData();
        blue_led = keypad_event = false;
    }

    for (unsigned i = 0; i < 8; i++)
    {
        if (( invert && ((keypad_data & (1<<i)) == 0)) ||
            (!invert && ((keypad_data & (1<<i)) != 0)))
            ns.set_color(i, BLACK);
    }

    ns.set_brightness(clamp_brightness());
    //SerialUSB.printf("brightness %d, rpg %u\n", brightness, rpg_state);
    ns.write(false); // don't wait for tranfer complete (long delay soon)

    for (unsigned i = 0; i < STRIP_LENGTH; i++)
    {
        ns.set_color(i, get_color((dh * i) - basehue));

        basehue += 1 * direction;
        if (basehue >= N_STEPS)
            basehue = 0;
        else if (basehue < 0)
            basehue = N_STEPS-1;
    }

    delay(17);
}
