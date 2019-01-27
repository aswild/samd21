#include "Arduino.h"
#include "PWM.h"

#define DEBUG_PORT 0
#define DEBUG_PIN  17
#include "debug_macros.h"

PWM pwm(100 * 1000);

int chan_p5, chan_p6;

void setup(void)
{
    //while (!SerialUSB);
    //SerialUSB.printf("hello world\n");

    DBGINIT();
    DBGHIGH();
    pwm.init();

    //SerialUSB.printf("prescaler=%lu\r\n", pwm.presc_div);

    chan_p5 = pwm.enable_pin(5);
    chan_p6 = pwm.enable_pin(6);

    pwm.set_chan(chan_p5, 50, false);
    pwm.set_chan(chan_p6, 0.25, false);

    pwm.start();
    DBGLOW();
}

void loop(void)
{
#if 0
    constexpr uint32_t us_min = 10;
    constexpr uint32_t us_max = 100;
    constexpr uint32_t us_step = 10;

    for (unsigned i = 0; i < ((us_max - us_min)/us_step); i++)
    {
        pwm.set_chaneriod_us(us_min + i*us_step, false);
        delay(500);
        pwm.set_chan(-1, 50);
        pwm.update();
        delay(500);
    }
#else
    DBGHIGH();
    pwm.set_chan(chan_p5, 0.25, false);
    delay(500);
    pwm.set_chan(chan_p6, 0.25, false);
    pwm.update();
    delay(1000);

    pwm.set_chan(chan_p5, 75, false);
    delay(500);
    pwm.set_chan(chan_p6, 75, false);

    pwm.update();
    DBGLOW();
    delay(2000);
#endif
}
