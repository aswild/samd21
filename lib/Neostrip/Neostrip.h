/*******************************************************************************
 * NeoPixel library for Arduino SAMD21 using SPI and DMA
 *
 * Copyright (C) 2018-2019 Allen Wild <allenwild93@gmail.com>
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

#ifndef NEOSTRIP_H
#define NEOSTRIP_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include "Adafruit_ZeroDMA.h"
#include "Color.h"
#include "Print.h"
#include "SPI.h"

// define this before including bittable.h
#define NEOSTRIP_OUTPUT_INVERT 1
#include "neostrip_bittable.h"

#define NEOSTRIP_SPI_CLOCK 2400000

// DMA source address to wait for the latch
#if NEOSTRIP_OUTPUT_INVERT
static const uint8_t neostrip_idle_byte __attribute__((used)) = 0xFF;
#else
static const uint8_t neostrip_idle_byte __attribute__((used)) = 0x00;
#endif

// number of bytes needed to "transfer" low for the 50us latch
// bits/sec * sec * bytes/bit = bytes
// For a 2.4MHz clock, this is exactly 15. For other values, it might round down but
// that is OK because the WS2812 doesn't need a full 50us to reset.
static const size_t neostrip_dma_zero_n_bytes = (NEOSTRIP_SPI_CLOCK * 50) / (1000000 * 8);

/* CIE 1931 luminance correction table
 * similar, but more technically correct than a gamma correction table.
 * Based on the formulas
 *   Y = (L* / 903.3)           if L* ≤ 8
 *   Y = ((L* + 16) / 116)^3    if L* > 8
 * Where L* is the preceived brightness and Y is the linear luminance
 *
 * Generated with cie1931.py. See:
 * http://jared.geek.nz/2013/feb/linear-led-pwm
 * http://poynton.ca/PDFs/SMPTE93_Gamma.pdf
 */
static const uint8_t cie1931_table[256] = {
      0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,
      2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   4,
      4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,   7,
      7,   7,   7,   8,   8,   8,   8,   9,   9,   9,  10,  10,  10,  10,  11,  11,
     11,  12,  12,  12,  13,  13,  13,  14,  14,  15,  15,  15,  16,  16,  17,  17,
     17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  23,  24,  24,  25,
     25,  26,  26,  27,  28,  28,  29,  29,  30,  31,  31,  32,  32,  33,  34,  34,
     35,  36,  37,  37,  38,  39,  39,  40,  41,  42,  43,  43,  44,  45,  46,  47,
     47,  48,  49,  50,  51,  52,  53,  54,  54,  55,  56,  57,  58,  59,  60,  61,
     62,  63,  64,  65,  66,  67,  68,  70,  71,  72,  73,  74,  75,  76,  77,  79,
     80,  81,  82,  83,  85,  86,  87,  88,  90,  91,  92,  94,  95,  96,  98,  99,
    100, 102, 103, 105, 106, 108, 109, 110, 112, 113, 115, 116, 118, 120, 121, 123,
    124, 126, 128, 129, 131, 132, 134, 136, 138, 139, 141, 143, 145, 146, 148, 150,
    152, 154, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181,
    183, 185, 187, 189, 191, 193, 196, 198, 200, 202, 204, 207, 209, 211, 214, 216,
    218, 220, 223, 225, 228, 230, 232, 235, 237, 240, 242, 245, 247, 250, 252, 255,
};

static inline uint8_t neostrip_get_dma_sercom_trigger(SERCOM *_s)
{
    Sercom *s = _s->getSercom();
    if (s == SERCOM0)
        return SERCOM0_DMAC_ID_TX;
    if (s == SERCOM1)
        return SERCOM1_DMAC_ID_TX;
    if (s == SERCOM2)
        return SERCOM2_DMAC_ID_TX;
    if (s == SERCOM3)
        return SERCOM3_DMAC_ID_TX;
    if (s == SERCOM4)
        return SERCOM4_DMAC_ID_TX;
    if (s == SERCOM5)
        return SERCOM5_DMAC_ID_TX;
    return 0;
}

enum ColorOrder
{
    COLOR_ORDER_GRB,
    COLOR_ORDER_RGB,
};

template <size_t N, ColorOrder CO=COLOR_ORDER_GRB>
class Neostrip
{
    public:
        Neostrip(SPIClass& _spi, int _brightness=25) : spi(_spi), brightness(_brightness)
        {
            memset(colors, 0, sizeof(colors));
            memset(rawcolors, 0, sizeof(rawcolors));
        }

