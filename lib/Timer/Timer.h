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

#ifndef TIMER_H
#define TIMER_H

#include <sam.h>
#include <stddef.h> // for NULL

// this macro must be called to hook up a class instance to the right TC IRQ
#define DECLARE_TIMER_HANDLER(_tc, _timer)          \
    void _tc##_Handler(void) {                      \
        void(*cb)(void) = _timer.get_callback();    \
        if (cb != NULL) cb();                       \
        _tc->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF;  \
    }

class Timer
{
    public:
        Timer(Tc *tc, void(*callback)(void)=NULL) : tc16(&tc->COUNT16), _callback(callback) { }
        inline void enable(void)  { tc16->CTRLA.reg |=  TC_CTRLA_ENABLE; sync(); }
        inline void disable(void) { tc16->CTRLA.reg &= ~TC_CTRLA_ENABLE; sync(); }
        inline void set_callback(void(*callback)(void)) { _callback = callback; }
        inline void(*get_callback(void))(void) { return _callback; }

        inline void start(void)
        {
            stop();
            tc16->CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
        }
        inline void stop(void)
        {
            tc16->CTRLBSET.reg = TC_CTRLBSET_CMD_STOP;
            tc16->COUNT.reg = 0;
        }

        void init(void);
        void set_us(uint32_t timeout_us);

    private:
        TcCount16 *tc16;
        void(*_callback)(void);

        inline void sync(void) { while (tc16->STATUS.bit.SYNCBUSY); }
        int get_timer_info(uint32_t *gclk_clkctrl_id, IRQn_Type *irqn);
};

#endif // TIMER_H
