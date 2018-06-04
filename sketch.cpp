#include "Arduino.h"

#define BLED 13
#define RN52_GPIO9 17

void setup(void)
{
    SerialUSB.begin(115200);
    Serial1.begin(115200);

    pinMode(BLED, OUTPUT);
    digitalWrite(BLED, HIGH);

    pinMode(RN52_GPIO9, OUTPUT);
    digitalWrite(RN52_GPIO9, HIGH);
    delay(100);
    digitalWrite(RN52_GPIO9, LOW);

    Serial1.flush();
    delay(100);
    while (Serial1.available())
        Serial1.read();

    while (!SerialUSB); // wait for USB host to open the port
    SerialUSB.printf("\r\nSAMD%d Mini!\r\n", 21);
    digitalWrite(BLED, LOW);
}

void loop(void)
{
    while (Serial1.available())
    {
        char c = Serial1.read();
        SerialUSB.write(c);
    }

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
        cmd += c;
        SerialUSB.write(c);
    }
    if (cmd.length())
    {
        //SerialUSB.printf("Send command: '%s'\r\n", cmd.c_str());
        Serial1.printf("%s\r", cmd.c_str());
        delay(100);
    }
}
