#include "Arduino.h"
#include "DigitalIO.h"
#include "Keyboard.h"
#include "Timer.h"

//#define DEBUG_PORT 0
//#define DEBUG_PIN  17
#include "debug_macros.h"

#include <atomic>

static DigitalOut blue_led(13);
static DigitalIn btn1(10, INPUT_PULLDOWN);
static DigitalIn btn2(11, INPUT_PULLDOWN);

static std::atomic_bool btn1_pressed(false);
static std::atomic_bool btn2_pressed(false);

static Timer led_timer(TC3);
DECLARE_TIMER_HANDLER(TC3, led_timer);

static void led_pulse()
{
    blue_led = HIGH;
    led_timer.start();
}

void setup()
{
    SerialUSB.begin(115200);
    Keyboard.begin();

    btn1.add_interrupt([] { btn1_pressed = true; }, RISING);
    btn2.add_interrupt([] { btn2_pressed = true; }, RISING);
    btn1.set_interrupt_filter(true);
    btn2.set_interrupt_filter(true);

    led_timer.init();
    led_timer.set_us(200 * 1000);
    led_timer.set_callback([] { blue_led = LOW; });
}

void loop()
{
    if (btn1_pressed)
    {
        led_pulse();
        SerialUSB.write("B1\n");
        btn1_pressed = false;
    }

    if (btn2_pressed)
    {
        constexpr auto key = ' ';

        led_pulse();
        SerialUSB.write("B2\n");
        Keyboard.press(key);
        Keyboard.release(key);
        btn2_pressed = false;
    }

    __WFI();
}
