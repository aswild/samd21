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
