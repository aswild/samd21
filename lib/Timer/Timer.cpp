/*******************************************************************************
 * SAMD21 Timer library
 *
 * Copyright (C) 2018 Allen Wild <allenwild93@gmail.com>
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

#include <sam.h>
#include "Timer.h"

#ifndef F_CPU
#include "variant.h"
#endif

// 65536*1024 / 48MHz = 1.398101s = 1398101us
#define MAX_TIMEOUT_US ((65536ul * 1024ul) / (F_CPU / 1000000ul))

int Timer::get_timer_info(uint32_t *clk_id, IRQn_Type *irqn)
{
    if (tc16 == &TC3->COUNT16)
    {
        *clk_id = GCLK_CLKCTRL_ID_TCC2_TC3_Val;
        *irqn = TC3_IRQn;
        return 0;
    }
    else if (tc16 == &TC4->COUNT16)
    {
        *clk_id = GCLK_CLKCTRL_ID_TC4_TC5_Val;
        *irqn = TC4_IRQn;
        return 0;
    }
    else if (tc16 == &TC5->COUNT16)
    {
        *clk_id = GCLK_CLKCTRL_ID_TC4_TC5_Val;
        *irqn = TC5_IRQn;
        return 0;
    }

    return 1;
}

void Timer::init(void)
{
    uint32_t clk_id;
    IRQn_Type irqn;

    if (get_timer_info(&clk_id, &irqn) != 0)
        return;

    // disable IRQ and set priority
    NVIC_DisableIRQ(irqn);
    NVIC_ClearPendingIRQ(irqn);
    NVIC_SetPriority(irqn, 0);

    // enable GCLK for TC4/TC5
    GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(clk_id));
    while (GCLK->STATUS.bit.SYNCBUSY);

    // disable and reset
    tc16->CTRLA.reg &= ~TC_CTRLA_ENABLE;
    sync();
    tc16->CTRLA.reg = TC_CTRLA_SWRST;
    sync();
    while (tc16->CTRLA.bit.SWRST);

    // setup CTRLA reg
    tc16->CTRLA.reg |=
        TC_CTRLA_MODE(TC_CTRLA_MODE_COUNT16_Val)    |     // 16-bit counter mode
        TC_CTRLA_WAVEGEN(TC_CTRLA_WAVEGEN_MFRQ_Val) |     // match frequency mode
        TC_CTRLA_PRESCALER(TC_CTRLA_PRESCALER_DIV1_Val);  // 1x prescaler
    sync();

    // default timeout = 1ms
    tc16->CC[0].reg = 48000-1;
    sync();

    // default oneshot mode on
    tc16->CTRLBSET.reg = TC_CTRLBSET_ONESHOT;

    // enable interrupt for overflow
    // in MFRQ one-shot, overflow is when the count hits TOP, i.e. CC0
    // If not using one-shot, we'd want the MC0 interrupt
    tc16->INTENSET.reg = TC_INTENSET_OVF;

    // enable
    tc16->CTRLA.reg |= TC_CTRLA_ENABLE;
    sync();

    // enable IRQ
    NVIC_EnableIRQ(irqn);
}

void Timer::set_us(uint32_t timeout_us)
{
    static const struct {
        uint32_t scale;
        uint32_t prescaler;
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

    uint32_t prescaler, cc;

    if (timeout_us >= MAX_TIMEOUT_US)
    {
        // max pulse width is (65536*1024)/48MHz = 1.398101sec
        // To avoid overflow, hard-cap the max settings here
        prescaler = TC_CTRLA_PRESCALER_DIV1024;
        cc = 0xffff;
    }
    else
    {
        unsigned int i;
        for (i = 0; i < 8; i++)
        {
            cc = (timeout_us * (F_CPU / 1000000)) / prescaler_table[i].scale - 1;
            if (cc < 65536)
                break;
        }
        prescaler = prescaler_table[i].prescaler;
    }

    // stop and disable because the prescaler in CTRLA is enable-protected
    stop();
    disable();

    tc16->CTRLA.reg = (tc16->CTRLA.reg & ~TC_CTRLA_PRESCALER_Msk) | prescaler;
    sync();
    tc16->CC[0].reg = (uint16_t)cc;
    sync();

    // re-enable
    enable();
}
