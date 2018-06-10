#include "Arduino.h"
#include "Reset.h"

const int BLUE_LED = 13; // Blue "stat" LED on pin 13
const int RX_LED = PIN_LED_RXL; // RX LED on pin 25, we use the predefined PIN_LED_RXL to make sure
const int TX_LED = PIN_LED_TXL; // TX LED on pin 26, we use the predefined PIN_LED_TXL to make sure

const int PB = 10;

void setup() 
{
    pinMode(BLUE_LED, OUTPUT);
    pinMode(RX_LED, OUTPUT);
    pinMode(TX_LED, OUTPUT);
    pinMode(11, OUTPUT);
    digitalWrite(11, LOW);

    pinMode(PB, INPUT_PULLUP);

    digitalWrite(RX_LED, HIGH);
    digitalWrite(TX_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);

    //while (!SerialUSB);
    SerialUSB.printf("%s\r\n", "Hello World");
}

void loop() 
{
    digitalWrite(BLUE_LED, LOW); // Blue LED off

    digitalWrite(RX_LED, LOW); // RX LED on
    delay(200);
    digitalWrite(RX_LED, HIGH); // RX LED off
    digitalWrite(TX_LED, LOW); // TX LED on
    delay(200);
    digitalWrite(TX_LED, HIGH); // TX LED off
#if 0
    digitalWrite(BLUE_LED, HIGH); // Blue LED on
    delay(200);
    digitalWrite(BLUE_LED, LOW); // Blue LED off
#else
    if (!digitalRead(PB))
    {
        digitalWrite(RX_LED, HIGH);
        digitalWrite(TX_LED, HIGH);
        digitalWrite(11, HIGH);
        initiateReset(500);
        while(true);
    }
#endif
}
