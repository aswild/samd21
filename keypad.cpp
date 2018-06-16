#include "Arduino.h"
#include "DigitalIO.h"
#include "MPR121.h"
#include "Wire.h"
#include "wiring_private.h"

DigitalOut blue_led(13, 1);        // Blue "stat" LED on pin 13
//DigitalOut tx_led(PIN_LED_TXL, 0); // TX LED on pin 26, we use the predefined PIN_LED_TXL to make sure
//DigitalOut rx_led(PIN_LED_RXL, 1); // RX LED on pin 25, we use the predefined PIN_LED_RXL to make sure
//DigitalOut p11(11, 0);
DigitalIn  button(10, INPUT_PULLUP);

volatile bool keypad_event = false;
static const uint8_t irq_pin = 2;

TwoWire i2c(&sercom2, 4, 3);
MPR121 keypad(i2c);

void keypad_irq(void)
{
    keypad_event = true;
    blue_led = 1;
}

void setup()
{
    while (!SerialUSB);
    SerialUSB.printf("%s\n", "MPR121 Keypad");

    // override pinmux settings from variant.cpp
    i2c.begin();
    pinPeripheral(4, PIO_SERCOM_ALT);
    pinPeripheral(3, PIO_SERCOM_ALT);
    keypad.init(false);
    attachInterrupt(digitalPinToInterrupt(irq_pin), keypad_irq, FALLING);

    blue_led = 0;
}

void loop()
{
    if (keypad_event)
    {
        uint16_t data = keypad.readTouchData();
        SerialUSB.printf("%04x\n", data);
        blue_led = keypad_event = false;
    }
}
