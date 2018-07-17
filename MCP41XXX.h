#ifndef MCP41XXX_H
#define MCP41XXX_H

#include "SPI.h"
#include "wiring_digital.h"

#define MCP41XXX_DEFAULT_CLOCK 4000000
#define MCP41XXX_CMD_SETWIPER 0x1100
#define MCP41XXX_CMD_SHUTDOWN 0x2100

class MCP41XXX
{
    public:
        MCP41XXX(SPIClass& _spi, uint32_t _cs_pin, uint32_t clock=MCP41XXX_DEFAULT_CLOCK) :
            spi(_spi), spi_settings(clock, MSBFIRST, SPI_MODE0), cs_pin(_cs_pin) { }

        void init(void)
        {
            pinMode(cs_pin, OUTPUT);
            digitalWrite(cs_pin, 1);
            spi.begin();
            shutdown();
        }

        void set_wiper(uint8_t pos) { write(MCP41XXX_CMD_SETWIPER | pos); }
        void shutdown(void) { write(MCP41XXX_CMD_SHUTDOWN); }

    private:
        SPIClass& spi;
        SPISettings spi_settings;
        uint32_t cs_pin;

        void write(uint16_t data)
        {
            union { uint16_t val; struct { uint8_t lsb; uint8_t msb; }; } t;
            t.val = data;
            spi.beginTransaction(spi_settings);
            digitalWrite(cs_pin, 0);
            spi.transfer(t.msb);
            spi.transfer(t.lsb);
            digitalWrite(cs_pin, 1);
            spi.endTransaction();
        }
};

#endif // MCP41XXX_H
