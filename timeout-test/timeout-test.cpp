#include "Arduino.h"
#include "DigitalIO.h"
#include "Timeout.h"

#include <stdlib.h> // for strtoul()

DigitalOut status_led(13);
DigitalIn button(9);

static void timeout_isr(void)
{
    PORT->Group[0].OUTCLR.reg = 1 << 18; // fast clear pin 10 PA18
}

static void button_isr(void)
{
    PORT->Group[0].OUTSET.reg = 1 << 18; // fast set pin 10 PA18
    timeout_start();
}

void setup(void)
{
    status_led = 1;
    pinMode(10, OUTPUT);
    digitalWrite(10, 0);
    timeout_init();
    timeout_set_callback(&timeout_isr);

    button.mode(INPUT);
    button.add_interrupt(button_isr, FALLING);
    status_led = 0;

    SerialUSB.begin(115200);
    while (!SerialUSB); // wait for USB host to open the port
    SerialUSB.print("SAMD21 TC Timer Test\r\n");
    button_isr();
}

void loop(void)
{
    SerialUSB.write("> ");
    char cmdbuf[16];
    size_t cmdlen = SerialUSB.readLine(cmdbuf, sizeof(cmdbuf), true);
    if (cmdlen)
    {
        //SerialUSB.printf("count=%u buf='%s'\r\n", cmdlen, cmdbuf);
        uint32_t val = strtoul(cmdbuf, NULL, 0);
        if (val)
        {
            timeout_set_us(val);
            uint16_t cc = TC5->COUNT16.CC[0].reg;
            uint32_t prescaler = TC5->COUNT16.CTRLA.bit.PRESCALER;
            SerialUSB.printf("%luus, prescaler=%lu, count=%u\r\n",
                             val, prescaler, cc);
            button_isr();
        }
    }
}
