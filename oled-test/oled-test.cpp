#include "Arduino.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SH110X.h"
#include "DigitalIO.h"
#include "Wire.h"
#include "wiring_private.h"

// variant puts the only I2C interface on two extra pins that aren't on the breadboard headers. And
// as usual Arduino makes it difficult to reassign pinmuxes for sercom and what not. Rather than
// editing variant.h, just define the TwoWire peripheral and interrupt handler here.

// Use pins A1 (PB08) and A2 (PB09) with SERCOM4 pins 0/1
static constexpr pin_size_t M_SCL = PIN_A1;
static constexpr pin_size_t M_SDA = PIN_A2;

static TwoWire Wire_A12(&sercom4, PIN_A1, PIN_A2);
void SERCOM4_Handler()
{
    Wire_A12.onService();
}

static Adafruit_SH1107 display(64, 128, &Wire_A12);

void setup(void)
{
    // Oh, the things I go through to work around how Arduino's variant framework can't handle more
    // than one peripheral function on any given pin...
    // display.begin() will call begin() on the TwoWire interface, which will call pinPeripheral
    // with whatever's defined in ulPinType for our pins, which in this case is analog. Making
    // matters worse, display.begin() will then go do stuff that talks over I2C (to initialize the
    // display) that requires the pins to be muxed correctly.
    // There was no way to fix this without patching library code, so I added a way to prevent
    // TwoWire::begin() from calling pinPeripheral().
    Wire_A12.noPinPeripheral();
    pinPeripheral(M_SCL, PIO_SERCOM_ALT);
    pinPeripheral(M_SDA, PIO_SERCOM_ALT);

    display.begin(0x3C, false); // address 0x3C, false because we don't have a reset pin wired up
    display.display();
    delay(500);

    Adafruit_SH1107& d = display;
    d.clearDisplay();
    d.setRotation(3);
    d.setTextColor(SH110X_WHITE);
#if 1
    d.setTextSize(1);
    d.setCursor(0, 0);
    d.print(
        "Mode:  Spacebar      "
        "Shots: 10            "
        "Hold:  100ms         "
        "Intvl: 100ms         "
        "                     "
        "Avg: 1.234ms         "
        "Min-Max 0.978-10.324 "
        "Stdev: 0.022         "
    );
#else
    d.setTextSize(2);
    d.setCursor(0, 0);
    d.print(
        "          \n"
        "MEOW MEOW \n"
        "BITCHES :3\n"
        "          \n"
        );
#endif

    d.display();
}

void loop(void)
{
    static DigitalOut led(13);

    led.toggle();
    delay(500);
}
