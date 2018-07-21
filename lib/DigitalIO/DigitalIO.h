/*******************************************************************************
 * DigitalIn/DigitalOut C++ wrappers for digitalRead and digitalWrite for Arduino,
 * based on the mbed library API.
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

#ifndef DIGITALIO_H
#define DIGITALIO_H

#include <cstdint>
#include "WInterrupts.h"

class DigitalOut
{
    public:
        DigitalOut(uint32_t pin);
        DigitalOut(uint32_t pin, int value);

        void write(int value);
        int read(void);

        DigitalOut& operator= (int value);
        DigitalOut& operator= (DigitalOut& rhs);
        operator int();

    protected:
        uint32_t _pin;
        int _value;
};

class DigitalIn
{
    public:
        DigitalIn(uint32_t pin);
        DigitalIn(uint32_t pin, uint32_t _mode);
        int read(void);
        void mode(uint32_t mode);
        void add_interrupt(voidFuncPtr isr, uint32_t mode);
        void remove_interrupt(void);
        void set_interrupt_filter(bool filter);
        operator int();

    protected:
        uint32_t _pin;

};

#endif
