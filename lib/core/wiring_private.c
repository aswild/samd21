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
#include "wiring_private.h"

int pinPeripheral(pin_size_t api_pin, EPioType peripheral)
{
    if (api_pin >= g_APinDescriptionLength)
        return -1;

    const EPortType port = g_APinDescription[api_pin].ulPort;
    const uint32_t pin = g_APinDescription[api_pin].ulPin;

    switch (peripheral)
    {
        // peripheral modes - the enum values here match what goes into the PMUX register
        case PIO_EXTINT:        // peripheral A
        case PIO_ANALOG:        // peripheral B
        case PIO_SERCOM:        // peripheral C
        case PIO_SERCOM_ALT:    // peripheral D
        case PIO_TIMER:         // peripheral E
        case PIO_TIMER_ALT:     // peripheral F
        case PIO_COM:           // peripheral G
        case PIO_AC_CLK:        // peripheral H
        {
            // PMUX registers are organized by even and odd pins
            if (pin & 1)
                PORT->Group[port].PMUX[pin / 2].bit.PMUXO = peripheral;
            else
                PORT->Group[port].PMUX[pin / 2].bit.PMUXE = peripheral;

            // enable pinmux rather than PIO mode
            PORT->Group[port].PINCFG[pin].bit.PMUXEN = 1;
            break;
        }

        // Weird mode, just disable pinmux and go back to whatever digital PIO state was configured
        case PIO_DIGITAL:
            PORT->Group[port].PINCFG[pin].bit.PMUXEN = 0;
            break;

        // other modes are just aliases for pinMode() calls
        case PIO_INPUT:
            pinMode(api_pin, INPUT);
            break;
        case PIO_INPUT_PULLUP:
            pinMode(api_pin, INPUT_PULLUP);
            break;
        case PIO_OUTPUT:
            pinMode(api_pin, OUTPUT);
            break;

        default:
            return -1;
    }

    return 0;
}
