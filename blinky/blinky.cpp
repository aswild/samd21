/*******************************************************************************
 * SAMD21 basic LED blinky demo program.
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
#include "Reset.h"
#include "DigitalIO.h"

//#define FASTBLINKY

#ifndef FASTBLINKY
DigitalOut blue_led(13, 1);        // Blue "stat" LED on pin 13
DigitalOut tx_led(PIN_LED_TXL, 0); // TX LED on pin 26, we use the predefined PIN_LED_TXL to make sure
DigitalOut rx_led(PIN_LED_RXL, 1); // RX LED on pin 25, we use the predefined PIN_LED_RXL to make sure
DigitalOut p11(11, 0);

DigitalIn  button(10, INPUT_PULLUP);

void setup()
{
    //while (!SerialUSB);
    SerialUSB.printf("%s\r\n", "Hello World");
    blue_led = 0;
}

void loop()
{
    rx_led = !rx_led;
    tx_led = !tx_led;

    if (!button)
    {
        rx_led = 1;
        tx_led = 1;
        p11 = 1;
        initiateReset(500);
        while(true);
    }

    delay(200);
}
#else

// blink as fast as possible demo
// about 8MHz/2, or 11-12MHz if we unroll that loop

void setup(void)
{
    USBDevice.detach();
    __disable_irq();
    PORT->Group[0].DIRSET.reg = 1UL << 19;

    for (;;)
    {
        PORT->Group[0].OUTTGL.reg = 1UL << 19;
    }
}

void loop(void) {}

#endif // FASTBLINKY
