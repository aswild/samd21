/*
Copyright (c) 2011 Anthony Buckton (abuckton [at] blackink [dot} net {dot} au)


Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

   Parts written by Jim Lindblom of Sparkfun
   Ported to mbed by A.Buckton, Feb 2011

   Ported to Arduino SAMD21 by Allen Wild, Jun 2018
*/

#ifndef MPR121_H
#define MPR121_H

#include "Wire.h"

class MPR121
{

public:
    // i2c Addresses, bit-shifted
    static const uint8_t ADD_VSS = 0x5a; // wiring on Sparkfun board
    static const uint8_t ADD_VDD = 0x5b;
    static const uint8_t ADD_SCL = 0x5c;
    static const uint8_t ADD_SDA = 0x5d;

    // Real initialiser, takes the i2c address of the device.
    MPR121(TwoWire& _i2c, uint8_t address=ADD_VSS);

    void init(void);

    bool getProximityMode(void);

    void setProximityMode(bool mode);

    uint8_t readTouchData(void);

    uint8_t read(uint8_t reg);

    size_t write(uint8_t reg, uint8_t value);
    size_t writeMany(uint8_t start, const uint8_t *dataSet, size_t length);

    void setElectrodeThreshold(int electrodeId, uint8_t touchThreshold, uint8_t releaseThreshold);

protected:
    // Configures the MPR with standard settings. This is permitted to be overwritten by sub-classes.
    void configureSettings(void);

private:
    // The I2C bus instance.
    TwoWire i2c;

    // 7 bit i2c address of this mpr121
    uint8_t address;

public:
    // MPR121 Register Defines
    static const uint8_t MHD_R        = 0x2B;
    static const uint8_t NHD_R        = 0x2C;
    static const uint8_t NCL_R        = 0x2D;
    static const uint8_t FDL_R        = 0x2E;
    static const uint8_t MHD_F        = 0x2F;
    static const uint8_t NHD_F        = 0x30;
    static const uint8_t NCL_F        = 0x31;
    static const uint8_t FDL_F        = 0x32;
    static const uint8_t NHDT         = 0x33;
    static const uint8_t NCLT         = 0x34;
    static const uint8_t FDLT         = 0x35;

    // Proximity sensing controls
    static const uint8_t MHDPROXR     = 0x36;
    static const uint8_t NHDPROXR     = 0x37;
    static const uint8_t NCLPROXR     = 0x38;
    static const uint8_t FDLPROXR     = 0x39;
    static const uint8_t MHDPROXF     = 0x3A;
    static const uint8_t NHDPROXF     = 0x3B;
    static const uint8_t NCLPROXF     = 0x3C;
    static const uint8_t FDLPROXF     = 0x3D;
    static const uint8_t NHDPROXT     = 0x3E;
    static const uint8_t NCLPROXT     = 0x3F;
    static const uint8_t FDLPROXT     = 0x40;

    // Electrode Touch/Release thresholds
    static const uint8_t ELE0_T       = 0x41;
    static const uint8_t ELE0_R       = 0x42;
    static const uint8_t ELE1_T       = 0x43;
    static const uint8_t ELE1_R       = 0x44;
    static const uint8_t ELE2_T       = 0x45;
    static const uint8_t ELE2_R       = 0x46;
    static const uint8_t ELE3_T       = 0x47;
    static const uint8_t ELE3_R       = 0x48;
    static const uint8_t ELE4_T       = 0x49;
    static const uint8_t ELE4_R       = 0x4A;
    static const uint8_t ELE5_T       = 0x4B;
    static const uint8_t ELE5_R       = 0x4C;
    static const uint8_t ELE6_T       = 0x4D;
    static const uint8_t ELE6_R       = 0x4E;
    static const uint8_t ELE7_T       = 0x4F;
    static const uint8_t ELE7_R       = 0x50;
    static const uint8_t ELE8_T       = 0x51;
    static const uint8_t ELE8_R       = 0x52;
    static const uint8_t ELE9_T       = 0x53;
    static const uint8_t ELE9_R       = 0x54;
    static const uint8_t ELE10_T      = 0x55;
    static const uint8_t ELE10_R      = 0x56;
    static const uint8_t ELE11_T      = 0x57;
    static const uint8_t ELE11_R      = 0x58;

    // Proximity Touch/Release thresholds
    static const uint8_t EPROXTTH     = 0x59;
    static const uint8_t EPROXRTH     = 0x5A;

    // Debounce configuration
    static const uint8_t DEB_CFG      = 0x5B;

    // AFE- Analogue Front End configuration
    static const uint8_t AFE_CFG      = 0x5C;

    // Filter configuration
    static const uint8_t FIL_CFG      = 0x5D;

    // Electrode configuration - transistions to "active mode"
    static const uint8_t ELE_CFG      = 0x5E;

    static const uint8_t GPIO_CTRL0   = 0x73;
    static const uint8_t GPIO_CTRL1   = 0x74;
    static const uint8_t GPIO_DATA    = 0x75;
    static const uint8_t GPIO_DIR     = 0x76;
    static const uint8_t GPIO_EN      = 0x77;
    static const uint8_t GPIO_SET     = 0x78;
    static const uint8_t GPIO_CLEAR   = 0x79;
    static const uint8_t GPIO_TOGGLE  = 0x7A;

    // Auto configration registers
    static const uint8_t AUTO_CFG_0   = 0x7B;
    static const uint8_t AUTO_CFG_U   = 0x7D;
    static const uint8_t AUTO_CFG_L   = 0x7E;
    static const uint8_t AUTO_CFG_T   = 0x7F;

    // Threshold defaults
    static const uint8_t E_THR_T      = 0x0F; // Electrode touch threshold
    static const uint8_t E_THR_R      = 0x0A; // Electrode release threshold
    static const uint8_t PROX_THR_T   = 0x02; // Prox touch threshold
    static const uint8_t PROX_THR_R   = 0x02; // Prox release threshold

};

#endif // MPR121_H