        void init(bool spi_init=true)
        {
            void *spi_data_reg = (void*)(&spi.getSERCOM()->getSercom()->SPI.DATA.reg);
            if (spi_init)
                spi.begin();

            dma.setTrigger(neostrip_get_dma_sercom_trigger(spi.getSERCOM()));
            dma.setAction(DMA_TRIGGER_ACTON_BEAT);
            dma.allocate();
            dma.loop(false);

            // main descriptor to send the data
            dma.addDescriptor(
                    (void*)(rawcolors),         // source address
                    spi_data_reg,               // dest address
                    (N*9), DMA_BEAT_SIZE_BYTE,  // data length and beat size
                    true, false);               // increment src addr, don't increment dest addr

            // 50us latch descriptor
            dma.addDescriptor(
                    (void*)(&neostrip_idle_byte), // source address
                    spi_data_reg,                 // dest address
                    neostrip_dma_zero_n_bytes,    // data length
                    DMA_BEAT_SIZE_BYTE,           // beat size
                    false, false);                // don't increment src or dest addresses

            dma.setCallback(dma_complete_callback, DMA_CALLBACK_TRANSFER_DONE, this);

            // allow the first transfer to start
            dma_complete = true;

            // begin the SPI transaction and never end it
            spi.beginTransaction(SPISettings(NEOSTRIP_SPI_CLOCK, MSBFIRST, SPI_MODE0));
        }

        void write(bool sync=false)
        {
            while (!dma_complete); // wait for previous transfer to finish
            dma_complete = false;

            expand_all_colors();
            dma.startJob();
            if (sync)
                while(!dma_complete);
        }

        void wait_for_complete(void) const
        {
            while (!dma_complete);
        }

        void set_color(size_t index, const Color& color)
        {
            if (index >= N)
                return;

            colors[index] = color;
        }

        void set_color(size_t index, uint32_t color_int)
        {
            set_color(index, Color(color_int));
        }

        void set_all_colors(const Color& color)
        {
            for (size_t i = 0; i < N; i++)
                colors[i] = color;
        }

        void clear(void)
        {
            memset(colors, 0, sizeof(colors));
        }

        Color get_color(size_t index) const
        {
            if (index >= N)
                return BLACK;
            return colors[index];
        }

        // brightness logic from Adafruit_NeoPixel_ZeroDMA.
        // 0-255 values are stored internally as 1-256
        void set_brightness(uint8_t b)
        {
            brightness = (uint16_t)b + 1;
        }
        uint8_t get_brightness(void) const
        {
            return brightness - 1;
        }

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

        // subscript operators enable treating Neostrip like an array of Colors
        const Color operator[](size_t index) const { return get_color(index); }
        Color& operator[](size_t index)
        {
            // CAUTION! No bounds check here! We always must return a non-const reference,
            // so there's no way to check bounds and do nothing like in set_color()
            // If you use this, be careful, and don't pass around the reference returned
            return colors[index];
        }

    private:
        SPIClass& spi;
        Adafruit_ZeroDMA dma;
        Color colors[N];
        uint8_t rawcolors[N * 9];
        volatile bool dma_complete;
        uint16_t brightness;

        static void dma_complete_callback(void *data)
        {
            Neostrip<N, CO> *ns = static_cast<Neostrip<N, CO>*>(data);
            ns->dma_complete = true;
        }

#ifndef NEOSTRIP_BITTABLE_H
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

#if NEOSTRIP_OUTPUT_INVERT
            // since the SPI hardware MOSI idles high, invert the bits in SW and drive the
            // NeoPixels' data pin through an inverter
            ret =  ~ret;
#endif
            return ret;
        }
#endif // !NEOSTRIP_BITTABLE_H

        // store the expanded representation of val into the first 3 bytes of dest
        static void expand_chunk(uint8_t *dest, uint8_t val)
        {
#ifdef NEOSTRIP_BITTABLE_H
            // from https://github.com/adafruit/Adafruit_NeoPixel_ZeroDMA/blob/master/Adafruit_NeoPixel_ZeroDMA.cpp
            uint32_t expanded = bitExpand[val];
            dest[0] = expanded >> 16;
            dest[1] = expanded >> 8;
            dest[2] = expanded;
#else
            uint32_t exp_low = expand_nibble(val, false);
            uint32_t exp_upp = expand_nibble(val, true);

            dest[0] = (exp_upp >> 4) & 0xff;
            dest[1] = ((exp_upp << 4) & 0xf0) | ((exp_low >> 8) & 0x0f);
            dest[2] = exp_low & 0xff;
#endif
        }

        inline uint8_t scale_brightness(uint8_t val) const
        {
            return ((uint16_t)val * brightness) >> 8;
        }

        void expand_all_colors(void)
        {
            for (size_t i = 0; i < N; i++)
            {
                const Color& c = colors[i];
                const size_t ri = i * 9;
                if (CO == COLOR_ORDER_GRB)
                {
                    expand_chunk(&rawcolors[ri+0], cie1931_table[scale_brightness(c.b.green)]);
                    expand_chunk(&rawcolors[ri+3], cie1931_table[scale_brightness(c.b.red)]);
                    expand_chunk(&rawcolors[ri+6], cie1931_table[scale_brightness(c.b.blue)]);
                }
                else /* if (CO == COLOR_ORDER_RGB) */
                {
                    expand_chunk(&rawcolors[ri+0], cie1931_table[scale_brightness(c.b.red)]);
                    expand_chunk(&rawcolors[ri+3], cie1931_table[scale_brightness(c.b.green)]);
                    expand_chunk(&rawcolors[ri+6], cie1931_table[scale_brightness(c.b.blue)]);
                }
            }
        }
};

#endif // NEOSTRIP_H
