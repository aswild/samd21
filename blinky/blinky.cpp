#include "Arduino.h"
#include "Reset.h"
#include "DigitalIO.h"

DigitalOut blue_led(13, 1);        // Blue "stat" LED on pin 13
DigitalOut tx_led(PIN_LED_TXL, 0); // TX LED on pin 26, we use the predefined PIN_LED_TXL to make sure
DigitalOut rx_led(PIN_LED_RXL, 1); // RX LED on pin 25, we use the predefined PIN_LED_RXL to make sure
DigitalOut p11(11, 0);

DigitalIn  button(10, INPUT_PULLUP);

void setup()
{
    //while (!SerialUSB);
    SerialUSB.printf("%s\r\n", "Hello World");
    blue_led = 0;
}

void loop()
{
    rx_led = !rx_led;
    tx_led = !tx_led;

    if (!button)
    {
        rx_led = 1;
        tx_led = 1;
        p11 = 1;
        initiateReset(500);
        while(true);
    }

    delay(200);
}
