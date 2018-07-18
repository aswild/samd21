#include "Arduino.h"
#include "DigitalIO.h"

#include <stdlib.h> // for strtoul()

DigitalOut status_led(13);
DigitalOut led(10);
DigitalIn button(9);

// 65536*1024 / 48MHz = 1.398101s = 1398101us
#define MAX_PULSE_US ((65536ul * 1024ul) / (F_CPU / 1000000ul))

static void get_prescaler_cc(uint32_t pulse_us, uint32_t *pprescaler, uint16_t *pcc)
{
    // adapted from Tone.cpp
    uint32_t prescaler;
    uint32_t cc = 0xffff+1;
    int i = -1;

    if (pulse_us >= MAX_PULSE_US)
    {
        // max pulse width is (65536*1024)/48MHz = 1.398101sec
        // To avoid overflow, hard-cap the max settings here
        *pprescaler = TC_CTRLA_PRESCALER_DIV1024;
        *pcc = 0xffff;
        return;
    }

    while (cc > 0xfffful)
    {
        i++;
        if (i == 5 || i == 7 || i == 9) // no prescaler for 32/128/512
            i++;
        cc = (pulse_us * (F_CPU / 1000000)) / (1<<i) - 1;
        SerialUSB.printf("get_prescaler: i=%d, cc=%lu\r\n", i, cc);
    }

    switch (i)
    {
        case  0: prescaler = TC_CTRLA_PRESCALER_DIV1;    break;
        case  1: prescaler = TC_CTRLA_PRESCALER_DIV2;    break;
        case  2: prescaler = TC_CTRLA_PRESCALER_DIV4;    break;
        case  3: prescaler = TC_CTRLA_PRESCALER_DIV8;    break;
        case  4: prescaler = TC_CTRLA_PRESCALER_DIV16;   break;
        case  6: prescaler = TC_CTRLA_PRESCALER_DIV64;   break;
        case  8: prescaler = TC_CTRLA_PRESCALER_DIV256;  break;
        case 10: prescaler = TC_CTRLA_PRESCALER_DIV1024; break;
        default: break;
    }

    *pprescaler = prescaler;
    *pcc = static_cast<uint16_t>(cc);
}

static inline void timer_sync(void)
{
    while (TC5->COUNT16.STATUS.bit.SYNCBUSY);
}

static inline void timer_stop(void)
{
    TC5->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP;
    TC5->COUNT16.COUNT.reg = 0;
}

static inline void timer_start(void)
{
    // stop, reset, and retrigger
    timer_stop();
    TC5->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
}

static inline void timer_set_timeout(uint32_t pulse_us)
{
    uint32_t prescaler;
    uint16_t cc;
    get_prescaler_cc(pulse_us, &prescaler, &cc);
    if (SerialUSB)
        SerialUSB.printf("set timeout %luus: prescaler=%lu, CC=%u\r\n",
                pulse_us, prescaler >> TC_CTRLA_PRESCALER_Pos, cc);

    timer_stop();
    TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE; // prescaler is enable-protected
    timer_sync();

    TC5->COUNT16.CTRLA.reg = (TC5->COUNT16.CTRLA.reg & ~TC_CTRLA_PRESCALER_Msk) | prescaler;
    timer_sync();
    TC5->COUNT16.CC[0].reg = cc;
    timer_sync();

    // re-enable
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    timer_sync();
}

static void timer_init(void)
{
    // disable IRQ and set priority
    NVIC_DisableIRQ(TC5_IRQn);
    NVIC_ClearPendingIRQ(TC5_IRQn);
    NVIC_SetPriority(TC5_IRQn, 0);

    // enable GCLK for TC4/TC5
    GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_TC4_TC5_Val));
    while (GCLK->STATUS.bit.SYNCBUSY);

    // disable and reset
    TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    timer_sync();
    TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC5->COUNT16.CTRLA.bit.SWRST);
    timer_sync();
    status_led = 0;

    // setup CTRLA reg
    TC5->COUNT16.CTRLA.reg |=
        TC_CTRLA_MODE(TC_CTRLA_MODE_COUNT16_Val)    |     // 16-bit counter mode
        TC_CTRLA_WAVEGEN(TC_CTRLA_WAVEGEN_MFRQ_Val) |     // match frequency mode
        TC_CTRLA_PRESCALER(TC_CTRLA_PRESCALER_DIV1_Val);  // 1x prescaler
    timer_sync();

    // default timeout = 1ms
    TC5->COUNT16.CC[0].reg = 48000-1;
    timer_sync();

    // default oneshot mode on
    TC5->COUNT16.CTRLBSET.reg = TC_CTRLBSET_ONESHOT;

    // enable interrupt for overflow
    // in MFRQ one-shot, overflow is when the count hits TOP, i.e. CC0
    // If not using one-shot, we'd want the MC0 interrupt
    TC5->COUNT16.INTENSET.reg = TC_INTENSET_OVF;

    // enable
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    timer_sync();

    // enable IRQ
    NVIC_EnableIRQ(TC5_IRQn);
}

void TC5_Handler(void)
{
    //led = 0;
    PORT->Group[0].OUTCLR.reg = 1 << 18; // fast clear pin 10 PA18

    // clear interrupt flag
    TC5->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF | TC_INTFLAG_MC0;
}

void button_isr(void)
{
    //led = 1;
    PORT->Group[0].OUTSET.reg = 1 << 18; // fast set pin 10 PA18
    timer_start();
}

void setup(void)
{
    status_led = 1;
    timer_init();
    status_led = 0;
    timer_set_timeout(50);

    button.mode(INPUT);
    button.add_interrupt(button_isr, FALLING);

    SerialUSB.begin(115200);
    while (!SerialUSB); // wait for USB host to open the port
    SerialUSB.print("SAMD21 TC Timer Test\r\n");
}

void loop(void)
{
    SerialUSB.write("> ");
    char cmdbuf[16];
    size_t cmdlen = SerialUSB.readLine(cmdbuf, sizeof(cmdbuf), true);
    if (cmdlen)
    {
        //SerialUSB.printf("count=%u buf='%s'\r\n", cmdlen, cmdbuf);
        uint32_t val = strtoul(cmdbuf, NULL, 0);
        SerialUSB.printf("value=%lu\r\n", val);
        if (val)
        {
            timer_set_timeout(val);
            button_isr();
        }
    }
}
