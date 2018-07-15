#include "DigitalIO.h"
#include "wiring_constants.h"
#include "wiring_digital.h"
#include "WInterrupts.h"

/********************* DigitalOut *************************/
DigitalOut::DigitalOut(uint32_t pin)
{
    _pin = pin;
    pinMode(_pin, OUTPUT);
    write(0);
}

DigitalOut::DigitalOut(uint32_t pin, int value)
{
    _pin = pin;
    pinMode(_pin, OUTPUT);
    write(value);
}

void DigitalOut::write(int value)
{
    _value = value;
    digitalWrite(_pin, _value);
}

int DigitalOut::read(void)
{
    return _value;
}

DigitalOut& DigitalOut::operator= (int value)
{
    write(value);
    return *this;
}

DigitalOut& DigitalOut::operator= (DigitalOut& rhs)
{
    write(rhs.read());
    return *this;
}

DigitalOut::operator int()
{
    return _value;
}

/********************* DigitalIn *************************/
DigitalIn::DigitalIn(uint32_t pin)
{
    _pin = pin;
    mode(INPUT);
}

DigitalIn::DigitalIn(uint32_t pin, uint32_t _mode)
{
    _pin = pin;
    mode(_mode);
}

int DigitalIn::read(void)
{
    return digitalRead(_pin);
}

void DigitalIn::mode(uint32_t _mode)
{
    pinMode(_pin, _mode);
}

void DigitalIn::add_interrupt(voidFuncPtr isr, uint32_t mode)
{
    attachInterrupt(_pin, isr, mode);
}

void DigitalIn::remove_interrupt(void)
{
    detachInterrupt(_pin);
}

void DigitalIn::set_interrupt_filter(bool filter)
{
    setInterruptFilter(_pin, filter);
}

DigitalIn::operator int()
{
    return digitalRead(_pin);
}
