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

class PWM
{
    public:
        // pin number must be mappable to one of the TCC0 WO[x] pins
        // freq is in Hz
        // dc is in percent in range [0, 100]. Use setDutyCycle for more precision
        PWM(uint32_t pin, uint32_t freq=1000, uint32_t dc=50);

        void init(void) const;
        void setFreq(uint32_t hz);
        void setPeriod(uint32_t ns);
        void setDutyCycle(uint32_t dc);
        void setDutyCycle(float dc); // set DC with float, 0.0 <= dc <= 1.0

        void start(void)
        {
            if (!started)
            {
                _start();
                started = true;
            }
        }

        void stop(void)
        {
            if (started)
            {
                _stop();
                started = false;
            }
        }

    private:
        uint32_t pin;
        uint64_t period_ns;
        uint64_t dc_ns;
        bool started;

        // start/stop "const" functions which do register-writes only
        // and don't update this->started
        void _start(void) const;
        void _stop(void) const;

        // write this object's values to the registers
        void write_period(void) const;
        void write_dc(void) const;
};

#endif //SAMD_PWM_H
