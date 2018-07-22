/*******************************************************************************
 * SAMD21 software timer (TC) test program
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
#include "Timeout.h"

#include <stdlib.h> // for strtoul()

#define DEBUG_PORT 0
#define DEBUG_PIN  17
#include "debug_macros.h"

DigitalOut status_led(13);
DigitalIn button(9);

static void timeout_isr(void)
{
    DBGLOW();
}

static void button_isr(void)
{
    DBGHIGH();
    timeout_start();
}

void setup(void)
{
    status_led = 1;
    DBGINIT();
    pinMode(10, OUTPUT);
    digitalWrite(10, 0);
    timeout_init();
    timeout_set_callback(&timeout_isr);

    button.mode(INPUT);
    button.add_interrupt(button_isr, FALLING);
    status_led = 0;

    SerialUSB.begin(115200);
    while (!SerialUSB); // wait for USB host to open the port
    SerialUSB.print("SAMD21 TC Timer Test\r\n");
    button_isr();
}

void loop(void)
{
    SerialUSB.write("> ");
    char cmdbuf[16];
    size_t cmdlen = SerialUSB.readLine(cmdbuf, sizeof(cmdbuf), true);
    if (cmdlen)
    {
        //SerialUSB.printf("count=%u buf='%s'\r\n", cmdlen, cmdbuf);
        uint32_t val = strtoul(cmdbuf, NULL, 0);
        if (val)
        {
            timeout_set_us(val);
            uint16_t cc = TC5->COUNT16.CC[0].reg;
            uint32_t prescaler = TC5->COUNT16.CTRLA.bit.PRESCALER;
            SerialUSB.printf("%luus, prescaler=%lu, count=%u\r\n",
                             val, prescaler, cc);
            button_isr();
        }
    }
}
