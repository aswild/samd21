/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"

void pinMode(pin_size_t api_pin, PinMode mode)
{
    if ((api_pin >= g_APinDescriptionLength) || (g_APinDescription[api_pin].ulPinType == PIO_NOT_A_PIN))
        return;

    const EPortType port = g_APinDescription[api_pin].ulPort;
    const uint32_t pin = g_APinDescription[api_pin].ulPin;

    switch (mode)
    {
        case INPUT:
            // enable input buffer, disable pull
            PORT->Group[port].PINCFG[pin].reg = PORT_PINCFG_INEN;
            // set direction to input
            PORT->Group[port].DIRCLR.reg = 1 << pin;
            break;

        case INPUT_PULLUP:
            PORT->Group[port].PINCFG[pin].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
            PORT->Group[port].DIRCLR.reg = 1 << pin;
            // output value is used for pull direction, cf '22.6.3.2 Input Configuration'
            PORT->Group[port].OUTSET.reg = 1 << pin;
            break;

        case INPUT_PULLDOWN:
            PORT->Group[port].PINCFG[pin].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
            PORT->Group[port].DIRCLR.reg = 1 << pin;
            PORT->Group[port].OUTCLR.reg = 1 << pin;
            break;

        case OUTPUT:
            // enable input buffer so we can still read the pin state
            PORT->Group[port].PINCFG[pin].reg = PORT_PINCFG_INEN;
            // set to output mode
            PORT->Group[port].DIRSET.reg = 1 << pin;
            break;

        default:
            // shouldn't get here
            break;
    }
}

void digitalWrite(pin_size_t api_pin, PinStatus val)
{
    // Unchecked! For speed, assume that the pin is valid. The mode must already be configured.
    // If this pin is an input with pullup/pulldown enabled, this will change the pull direction
    // as a side effect of how PIO works on SAMD21.
    const EPortType port = g_APinDescription[api_pin].ulPort;
    const uint32_t pin = g_APinDescription[api_pin].ulPin;

    if (val == LOW)
        PORT->Group[port].OUTCLR.reg = 1 << pin;
    else if (val == HIGH)
        PORT->Group[port].OUTSET.reg = 1 << pin;
}

PinStatus digitalRead(pin_size_t api_pin)
{
    // Unchecked! For speed, assume that the pin is valid. The mode must already be configured.
    const EPortType port = g_APinDescription[api_pin].ulPort;
    const uint32_t pin = g_APinDescription[api_pin].ulPin;

    return (PORT->Group[port].IN.reg & (1 << pin)) ? HIGH : LOW;
}
