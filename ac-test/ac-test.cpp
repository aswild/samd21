#include "Arduino.h"
#include "DigitalIO.h"
#include "wiring_private.h"

static DigitalOut blue_led(13, HIGH);

static void init_ac()
{
    // disable the AC IRQ
    NVIC_DisableIRQ(AC_IRQn);
    NVIC_ClearPendingIRQ(AC_IRQn);
    NVIC_SetPriority(AC_IRQn, 0);

    // Enable AC in power-management (default arduino core doesn't do this for AC like for other peripherals)
    PM->APBCMASK.reg |= PM_APBCMASK_AC;

    // Set up clocks. Note: GCLK0 is 48MHz CPU clock, GCLK1 is 32.768KHz crystal clock
    while (GCLK->STATUS.bit.SYNCBUSY);
    // AC analog clock. Max 64KHz (cf '37.6 Maximum Clock Frequencies' Table 37-6), so use GCLK1 @ 32.768KHz
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCM_AC_ANA) | GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_CLKEN;
    // AC digital clock, used for sampling & filtering. Max 48MHz, but use GCLK1 to 'slow down' filtering
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCM_AC_DIG) | GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_CLKEN;

    // wait for clock domain sync
    while (GCLK->STATUS.bit.SYNCBUSY);
    while (AC->STATUSB.bit.SYNCBUSY);

    // reset and disable AC
    AC->CTRLA.reg = AC_CTRLA_SWRST;
    while (AC->CTRLA.bit.SWRST || AC->STATUSB.bit.SYNCBUSY);
    AC->CTRLA.bit.ENABLE = 0;
    while (AC->STATUSB.bit.SYNCBUSY);

    // Configure the AC, using channel 0
    AC->CTRLA.bit.RUNSTDBY = 1;
    AC->COMPCTRL[0].reg = AC_COMPCTRL_FLEN_MAJ5     // 3/5 majority filter
                        | AC_COMPCTRL_HYST          // enable hysteresis
                        | AC_COMPCTRL_OUT_SYNC      // send synchronous (filtered) output to IO pin
                        | AC_COMPCTRL_MUXPOS_PIN3   // positive input select (mini board pin 9 PA07)
                        | AC_COMPCTRL_MUXNEG_PIN2   // negative input select (mini board pin 8 PA06)
                        | AC_COMPCTRL_INTSEL_TOGGLE // interrupt mode (when interrupt is enabled)
                        | AC_COMPCTRL_SPEED_HIGH    // faster (and use more power)
                     // | AC_COMPCTRL_SINGLE        // single-shot mode disabled for continuous operation
                        ;

    // enable interrupt for AC 0
    AC->INTENSET.reg = AC_INTENSET_COMP0;

    // enable the AC, both the AC itself and comparator 0
    AC->CTRLA.bit.ENABLE = 1;
    AC->COMPCTRL[0].bit.ENABLE = 1;
    // sync for clock domain, and because it takes 2 digital-clock cycles for the compator to be enabled
    while (AC->STATUSB.bit.SYNCBUSY || !(AC->COMPCTRL[0].bit.ENABLE));

    // set up pinmux
    pinPeripheral(8, PIO_ANALOG);   // pin 8 PA06 is negative input, select analog function
    pinPeripheral(9, PIO_ANALOG);   // pin 9 PA07 is positive input, select analog function
    //pinPeripheral(10, PIO_AC_CLK);  // pin 10 PA18 is comparator output IO pin, peripheral function H is AC/CMP[0]

    // Enable IRQ
    NVIC_ClearPendingIRQ(AC_IRQn);
    NVIC_EnableIRQ(AC_IRQn);
}

void AC_Handler()
{
    if (AC->INTFLAG.bit.COMP0)
    {
        blue_led = AC->STATUSA.bit.STATE0;
    }

    // clear all interrupt flags
    AC->INTFLAG.reg = AC_INTFLAG_MASK;
}

void setup()
{
    init_ac();
    blue_led = 0;
}

void loop()
{
    __WFI();
}
