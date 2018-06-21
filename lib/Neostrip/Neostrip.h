#ifndef NEOSTRIP_H
#define NEOSTRIP_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include "SPI.h"
#include "Adafruit_ZeroDMA.h"

// define this before including bittable.h
#define NEOSTRIP_OUTPUT_INVERT 1
#include "neostrip_bittable.h"

#define NEOSTRIP_SPI_CLOCK 2400000

// DMA source address to wait for the latch
#if NEOSTRIP_OUTPUT_INVERT
static const uint8_t dma_zero_byte __attribute__((used)) = 0xFF;
#else
static const uint8_t dma_zero_byte __attribute__((used)) = 0x00;
#endif

// number of bytes needed to "transfer" low for the 50us latch
// bits/sec * sec * bytes/bit = bytes
// For a 2.4MHz clock, this is exactly 15. For other values, it might round down but
// that is OK because the WS2812 doesn't need a full 50us to reset.
static const size_t dma_zero_n_bytes = (NEOSTRIP_SPI_CLOCK * 50) / (1000000 * 8);

typedef union __attribute__((packed)) {
    struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
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

static inline uint8_t get_trigger(SERCOM *_s)
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

template <size_t N>
class Neostrip
{
    public:
        Neostrip(SPIClass& _spi) : spi(_spi)
        {
            memset(colors, 0, sizeof(colors));
            memset(rawcolors, 0, sizeof(rawcolors));
        }

        void init(void)
        {
            void *spi_data_reg = (void*)(&spi.getSERCOM()->getSercom()->SPI.DATA.reg);
            spi.begin();

            dma.setTrigger(get_trigger(spi.getSERCOM()));
            dma.setAction(DMA_TRIGGER_ACTON_BEAT);
            dma.allocate();
            dma.loop(false);

            // main descriptor to send the data
            dma.addDescriptor(
                    (void*)(rawcolors),         // source address
                    spi_data_reg,               // dest address
                    (N*9), DMA_BEAT_SIZE_BYTE,  // data length and beat size
                    true, false);               // increment src addr, don't increment dest addr

            dma.addDescriptor(
                    (void*)(&dma_zero_byte),    // source address
                    spi_data_reg,               // dest address
                    dma_zero_n_bytes,           // data length
                    DMA_BEAT_SIZE_BYTE,         // beat size
                    false, false);              // don't increment src or dest addresses

            dma.setCallback(dma_complete_callback, DMA_CALLBACK_TRANSFER_DONE, this);

            // allow the first transfer to start
            dma_complete = true;

            // begin the SPI transaction and never end it
            spi.beginTransaction(SPISettings(NEOSTRIP_SPI_CLOCK, MSBFIRST, SPI_MODE0));
        }

        void write(bool sync=true)
        {
            while (!dma_complete); // wait for previous transfer to finish
            dma_complete = false;
            dma.startJob();
            if (sync)
                while(!dma_complete);
        }

        void wait_for_complete(void)
        {
            while (!dma_complete);
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
        Adafruit_ZeroDMA dma;
        Color colors[N];
        uint8_t rawcolors[N * 9];
        volatile bool dma_complete;

        static void dma_complete_callback(void *data)
        {
            Neostrip<N> *ns = static_cast<Neostrip<N>*>(data);
            ns->dma_complete = true;
        }

        // return the offset in the rawcolors buffer for the given pixel number
        inline uint8_t * rawcolor(size_t index)
        {
            return &rawcolors[index * 9];
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

        // expand a 24-bit color to 9 bytes of ws2812 data
        static void expand_color(const Color& color, uint8_t *dest)
        {
            expand_chunk(&dest[0], color.b.green);
            expand_chunk(&dest[3], color.b.red);
            expand_chunk(&dest[6], color.b.blue);
        }
};

#endif // NEOSTRIP_H
