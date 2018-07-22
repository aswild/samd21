/*
  This sketch easily and quickly finds the right ADC correction values for a particular Arduino ZERO board.
  The correction values that are found are only valid for the board where the sketch is executed.

  This example code is in the public domain.

  Written 6 May 2015 by Claudio Indellicati
*/

/*
  How to use this sketch

  1) Remove any connection cable, shield or jumper from your Arduino ZERO
  2) Connect pin A1 to the nearest GND pin using the shortest jumper possible
  3) Connect pin A2 to the 3.3V pin using the shortest jumper possible
  4) Connect the Arduino ZERO to your PC using a USB cable plugged in the USB programming port of the board
  5) Upload this sketch and leave the board powered on for at least one minute
  6) Open the SerialUSB Monitor and press the reset button on the Arduino ZERO
  7) At the and of the procedure you can find logged
       - the offset and gain values for the board where the sketch has been just executed
       - the instruction line to copy/paste in the final sketch
*/

#include "Arduino.h"

#define ADC_GND_PIN          A1
#define ADC_3V3_PIN          A2

#define ADC_READS_SHIFT      8
#define ADC_READS_COUNT      (1 << ADC_READS_SHIFT)

#define ADC_MIN_GAIN         0x0400
#define ADC_UNITY_GAIN       0x0800
#define ADC_MAX_GAIN         (0x1000 - 1)
#define ADC_RESOLUTION_BITS  12
#define ADC_RANGE            (1 << ADC_RESOLUTION_BITS)
#define ADC_TOP_VALUE        (ADC_RANGE - 1)

#define MAX_TOP_VALUE_READS  10

static void analogReadCorrection (int offset, uint16_t gain)
{
  // Set correction values
  ADC->OFFSETCORR.reg = ADC_OFFSETCORR_OFFSETCORR(offset);
  ADC->GAINCORR.reg = ADC_GAINCORR_GAINCORR(gain);

  // Enable digital correction logic
  ADC->CTRLB.bit.CORREN = 1;
  while(ADC->STATUS.bit.SYNCBUSY);
}

uint16_t readGndLevel()
{
  uint32_t readAccumulator = 0;

  for (int i = 0; i < ADC_READS_COUNT; ++i)
    readAccumulator += analogRead(ADC_GND_PIN);

  uint16_t readValue = readAccumulator >> ADC_READS_SHIFT;

  SerialUSB.print("ADC(GND) = ");
  SerialUSB.println(readValue);

  return readValue;
}

uint16_t read3V3Level()
{
  uint32_t readAccumulator = 0;

  for (int i = 0; i < ADC_READS_COUNT; ++i)
    readAccumulator += analogRead(ADC_3V3_PIN);

  uint16_t readValue = readAccumulator >> ADC_READS_SHIFT;

  if (readValue < (ADC_RANGE >> 1))
    readValue += ADC_RANGE;

  SerialUSB.print("ADC(3.3V) = ");
  SerialUSB.println(readValue);

  return readValue;
}

void setup()
{
  while (!SerialUSB);
  SerialUSB.begin(9600);

  SerialUSB.println("\r\nCalibrating ADC with factory values");

  analogReadResolution(ADC_RESOLUTION_BITS);

  SerialUSB.println("\r\nReading GND and 3.3V ADC levels");
  SerialUSB.print("   ");
  readGndLevel();
  SerialUSB.print("   ");
  read3V3Level();

  int offsetCorrectionValue = 0;
  uint16_t gainCorrectionValue = ADC_UNITY_GAIN;

  SerialUSB.print("\r\nOffset correction (@gain = ");
  SerialUSB.print(gainCorrectionValue);
  SerialUSB.println(" (unity gain))");

  // Set default correction values and enable correction
  analogReadCorrection(offsetCorrectionValue, gainCorrectionValue);

  for (int offset = 0; offset < (int)(ADC_OFFSETCORR_MASK >> 1); ++offset)
  {
    analogReadCorrection(offset, gainCorrectionValue);

    SerialUSB.print("   Offset = ");
    SerialUSB.print(offset);
    SerialUSB.print(", ");

    if (readGndLevel() == 0)
    {
      offsetCorrectionValue = offset;
      break;
    }
  }

  SerialUSB.println("\r\nGain correction");

  uint8_t topValueReadsCount = 0U;

  uint16_t minGain = 0U,
           maxGain = 0U;

  analogReadCorrection(offsetCorrectionValue, gainCorrectionValue);
  SerialUSB.print("   Gain = ");
  SerialUSB.print(gainCorrectionValue);
  SerialUSB.print(", ");
  uint16_t highLevelRead = read3V3Level();

  if (highLevelRead < ADC_TOP_VALUE)
  {
    for (uint16_t gain = ADC_UNITY_GAIN + 1; gain <= ADC_MAX_GAIN; ++gain)
    {
      analogReadCorrection(offsetCorrectionValue, gain);

      SerialUSB.print("   Gain = ");
      SerialUSB.print(gain);
      SerialUSB.print(", ");
      highLevelRead = read3V3Level();

      if (highLevelRead == ADC_TOP_VALUE)
      {
        if (minGain == 0U)
          minGain = gain;

        if (++topValueReadsCount >= MAX_TOP_VALUE_READS)
        {
          maxGain = minGain;
          break;
        }

        maxGain = gain;
      }

      if (highLevelRead > ADC_TOP_VALUE)
        break;
    }
  }
  else if (highLevelRead >= ADC_TOP_VALUE)
  {
    if (highLevelRead == ADC_TOP_VALUE)
      maxGain = ADC_UNITY_GAIN;

    for (uint16_t gain = ADC_UNITY_GAIN - 1; gain >= ADC_MIN_GAIN; --gain)
    {
      analogReadCorrection(offsetCorrectionValue, gain);

      SerialUSB.print("   Gain = ");
      SerialUSB.print(gain);
      SerialUSB.print(", ");
      highLevelRead = read3V3Level();

      if (highLevelRead == ADC_TOP_VALUE)
      {
        if (maxGain == 0U)
          maxGain = gain;

        minGain = gain;
      }

      if (highLevelRead < ADC_TOP_VALUE)
        break;
    }
  }

  gainCorrectionValue = (minGain + maxGain) >> 1;

  analogReadCorrection(offsetCorrectionValue, gainCorrectionValue);

  SerialUSB.println("\r\nReadings after corrections");
  SerialUSB.print("   ");
  readGndLevel();
  SerialUSB.print("   ");
  read3V3Level();

  SerialUSB.println("\r\n==================");
  SerialUSB.println("\r\nCorrection values:");
  SerialUSB.print("   Offset = ");
  SerialUSB.println(offsetCorrectionValue);
  SerialUSB.print("   Gain = ");
  SerialUSB.println(gainCorrectionValue);
  SerialUSB.println("\r\nAdd the next line to your sketch:");
  SerialUSB.print("   analogReadCorrection(");
  SerialUSB.print(offsetCorrectionValue);
  SerialUSB.print(", ");
  SerialUSB.print(gainCorrectionValue);
  SerialUSB.println(");");
  SerialUSB.println("\r\n==================");
}

void loop()
{
}
