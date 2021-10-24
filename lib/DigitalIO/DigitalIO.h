/*******************************************************************************
 * DigitalIn/DigitalOut C++ wrappers for digitalRead and digitalWrite for Arduino,
 * based on the mbed library API.
 *
 * Copyright (C) 2018-2021 Allen Wild <allenwild93@gmail.com>
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

#ifndef DIGITALIO_H
#define DIGITALIO_H

#include "Arduino.h"

class DigitalOut
{
    public:
        DigitalOut(pin_size_t pin)
            : DigitalOut(pin, LOW) {}

        DigitalOut(pin_size_t pin, int value)
            : DigitalOut(pin, static_cast<PinStatus>(value)) {}

        DigitalOut(pin_size_t pin, PinStatus value)
            : m_pin(pin)
        {
            pinMode(m_pin, OUTPUT);
            this->write(value);
        }

        inline void write(PinStatus value)
        {
            digitalWrite(m_pin, value);
        }

        inline void write(int value)
        {
            this->write(static_cast<PinStatus>(value));
        }

        inline void toggle()
        {
            this->write(!this->read());
        }

        inline PinStatus read() const
        {
            return digitalRead(m_pin);
        }

        inline DigitalOut& operator=(int value)
        {
            this->write(value);
            return *this;
        }

        inline DigitalOut& operator=(PinStatus value)
        {
            this->write(value);
            return *this;
        }

        inline operator bool()
        {
            return this->read() != LOW;
        }

    private:
        pin_size_t m_pin;
};

class DigitalIn
{
    public:
        DigitalIn(pin_size_t pin)
            : DigitalIn(pin, INPUT) {}

        DigitalIn(pin_size_t pin, PinMode mode)
            : m_pin(pin)
        {
            this->set_mode(mode);
        }

        inline PinStatus read()
        {
            return digitalRead(m_pin);
        }

        inline void set_mode(PinMode mode)
        {
            pinMode(m_pin, mode);
        }

        inline void add_interrupt(voidFuncPtr isr, PinStatus mode)
        {
            attachInterrupt(m_pin, isr, mode);
        }

        inline void remove_interrupt(void)
        {
            detachInterrupt(m_pin);
        }

        inline void set_interrupt_filter(bool filter)
        {
            setInterruptFilter(m_pin, filter);
        }

        inline operator int()
        {
            return static_cast<int>(digitalRead(m_pin));
        }

    private:
        pin_size_t m_pin;

};

#endif
