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

#ifndef SAMD_PWM_H
#define SAMD_PWM_H

#include <stdint.h>

class PWM
{
    public:
        // freq is in Hz
        PWM(uint32_t freq=1000);

        // initialize hardware
        void init(void);

        // start/stop - the usual suspects
        void start(void) const;
        void stop(void) const;

        // clear "lock update" bit and apply new duty cycles on the next HW UPDATE
        // (doesn't bypass double buffering)
        void update(void) const;

        // run pinPeripheral for the given pin, if it can be used with TCC0.
        // Returns the WOx channel number (0-7), or -1 if the pin was invalid.
        // To disable a pin, just call pinMode for that pin to mux it back to GPIO.
        int enable_pin(int pin_num) const;

        // functions to set the frequency/period.
        // double buffered registers until update() is called
        // Note: the prescaler is enable-protected, so a large period change will
        // cause an enable/disable cycle and interrupt the output.
        // immediate controls whether to immediately apply the changes, or leave them in the
        //   double-buffered registers until a HW UPDATE event. Has no effect unless TCC is
        //   already enabled and the prescaler doesn't change.
        void set_freq(uint32_t hz, bool immediate=true);
        void set_period_ns(uint64_t ns, bool immediate=true);
        inline void set_period_us(uint64_t us, bool immediate=true)
            { set_period_ns(us * 1000ull, immediate); }
        inline void set_period_ms(uint64_t ms, bool immediate=true)
            { set_period_ns(ms * 1000000ull, immediate); }

        // functions to set the pulse width (duty cycle)
        // if chan is -1, set all 4 channels, otherwise set chan%4
        // immediate determines whether to disable the Lock Update bit,
        // otherwise a manual call to update() is needed.
        void set_width_ns(int chan, uint64_t dc_ns, bool immediate=true) const;
        inline void set_width_us(int chan, uint32_t us, bool imm=true) const
            { set_width_ns(chan, us * 1000ull, imm); }
        inline void set_width_ms(int chan, uint32_t ms, bool imm=true) const
            { set_width_ns(chan, ms * 1000000ull, imm); }

        // set width as a integer percent, range [1, 100]
        void set_chan(int chan, int percent, bool immediate=true) const;

        // same as above but with float duty cycle [0.0, 1.0]
        void set_chan(int chan, float dc, bool immediate=true) const;
        inline void set_chan(int chan, double dc, bool immediate=true) const
            { set_chan(chan, static_cast<float>(dc), immediate); }

    private:
        uint64_t period_ns;
        uint32_t presc_div;
};

#endif //SAMD_PWM_H
