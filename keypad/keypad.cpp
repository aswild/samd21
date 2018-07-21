/*******************************************************************************
 * SAMD21 test program for the MPR121 capacitive touchpad
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
#include "MPR121.h"
#include "Wire.h"
#include "wiring_private.h"

DigitalOut blue_led(13, 1);        // Blue "stat" LED on pin 13
//DigitalOut tx_led(PIN_LED_TXL, 0); // TX LED on pin 26, we use the predefined PIN_LED_TXL to make sure
//DigitalOut rx_led(PIN_LED_RXL, 1); // RX LED on pin 25, we use the predefined PIN_LED_RXL to make sure
//DigitalOut p11(11, 0);
DigitalIn  button(10, INPUT_PULLUP);

volatile bool keypad_event = false;
static const uint8_t irq_pin = 2;

TwoWire i2c(&sercom2, 4, 3);
MPR121 keypad(i2c);

static void keypad_isr(void)
{
    keypad_event = true;
    blue_led = 1;
}

void setup()
{
    while (!SerialUSB);
    SerialUSB.printf("%s\n", "MPR121 Keypad");

    // override pinmux settings from variant.cpp
    i2c.begin();
    pinPeripheral(4, PIO_SERCOM_ALT);
    pinPeripheral(3, PIO_SERCOM_ALT);
    keypad.init(false);

    pinMode(irq_pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(irq_pin), keypad_isr, FALLING);

    blue_led = 0;
}

void loop()
{
    if (keypad_event)
    {
        uint16_t data = keypad.readTouchData();
        SerialUSB.printf("%04x\n", data);
        blue_led = keypad_event = false;
    }
}
