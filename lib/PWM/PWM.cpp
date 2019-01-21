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

#include "Arduino.h"
#include "PWM.h"

// 24-bit counter, 1024x prescaler, 48MHz
// 17179869184 / 48MHz = 357.9139s = 357913941333ns
// note: this overflows 32 bits but fits in 64
static constexpr uint64_t MAX_PERIOD_NS =
    static_cast<uint64_t>((static_cast<double>((1ull << 24) * 1024ull) / F_CPU) * 1000000000.0);

static_assert(MAX_PERIOD_NS == 357913941333ull, "unexpected value for MAX_PERIOD_NS");

struct TCC0_Pininfo_t;
{
    uint8_t port;
    uint8_t pin;
    uint8_t type; // 4 == PIO_TIMER or 5 == PIO_TIMER_ALT
    uint8_t wox;  // WO[x] number
};

// some more are available for SAMD21J, but not included here
static const TCC0_Pininfo_t TCC0_Pintable[] = {
    { 0,  4, 4, 0 },
    { 0,  5, 4, 1 },
    { 0,  8, 4, 0 },
    { 0,  9, 4, 1 },
    { 0, 10, 5, 2 },
    { 0, 11, 5, 3 },
    { 0, 12, 5, 6 },
    { 0, 13, 5, 7 },
    { 0, 14, 5, 4 },
    { 0, 15, 5, 5 },
    { 0, 16, 5, 6 },
    { 0, 17, 5, 7 },
    { 0, 18, 5, 2 },
    { 0, 19, 5, 3 },
    { 0, 20, 5, 6 },
    { 0, 21, 5, 7 },
    { 0, 22, 5, 4 },
    { 0, 23, 5, 5 },
    { 1, 10, 5, 4 },
    { 1, 11, 5, 5 },
};

static const TCC0_Pininfo_t* find_pininfo(uint32_t pin)
{
    uint8_t port = static_cast<uint8_t>(g_APinDescription[pin].ulPort);
    uint8_t ppin = static_cast<uint8_t>(g_APinDescription[pin].ulPin);
    for (unsigned i = 0; i < (sizeof(TCC0_Pintable)/sizeof(TCC0_Pintable[0])); i++)
    {
        const TCC0_Pininfo_t *const info = &TCC0_Pintable[i];
        if (info->port == port && info->pin == ppin)
            return info;
    }
    return NULL;
}

void TCC0_Handler(void)
{
    // we don't use interrupts, but defining this function overwrites
    // the default infinite-loop Dummy_Handler and will cause a build error if other
    // code tries to use TCC0 and defines this handler.
    // Write to clear all defined flags in this register (datasheet section 31.8.12)
    TCC0->INTFLAG.reg = 0x000FFC0F;
}

PWM::PWM(uint32_t pin, uint32_t freq, uint32_t dc)
    : pin(pin), started(false)
{
    period_ns = 1000000000ul / freq;
    if (period_ns > MAX_PERIOD_NS)
        period_ns = MAX_PERIOD_NS;

    if (dc <= 0)
        dc_ns = 0;
    else if (dc >= 100)
        dc_ns = period_ns;
    else
        dc_ns = (period_ns * 100) / dc;
}

void PWM::init(void) const
{
    // enable GCLK for TCC0/TCC1
    GCLK->CLKCTRL.reg = static_cast<uint16_t>(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC0_TCC1);
    while (GCLK->STATUS.bit.SYNCBUSY);

    // disable and reset
    TCC0->CTRLA.reg &= TCC_CTRLA_ENABLE;
    sync(TCC_SYNCBUSY_ENABLE);
    TCC0->CTRLA.reg = TC_CTRLA_SWRST;
    sync(TCC_SYNCBUSY_SWRST);
    while (TCC0->CTRLA.bit.SWRST);

    // setup CTRLA
    TCC0->CTRLA.reg |= TCC_CTRLA_PRESCSYNC(TCC_CTRLA_PRESCSYNC_RESYNC_Val); // reset counter and prescaler on retrigger
}
