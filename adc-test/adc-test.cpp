#include "Arduino.h"
#include "wiring_private.h"

#define ADC_PIN A2

#define ASCALE (3.3 / 4096)

#define ADC_SYNC() do { } while (ADC->STATUS.bit.SYNCBUSY)
#define ADC_GETR(_reg) (ADC->_reg.reg)
#define ADC_SETR(_reg, _val) do { ADC->_reg.reg = (_val); ADC_SYNC(); } while(0)
#define ADC_SETB(_reg, _val) do { ADC->_reg.reg |= (_val); ADC_SYNC(); } while(0)
#define ADC_CLRB(_reg, _val) do { ADC->_reg.reg &= ~(_val); ADC_SYNC(); } while(0)

static void adc_init(void)
{
    // SystemInit() handles setting the ADC calibration values from NVM
    // init() (startup.c) configures the ADC with
    //   * appropriate GCLK settings
    //   * 512X clock prescaler
    //   * 10-bit resolution
    //   * max sampling time (SAMPCTRL = 0x3f)
    //   * internal GND as negative input
    //   * 1 sample and no adjust (AVGCTRL)
    //   * calls analogReference(AR_DEFAULT)
    //     - INTVCC1 (1/2 VDDANA = 1.65V), GAIN=DIV2

    // disable ADC
    ADC_CLRB(CTRLA, ADC_CTRLA_ENABLE);

    // set 12-bit precision
    ADC_SETR(CTRLB, (ADC_GETR(CTRLB) & ~ADC_CTRLB_RESSEL_Msk) | ADC_CTRLB_RESSEL_12BIT);

    // set mux position (pin input)
    ADC_SETR(INPUTCTRL, (ADC_GETR(INPUTCTRL) & ~ADC_INPUTCTRL_MUXPOS_Msk) |
                        g_APinDescription[ADC_PIN].ulADCChannelNumber);

    // enable ADC
    ADC_SETB(CTRLA, ADC_CTRLA_ENABLE);

    // perform and discard one conversion
    ADC_SETB(SWTRIG, ADC_SWTRIG_START);
    while (ADC->INTFLAG.bit.RESRDY == 0);
    ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;
    ADC_SYNC();
}

static uint32_t adc_read(void)
{
    ADC_SETB(SWTRIG, ADC_SWTRIG_START);
    PORT->Group[0].OUTSET.reg = 1<<2; // fast-set pin 14 PA02
    while (ADC->INTFLAG.bit.RESRDY == 0);
    PORT->Group[0].OUTCLR.reg = 1<<2; // fast-clear pin 14 PA02
    return ADC->RESULT.reg;
}

void setup(void)
{
    pinMode(14, OUTPUT);
    adc_init();

    Serial1.begin(115200);
    Serial1.print("SAMD21 ADC\r\n");

    SerialUSB.begin(115200);
    //while (!SerialUSB); // wait for USB host to open the port
    SerialUSB.print("SAMD21 ADC Test\r\n");
}

void loop(void)
{
    uint32_t value = adc_read();
    double scaled = value * ASCALE;

    Serial1.printf("%04lx = ", value);
    Serial1.print(scaled, 3);
    Serial1.print("\n");

    SerialUSB.printf("%04lx = ", value);
    SerialUSB.print(scaled, 3);
    SerialUSB.print("\n");
    //SerialUSB.printf("%04lx = %f\n", value, scaled);

    delay(1000);
}
