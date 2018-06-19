#ifndef NEOSTRIP_H
#define NEOSTRIP_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include "SPI.h"

#define NEOSTRIP_SPI_CLOCK 2400000

typedef union __attribute__((packed)) {
    struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t alpha;
    } b;
    uint32_t i;
} Color;

static const Color BLACK    = { .i = 0x000000 };
static const Color WHITE    = { .i = 0x333333 };
static const Color RED      = { .i = 0x330000 };
static const Color YELLOW   = { .i = 0x333300 };
static const Color GREEN    = { .i = 0x003300 };
static const Color CYAN     = { .i = 0x003333 };
static const Color BLUE     = { .i = 0x000033 };
static const Color MAGENTA  = { .i = 0x330033 };

template <size_t N>
class Neostrip
{
    public:
        Color colors[N];

        Neostrip(SPIClass& _spi) : spi(_spi), spi_settings(NEOSTRIP_SPI_CLOCK, MSBFIRST, SPI_MODE0)
        {
            memset(colors, 0, sizeof(colors));
            memset(rawcolors, 0, sizeof(rawcolors));
        }

        void init(void)
        {
            spi.begin();
        }

        void write(void)
        {
            spi.beginTransaction(spi_settings);
            //spi.transfer(static_cast<void*>(rawcolors), N*9);
            //for (uint8_t *b = &rawcolors[0]; b < &rawcolors[N*9]; b++)
                //spi.transfer(*b);
            for (size_t i = 0; i < N*9; i++)
                spi.transfer(rawcolors[i]);
            //for (size_t i = 0; i < 90; i++)
                //spi.transfer(0);
            spi.endTransaction();
        }

        void update_color(size_t index, const Color& color)
        {
            if (index < 0 || index >= N)
                return;

            colors[index] = color;
            expand_color(color, rawcolor(index));
        }

        void set_all_colors(const Color& color)
        {
            for (size_t i = 0; i < N; i++)
                update_color(i, color);
        }

        void clear(void) { set_all_colors(BLACK); };

#if 0
        void dump_rawcolors(Print& p)
        {
            for (size_t i = 0; i < N; i++)
            {
                for (size_t j = 0; j < 9; j++)
                {
                    p.printf("%02x ", rawcolors[i*9+j]);
                }
                p.printf("\n");
            }
        }
#endif

    private:
        SPIClass& spi;
        SPISettings spi_settings;
        uint8_t rawcolors[N * 9];

        // return the offset in the rawcolors buffer for the given pixel number
        inline uint8_t * rawcolor(size_t index)
        {
            return &rawcolors[index * 9];
        }

        // Expand a nibble of val (upper or lower) from 00000000abcd to 1a01b01c01d0
        // Magic multiplication and masking from
        //  https://developer.mbed.org/users/JacobBramley/code/PixelArray/file/47802e75974e/neopixel.cpp
        static inline uint32_t expand_nibble(uint8_t val, bool upper)
        {
            if (upper)
                val = (val >> 4);
            val = val & 0xf;

            uint32_t ret =  04444 |         // 100100100100
                   ((val * 0x88) & 0x410) | // 0a00000c0000
                   ((val * 0x22) & 0x82);   // 0000b00000d0

            // since the SPI hardware MOSI idles high, invert the bits in SW and drive the
            // NeoPixels' data pin through an inverter
            return ~ret;
        }

        // store the expanded representation of val into the first 3 bytes of dest
        static void expand_chunk(uint8_t *dest, uint8_t val)
        {
            uint32_t exp_low = expand_nibble(val, false);
            uint32_t exp_upp = expand_nibble(val, true);

            dest[0] = (exp_upp >> 4) & 0xff;
            dest[1] = ((exp_upp << 4) & 0xf0) | ((exp_low >> 8) & 0x0f);
            dest[2] = exp_low & 0xff;
        }

        // expand a 24-bit color to 9 bytes of ws2812 data
        static void expand_color(const Color& color, uint8_t *dest)
        {
            expand_chunk(&dest[0], color.b.green);
            expand_chunk(&dest[3], color.b.red);
            expand_chunk(&dest[6], color.b.blue);
        }
};

#endif // NEOSTRIP_H
