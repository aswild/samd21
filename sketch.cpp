#include "Arduino.h"
#include "DigitalIO.h"

#include <stdlib.h> // for strtoul()

DigitalOut status_led(13);
DigitalIn button(9);

// 65536*1024 / 48MHz = 1.398101s = 1398101us
#define MAX_PULSE_US ((65536ul * 1024ul) / (F_CPU / 1000000ul))

static void timer_calculate_prescaler(uint32_t pulse_us, uint32_t *pprescaler, uint16_t *pcc)
{
    static const struct {
        uint32_t scale;
        uint32_t prescale;
    } prescaler_table[8] = {
        { 1 <<  0, TC_CTRLA_PRESCALER_DIV1 },
        { 1 <<  1, TC_CTRLA_PRESCALER_DIV2 },
        { 1 <<  2, TC_CTRLA_PRESCALER_DIV4 },
        { 1 <<  3, TC_CTRLA_PRESCALER_DIV8 },
        { 1 <<  4, TC_CTRLA_PRESCALER_DIV16 },
        { 1 <<  6, TC_CTRLA_PRESCALER_DIV64 },
        { 1 <<  8, TC_CTRLA_PRESCALER_DIV256 },
        { 1 << 10, TC_CTRLA_PRESCALER_DIV1024 },
    };

    if (pulse_us >= MAX_PULSE_US)
    {
        // max pulse width is (65536*1024)/48MHz = 1.398101sec
        // To avoid overflow, hard-cap the max settings here
        *pprescaler = TC_CTRLA_PRESCALER_DIV1024;
        *pcc = 0xffff;
        return;
    }

    uint32_t cc;
    unsigned int i;
    for (i = 0; i < 8; i++)
    {
        cc = (pulse_us * (F_CPU / 1000000)) / prescaler_table[i].scale - 1;
        if (cc < 65536)
            break;
    }

    *pprescaler = prescaler_table[i].prescale;
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

static void timer_set_timeout(uint32_t pulse_us)
{
    uint32_t prescaler;
    uint16_t cc;
    timer_calculate_prescaler(pulse_us, &prescaler, &cc);
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
    timer_sync();
    while (TC5->COUNT16.CTRLA.bit.SWRST);

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
    PORT->Group[0].OUTCLR.reg = 1 << 18; // fast clear pin 10 PA18

    // clear interrupt flag
    TC5->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF;
}

void button_isr(void)
{
    PORT->Group[0].OUTSET.reg = 1 << 18; // fast set pin 10 PA18
    timer_start();
}

void setup(void)
{
    status_led = 1;
    pinMode(10, OUTPUT);
    digitalWrite(10, 0);
    timer_init();

    button.mode(INPUT);
    button.add_interrupt(button_isr, FALLING);
    status_led = 0;

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
