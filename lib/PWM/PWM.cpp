/*******************************************************************************
 * SAMD21 PWM Library using TCC0
 *
 * Copyright (C) 2019 Allen Wild <allenwild93@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <cmath>
#include "sam.h"
#include "PWM.h"
#include "variant.h"
#include "wiring_private.h"

/*******************************************************************************
 * HELPER FUNCTIONS
 ******************************************************************************/

template<typename T>
static inline T clamp(T val, T min, T max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

// wait for bits in SYNCBUSY register to clear
static inline void sync(uint32_t mask) { while(TCC0->SYNCBUSY.reg & mask); }

/*******************************************************************************
 * TIMING LIMITS AND PIN INFO
 ******************************************************************************/

// 24-bit counter, 1024x prescaler, 48MHz
// 17179869184 / 48MHz = 357.9139s = 357913941333ns
// note: this overflows 32 bits but fits in 64
static constexpr uint64_t MAX_PERIOD_NS =
    static_cast<uint64_t>((static_cast<double>((1ull << 24) * 1024ull) / F_CPU) * 1000000000.0);
static_assert((F_CPU != 48000000) || (MAX_PERIOD_NS == 357913941333ull), "unexpected value for MAX_PERIOD_NS");

// 1 / 48MHz = 20.8333 ns
static constexpr uint64_t MIN_PERIOD_NS = static_cast<uint64_t>(((1000000000ull - 1) / F_CPU) + 1);
static_assert((F_CPU != 48000000) || (MIN_PERIOD_NS == 21ull), "unexpected value for MIN_PERIOD_NS");

// pin info to map IO pins to the right IO multiplexing function and CC channel
struct TCC0_Pininfo_t
{
    uint8_t port;
    uint8_t pin;
    uint8_t type; // 4 == PIO_TIMER or 5 == PIO_TIMER_ALT
    uint8_t wox;  // WO[x] number
};

// all available pins for TCC0 outputs (datasheet table 7-1)
static const TCC0_Pininfo_t TCC0_Pintable[] = {
    { 0,  4, 4, 0 }, { 0,  5, 4, 1 }, { 0,  8, 4, 0 }, { 0,  9, 4, 1 },
    { 0, 10, 5, 2 }, { 0, 11, 5, 3 }, { 0, 12, 5, 6 }, { 0, 13, 5, 7 },
    { 0, 14, 5, 4 }, { 0, 15, 5, 5 }, { 0, 16, 5, 6 }, { 0, 17, 5, 7 },
    { 0, 18, 5, 2 }, { 0, 19, 5, 3 }, { 0, 20, 5, 6 }, { 0, 21, 5, 7 },
    { 0, 22, 5, 4 }, { 0, 23, 5, 5 }, { 1, 10, 5, 4 }, { 1, 11, 5, 5 },
#ifdef __SAMD21J18A__
    { 1, 12, 5, 6 }, { 1, 13, 5, 7 }, { 1, 16, 5, 4 }, { 1, 17, 5, 5 },
#endif
};

// search function for the table above, maps an arduino pin to the right entry
// or NULL if the pin isn't usable for TCC0 output
static const TCC0_Pininfo_t* find_pininfo(int pin)
{
    const uint8_t port = static_cast<uint8_t>(g_APinDescription[pin].ulPort);
    const uint8_t ppin = static_cast<uint8_t>(g_APinDescription[pin].ulPin);
    for (unsigned i = 0; i < (sizeof(TCC0_Pintable)/sizeof(TCC0_Pintable[0])); i++)
    {
        const TCC0_Pininfo_t *const info = &TCC0_Pintable[i];
        if (info->port == port && info->pin == ppin)
            return info;
    }
    return NULL;
}

struct TCCTiming
{
    uint16_t presc_div; // prescaler divider
    uint16_t presc_reg; // value for CTRLA.PRESCALER
    uint32_t count;     // value for PER or CCx

    TCCTiming(uint64_t ns)
    {
        static const struct {
            uint16_t scale;
            uint16_t reg;
        } presc_table[8] = {
            { 1 <<  0, TCC_CTRLA_PRESCALER_DIV1_Val },
            { 1 <<  1, TCC_CTRLA_PRESCALER_DIV2_Val },
            { 1 <<  2, TCC_CTRLA_PRESCALER_DIV4_Val },
            { 1 <<  3, TCC_CTRLA_PRESCALER_DIV8_Val },
            { 1 <<  4, TCC_CTRLA_PRESCALER_DIV16_Val },
            { 1 <<  6, TCC_CTRLA_PRESCALER_DIV64_Val },
            { 1 <<  8, TCC_CTRLA_PRESCALER_DIV256_Val },
            { 1 << 10, TCC_CTRLA_PRESCALER_DIV1024_Val },
        };

#if 0 // remove range checks since PWM class should have already clamped ns
        if (ns <= MAX_PERIOD_NS)
        {
            this->presc_div = 1;
            this->presc_reg = TCC_CTRLA_PRESCALER_DIV1_Val;
            this->count = 1;
        }
        else if (ns >= MAX_PERIOD_NS)
        {
            this->presc_div = 1024;
            this->presc_reg = TCC_CTRLA_PRESCALER_DIV1024_Val;
            this->count = 0x00FFFFFF;
        }
        else
#endif
        {
            uint8_t presc_id;
            uint64_t count64;
            for (presc_id = 0; presc_id < 8; presc_id++)
            {
                count64 = ((ns * F_CPU) / (1000000000ull * presc_table[presc_id].scale)) - 1;
                if (count64 < (1ul << 24))
                    break;
            }
            this->presc_div = presc_table[presc_id].scale;
            this->presc_reg = presc_table[presc_id].reg;
            this->count = static_cast<uint32_t>(count64);
        }
    }
};

void TCC0_Handler(void)
{
    // we don't use interrupts, but defining this function overwrites
    // the default infinite-loop Dummy_Handler and will cause a build error if other
    // code tries to use TCC0 and defines this handler.
    // Write to clear all defined flags in this register (datasheet section 31.8.12)
    TCC0->INTFLAG.reg = 0x000FFC0F;
}


/*******************************************************************************
 * PWM CLASS METHODS
 ******************************************************************************/

PWM::PWM(uint32_t freq)
    : period_ns(clamp(1000000000ull / freq, MIN_PERIOD_NS, MAX_PERIOD_NS)) {}

void PWM::init(void)
{
    // calculate prescaler and period ticks
    const TCCTiming timing(this->period_ns);
    this->presc_div = timing.presc_div;

    // enable GCLK for TCC0/TCC1
    GCLK->CLKCTRL.reg = static_cast<uint16_t>(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC0_TCC1);
    while (GCLK->STATUS.bit.SYNCBUSY);

    // disable and reset
    TCC0->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
    sync(TCC_SYNCBUSY_ENABLE);
    TCC0->CTRLA.reg = TC_CTRLA_SWRST;
    sync(TCC_SYNCBUSY_SWRST);
    while (TCC0->CTRLA.bit.SWRST);

    // setup CTRLA (none of these fields have a SYNCBUSY bit)
    TCC0->CTRLA.reg = TCC_CTRLA_PRESCSYNC(TCC_CTRLA_PRESCSYNC_RESYNC_Val) | // reset counter and prescaler on retrigger
                      TCC_CTRLA_PRESCALER(timing.presc_reg) | // set prescaler
                      TCC_CTRLA_ALOCK | // automatically lock double-buffer update (we do that manually)
                      0;

    // Normal PWM mode
    TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    sync(TCC_SYNCBUSY_WAVE);

    // set period
    TCC0->PERB.bit.PERB = timing.count;
    sync(TCC_SYNCBUSY_PERB);

    // don't-enable yet, start() will do that
}

void PWM::start(void) const
{
    if (TCC0->CTRLA.bit.ENABLE == 0)
    {
        TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
        sync(TCC_SYNCBUSY_ENABLE);
    }
    else
    {
        TCC0->CTRLBSET.reg = TCC_CTRLBSET_CMD_RETRIGGER;
        sync(TCC_SYNCBUSY_CTRLB);
    }
    update();
}

void PWM::stop(void) const
{
    TCC0->CTRLBSET.reg = TCC_CTRLBSET_CMD_STOP;
    sync(TCC_SYNCBUSY_CTRLB);
}

void PWM::update(void) const
{
    TCC0->CTRLBCLR.reg = TCC_CTRLBCLR_LUPD;
    sync(TCC_SYNCBUSY_CTRLB);
}

void PWM::set_freq(uint32_t hz, bool immediate)
{
    set_period_ns(1000000000ull / hz, immediate);
}

int PWM::enable_pin(int pin_num) const
{
    const TCC0_Pininfo_t* pi = find_pininfo(pin_num);
    if (pi == NULL)
        return -1;
    pinPeripheral(pin_num, static_cast<EPioType>(pi->type));
    return pi->wox;
}

void PWM::set_period_ns(uint64_t ns, bool immediate)
{
    uint64_t new_period_ns = clamp(ns, MIN_PERIOD_NS, MAX_PERIOD_NS);
    TCCTiming timing(new_period_ns);

    if (timing.presc_div != this->presc_div)
    {
        if (TCC0->CTRLA.bit.ENABLE)
        {
            // enabled and changing the prescaler means we have to disable/enable
            TCC0->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
            sync(TCC_SYNCBUSY_ENABLE);
            TCC0->CTRLA.bit.PRESCALER = timing.presc_reg;
            TCC0->PER.bit.PER = timing.count;
            sync(TCC_SYNCBUSY_PER);
            TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
            sync(TCC_SYNCBUSY_ENABLE);
        }
        else
        {
            // changing the prescaler, but we're already disabled
            TCC0->CTRLA.bit.PRESCALER = timing.presc_reg;
            TCC0->PER.bit.PER = timing.count;
            sync(TCC_SYNCBUSY_PER);
        }
    }
    else
    {
        if (TCC0->CTRLA.bit.ENABLE)
        {
            // enabled but only changing period, use double-buffered register
            TCC0->PERB.bit.PERB = timing.count;
            sync(TCC_SYNCBUSY_PERB);
            if (immediate)
                update();
        }
        else
        {
            // disabled, set PER directly
            TCC0->CTRLA.bit.PRESCALER = timing.presc_reg;
            TCC0->PER.bit.PER = timing.count;
            sync(TCC_SYNCBUSY_PER);
        }
    }
    this->period_ns = new_period_ns;
    this->presc_div = timing.presc_div;
}

void PWM::set_width_ns(int chan, uint64_t dc_ns, bool immediate) const
{
    dc_ns = clamp(dc_ns, 0ull, period_ns);
    uint32_t dc_count = static_cast<uint32_t>((dc_ns * F_CPU) / (1000000000ull * presc_div) - 0);

    if (chan < 0)
    {
        TCC0->CCB[0].bit.CCB = dc_count;
        TCC0->CCB[1].bit.CCB = dc_count;
        TCC0->CCB[2].bit.CCB = dc_count;
        TCC0->CCB[3].bit.CCB = dc_count;
        sync(TCC_SYNCBUSY_CCB0 | TCC_SYNCBUSY_CCB1 | TCC_SYNCBUSY_CCB2 | TCC_SYNCBUSY_CCB3);
    }
    else
    {
        chan = chan % 4;
        TCC0->CCB[chan].bit.CCB = dc_count;
        while (TCC0->SYNCBUSY.vec.CCB & (1<<chan));
    }

    if (immediate)
        update();
}

void PWM::set_chan(int chan, int percent, bool immediate) const
{
    const uint64_t width_ns = (period_ns * clamp(percent, 0, 100)) / 100ull;
    set_width_ns(chan, width_ns, immediate);
}

void PWM::set_chan(int chan, float dc, bool immediate) const
{
    const float width_ns_f = period_ns * clamp(dc, 0.0f, 1.0f);
    const uint64_t width_ns = static_cast<uint64_t>(std::llroundf(width_ns_f));
    set_width_ns(chan, width_ns, immediate);
}
