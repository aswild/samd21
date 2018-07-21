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
