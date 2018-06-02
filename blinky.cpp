#include "Arduino.h"

const int BLUE_LED = 13; // Blue "stat" LED on pin 13
const int RX_LED = PIN_LED_RXL; // RX LED on pin 25, we use the predefined PIN_LED_RXL to make sure
const int TX_LED = PIN_LED_TXL; // TX LED on pin 26, we use the predefined PIN_LED_TXL to make sure

bool ledState = LOW;

void setup() 
{
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RX_LED, OUTPUT);
  pinMode(TX_LED, OUTPUT);
  digitalWrite(RX_LED, HIGH);
  digitalWrite(TX_LED, HIGH);
  digitalWrite(BLUE_LED, LOW);
}

void loop() 
{
  digitalWrite(RX_LED, LOW); // RX LED on
  delay(200);
  digitalWrite(RX_LED, HIGH); // RX LED off
  digitalWrite(TX_LED, LOW); // TX LED on
  delay(200);
  digitalWrite(TX_LED, HIGH); // TX LED off
  digitalWrite(BLUE_LED, HIGH); // Blue LED on
  delay(200);
  digitalWrite(BLUE_LED, LOW); // Blue LED off
}
