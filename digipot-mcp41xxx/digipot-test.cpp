/*******************************************************************************
 * SAMD21 MCP41XXX digital potentiometer test program
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
#include "MCP41XXX.h"

#include <stdlib.h> // for atoi()

MCP41XXX pot(SPI, 10, 2000000);

void setup(void)
{
    SerialUSB.begin(115200);
    while (!SerialUSB); // wait for USB host to open the port
    SerialUSB.print("SAMD21 MCP41XXX DigiPot test\r\n");

    pot.init();
    //SPI.setHardwareSs(true, 10);
}

void loop(void)
{
    SerialUSB.write("> ");
    char cmdbuf[16];
    size_t cmdlen = SerialUSB.readLine(cmdbuf, sizeof(cmdbuf), true);
    if (cmdlen)
    {
        //SerialUSB.printf("count=%u buf='%s'\r\n", cmdlen, cmdbuf);
        uint8_t pos = static_cast<uint8_t>(strtoul(cmdbuf, NULL, 0));
        SerialUSB.printf("set wiper %u\r\n", pos);
        pot.set_wiper(pos);
        delay(50);
    }
}
