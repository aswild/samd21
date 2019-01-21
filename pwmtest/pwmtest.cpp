#include "Arduino.h"
#include "wiring_private.h"

#define DEBUG_PORT 0
#define DEBUG_PIN  17
#include "debug_macros.h"

// 24-bit counter, 1024x prescaler, 48MHz
// 17179869184 / 48MHz = 357.9139s = 357913941333ns
// note: this overflows 32 bits but fits in 64
static constexpr uint64_t MAX_PERIOD_NS =
    static_cast<uint64_t>((static_cast<double>((1ull << 24) * 1024ull) / F_CPU) * 1000000000.0);

static_assert(MAX_PERIOD_NS == 357913941333ull, "unexpected value for MAX_PERIOD_NS");

struct TCCTiming
{
    uint32_t prescaler;
    uint32_t period;
    uint32_t cc;
};

static void CalcTiming(TCCTiming& t, uint64_t period_ns, uint64_t dc_ns)
{
    static const struct {
        uint32_t scale;
        uint32_t prescaler;
    } prescaler_table[8] = {
        { 1 <<  0, TCC_CTRLA_PRESCALER_DIV1 },
        { 1 <<  1, TCC_CTRLA_PRESCALER_DIV2 },
        { 1 <<  2, TCC_CTRLA_PRESCALER_DIV4 },
        { 1 <<  3, TCC_CTRLA_PRESCALER_DIV8 },
        { 1 <<  4, TCC_CTRLA_PRESCALER_DIV16 },
        { 1 <<  6, TCC_CTRLA_PRESCALER_DIV64 },
        { 1 <<  8, TCC_CTRLA_PRESCALER_DIV256 },
        { 1 << 10, TCC_CTRLA_PRESCALER_DIV1024 },
    };

    uint8_t prescaler_id;
    uint64_t period;

    if (period_ns >= MAX_PERIOD_NS)
    {
        prescaler_id = 7;
        period = 0x00FFFFFF;
        period_ns = MAX_PERIOD_NS;
    }
    else
    {
        for (prescaler_id = 0; prescaler_id < 8; prescaler_id++)
        {
            period = ((period_ns * F_CPU) / (1000000000ull * prescaler_table[prescaler_id].scale)) - 1;
            if (period < (1ul << 24))
                break;
        }
    }

    uint64_t cc;
    if (dc_ns >= period_ns)
        cc = period;
    else
        cc = ((dc_ns * F_CPU) / (1000000000ull * prescaler_table[prescaler_id].scale) - 1);

    t.prescaler = prescaler_table[prescaler_id].prescaler;
    t.period = period;
    t.cc = cc;
}

void setup(void)
{
    constexpr uint64_t period_ns = 100000;
    constexpr uint64_t dc_ns = static_cast<uint64_t>(period_ns * 0.5);
    TCCTiming timing;
    CalcTiming(timing, period_ns, dc_ns);

    // enable GCLK for TCC0/TCC1
    GCLK->CLKCTRL.reg = static_cast<uint16_t>(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC0_TCC1);
    while (GCLK->STATUS.bit.SYNCBUSY);

    // disable and reset
    TCC0->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.bit.ENABLE);
    TCC0->CTRLA.reg = TC_CTRLA_SWRST;
    while (TCC0->SYNCBUSY.bit.SWRST);
    while (TCC0->CTRLA.bit.SWRST);

    // setup CTRLA
    TCC0->CTRLA.reg |=
        TCC_CTRLA_PRESCSYNC(TCC_CTRLA_PRESCSYNC_RESYNC_Val) | // reset counter and prescaler on retrigger
        TCC_CTRLA_PRESCALER(timing.prescaler);

    TCC0->WAVEB.bit.WAVEGENB = TCC_WAVEB_WAVEGENB_NPWM_Val; // normal PWM mode
    while (TCC0->SYNCBUSY.bit.WAVEB);

    TCC0->PERB.bit.PERB = timing.period;
    while (TCC0->SYNCBUSY.bit.PERB);

#if 0
    for (unsigned i = 0; i < 4; i++)
    {
        TCC0->CCB[i].bit.CCB = timing.cc;
        while (TCC0->SYNCBUSY.vec.CCB & (1<<i));
    }
#endif
    TCC0->CCB[1].bit.CCB = timing.cc;
    while (TCC0->SYNCBUSY.vec.CCB & (1<<1));
    TCC0->CCB[2].bit.CCB = 3000;
    while (TCC0->SYNCBUSY.vec.CCB & (1<<2));

    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    TCC0->CTRLBSET.reg = TCC_CTRLBSET_CMD_RETRIGGER;

    DBGINIT();
    pinPeripheral(5, PIO_TIMER_ALT); // PA15, TCC0.WO[5]
    pinPeripheral(6, PIO_TIMER_ALT); // PA20, TCC0.WO[6]
}

void loop(void)
{
    DBGHIGH();
    delay(500);
    DBGLOW();
    delay(500);
}
