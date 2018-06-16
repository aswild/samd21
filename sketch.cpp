#include "Arduino.h"
#include "DigitalIO.h"

DigitalOut blue_led(13, HIGH);
DigitalOut gpio9(17, HIGH);

void Serial1_IrqHook(void)
{
    while (Serial1.available())
        SerialUSB.write(Serial1.read());
}

void setup(void)
{
    SerialUSB.begin(115200);
    Serial1.begin(115200);

    delay(100);
    gpio9 = 0;

    Serial1.flush();
    delay(100);
    while (Serial1.available())
        Serial1.read();

    while (!SerialUSB); // wait for USB host to open the port
    SerialUSB.printf("\r\nSAMD%d Mini!\r\n", 21);
    blue_led = 0;
}

void loop(void)
{
    while (Serial1.available())
        SerialUSB.write(Serial1.read());

    SerialUSB.write("> ");
    String cmd = "";
    while (true)
    {
        while (!SerialUSB.available());
        char c = SerialUSB.read();
        if (c == '\r')
        {
            SerialUSB.print("\r\n");
            break;
        }
        else if (c == '\b')
        {
            cmd.remove(cmd.length()-1, 1);
            SerialUSB.print("\b \b");
        }
        else
        {
            cmd += c;
            SerialUSB.write(c);
        }
    }
    if (cmd.length())
    {
        //SerialUSB.printf("Send command: '%s'\r\n", cmd.c_str());
        Serial1.printf("%s\r", cmd.c_str());
        delay(100);
    }
}
