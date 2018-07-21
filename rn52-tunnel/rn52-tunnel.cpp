/*******************************************************************************
 * SAMD21 serial port tunnel
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

DigitalOut blue_led(13, HIGH);
DigitalOut gpio9(17, HIGH);

#if 0
void Serial1_IrqHook(void)
{
    while (Serial1.available())
        SerialUSB.write(Serial1.read());
}
#endif

void setup(void)
{
    Serial1.setRxBufferSize(256);
    Serial1.begin(115200);

    delay(100);
    gpio9 = 0;

    Serial1.flush();
    delay(100);
    while (Serial1.available())
        Serial1.read();

    SerialUSB.begin(115200);
    while (!SerialUSB); // wait for USB host to open the port
    SerialUSB.printf("\r\nSAMD%d Mini!\r\n", 21);
    blue_led = 0;
}

void loop(void)
{
    while (Serial1.available())
    {
        while (Serial1.available())
            SerialUSB.write(Serial1.read());
        delay(10);
    }

    SerialUSB.write("> ");
    char cmdbuf[16];
    size_t cmdlen = SerialUSB.readLine(cmdbuf, sizeof(cmdbuf), true);
    if (cmdlen) {
        //SerialUSB.printf("count=%u buf='%s'\r\n", cmdlen, cmdbuf);
        Serial1.printf("%s\r", cmdbuf);
        delay(50);
    }
}
