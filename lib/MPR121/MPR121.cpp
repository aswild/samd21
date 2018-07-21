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

Ported to Arduino SAMD21 by Allen Wild, Jun 2018
*/

#include "MPR121.h"

MPR121::MPR121(TwoWire& _i2c, uint8_t _address) : i2c(_i2c), address(_address)
{
    // nothing to do besides initialize private vars
}

void MPR121::init(bool i2cBegin)
{
    // the user may call i2c.begin() and configure pin mappings before calling this function
    if (i2cBegin)
        i2c.begin();

    // Configure the MPR121 settings to default
    configureSettings();
}

void MPR121::configureSettings(void)
{
    // Put the MPR into setup mode
    this->write(ELE_CFG,0x00);

    // Electrode filters for when data is > baseline
    const uint8_t gtBaseline[] = {
         0x01,  //MHD_R
         0x01,  //NHD_R
         0x00,  //NCL_R
         0x00   //FDL_R
         };

    writeMany(MHD_R,gtBaseline,4);

     // Electrode filters for when data is < baseline
     const uint8_t ltBaseline[] = {
        0x01,   //MHD_F
        0x01,   //NHD_F
        0xFF,   //NCL_F
        0x02    //FDL_F
        };

    writeMany(MHD_F,ltBaseline,4);

    // Electrode touch and release thresholds
    const uint8_t electrodeThresholds[] = {
        E_THR_T, // Touch Threshhold
        E_THR_R  // Release Threshold
        };

    for(int i=0; i<12; i++){
        writeMany((ELE0_T+(i*2)),electrodeThresholds,2);
    }

    // Proximity Settings
    const uint8_t proximitySettings[] = {
        0xff,   //MHD_Prox_R
        0xff,   //NHD_Prox_R
        0x00,   //NCL_Prox_R
        0x00,   //FDL_Prox_R
        0x01,   //MHD_Prox_F
        0x01,   //NHD_Prox_F
        0xFF,   //NCL_Prox_F
        0xff,   //FDL_Prox_F
        0x00,   //NHD_Prox_T
        0x00,   //NCL_Prox_T
        0x00    //NFD_Prox_T
        };
    writeMany(MHDPROXR,proximitySettings,11);

    const uint8_t proxThresh[] = {
        PROX_THR_T, // Touch Threshold
        PROX_THR_R  // Release Threshold
        };
    writeMany(EPROXTTH,proxThresh,2);

    this->write(FIL_CFG,0x04);

    // Set the electrode config to transition to active mode
    this->write(ELE_CFG,0x0c);
}

void MPR121::setElectrodeThreshold(int electrode, uint8_t touch, uint8_t release)
{
    if(electrode > 11) return;

    // Get the current mode
    uint8_t mode = this->read(ELE_CFG);

    // Put the MPR into setup mode
    this->write(ELE_CFG,0x00);

    // Write the new threshold
    this->write((ELE0_T+(electrode*2)), touch);
    this->write((ELE0_T+(electrode*2)+1), release);

    //Restore the operating mode
    this->write(ELE_CFG, mode);
}

uint8_t MPR121::read(uint8_t reg)
{
    uint8_t data = 0xff;

    //Start the command
    i2c.beginTransmission(address);

    // Set the register key to read, no stop bit
    i2c.write(reg);
    i2c.endTransmission(false);

    // Re-send the target address in read mode
    i2c.requestFrom(address, 1);

    // Read in the result
    data = i2c.read();

    return data;
}

size_t MPR121::readMany(uint8_t start, uint8_t *dataSet, size_t length)
{
    // set the register to read, no stop bit
    i2c.beginTransmission(address);
    i2c.write(start);
    i2c.endTransmission(false);

    // initiate read (SW buffered)
    i2c.requestFrom(address, length);

    // read response (from SW buffer)
    size_t i;
    for (i = 0; i2c.available() && i < length; i++)
        dataSet[i] = i2c.read();

    return i;
}

size_t MPR121::write(uint8_t reg, uint8_t value)
{
    // Start the command
    i2c.beginTransmission(address);

    // write the register address and value
    i2c.write(reg);
    i2c.write(value);

    // send the data and return the status
    return i2c.endTransmission();
}

size_t MPR121::writeMany(uint8_t start, const uint8_t* dataSet, size_t length)
{
    //Start the command
    i2c.beginTransmission(address);

    // write the first register address
    i2c.write(start);

    // write the data
    for (size_t i = 0; i < length; i++)
        i2c.write(dataSet[i]);

    // send the data and return the status
    return i2c.endTransmission();
}

bool MPR121::getProximityMode(void)
{
    return this->read(ELE_CFG) > 0x0c;
}

void MPR121::setProximityMode(bool mode)
{
    this->write(ELE_CFG,0x00);
    if(mode) {
        this->write(ELE_CFG,0x30); //Sense proximity from ALL pads
    } else {
        this->write(ELE_CFG,0x0c); //Sense touch, all 12 pads active.
    }
}

uint16_t MPR121::readTouchData(void)
{
    uint8_t data[2] = {0};
    readMany(ELE_ST0, data, 2);
    return ((data[1] & ELE_ST1_MASK) << 8) | data[0];
}
