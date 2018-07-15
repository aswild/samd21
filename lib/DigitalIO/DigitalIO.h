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
