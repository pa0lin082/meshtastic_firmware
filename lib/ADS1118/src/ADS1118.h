#ifndef ADS1118_h
#define ADS1118_h

#include "<driver_ads1118.h>"
#include "Arduino.h"
#include <SPI.h>
#include <stdint.h>
/**
 * Union representing the "config register" in 3 ways:
 * bits, word (16 bits) and nibbles (4 bits)
 * (See the datasheet [1] for more information)
 */
/// Union configuration register
union Config {
    /// Structure of the config register of the ADS1118. (See datasheet [1])
    struct {
        uint8_t reserved : 1;      ///< "Reserved" bit
        uint8_t noOperation : 2;   ///< "NOP" bits
        uint8_t pullUp : 1;        ///< "PULL_UP_EN" bit
        uint8_t sensorMode : 1;    ///< "TS_MODE" bit
        uint8_t rate : 3;          ///< "DR" bits
        uint8_t operatingMode : 1; ///< "MODE" bit
        uint8_t pga : 3;           ///< "PGA" bits
        uint8_t mux : 3;           ///< "MUX" bits
        uint8_t singleStart : 1;   ///< "SS" bit
    } bits;
    uint16_t word; ///< Representation in word (16-bits) format
    struct {
        uint8_t lsb; ///< Byte LSB
        uint8_t msb; ///< Byte MSB
    } byte;          ///< Representation in bytes (8-bits) format
};

/**
 * Class representing the ADS1118 sensor chip
 * @author Alvaro Salazar <alvaro@denkitronik.com>
 */
class ADS1118
{
  public:
    void begin(); ///< This method initialize the SPI port and the config register
#if defined(__AVR__)
    ADS1118(uint8_t io_pin_cs); ///< Constructor
#elif defined(ESP32)
    ADS1118(uint8_t io_pin_cs, SPIClass *spi = &SPI);     ///< Constructor
    void begin(uint8_t sclk, uint8_t miso, uint8_t mosi); ///< This method initialize the SPI port and the config register
#endif
    double getTemperature(); ///< Getting the temperature in degrees celsius from the internal sensor of the ADS1118
    uint16_t getADCValue(ads1118_channel_t inputs); ///< Getting a sample from the specified input
    bool getADCValueNoWait(ads1118_channel_t pin_drdy, uint16_t &value);
    bool getMilliVoltsNoWait(ads1118_channel_t pin_drdy, double &volts); ///< Getting the millivolts from the settled inputs
    double getMilliVolts(ads1118_channel_t inputs);                      ///< Getting the millivolts from the specified inputs
    double getMilliVolts();                                              ///< Getting the millivolts from the settled inputs
    void decodeConfigRegister(
        union Config configRegister);           ///< Decoding a configRegister structure and then print it out to the Serial port
    void setSamplingRate(uint8_t samplingRate); ///< Setting the sampling rate specified in the config register
    void setFullScaleRange(uint8_t fsr);        ///< Setting the full scale range in the config register
    void setContinuousMode();                   ///< Setting to continuous adquisition mode
    void setSingleShotMode();                   ///< Setting to single shot adquisition and power down mode
    void disablePullup();                       ///< Disabling the internal pull-up resistor of the DOUT pin
    void enablePullup();                        ///< Enabling the internal pull-up resistor of the DOUT pin
    void setInputSelected(uint8_t input);       ///< Setting the inputs to be adquired in the config register.
                                                // Input multiplexer configuration selection for bits "MUX"
    // Differential inputs
    const ads1118_channel_t DIFF_0_1 = ADS1118_CHANNEL_AIN0_AIN1; ///< Differential input: Vin=A0-A1
    const ads1118_channel_t DIFF_0_3 = ADS1118_CHANNEL_AIN0_AIN3; ///< Differential input: Vin=A0-A3
    const ads1118_channel_t DIFF_1_3 = ADS1118_CHANNEL_AIN1_AIN3; ///< Differential input: Vin=A1-A3
    const ads1118_channel_t DIFF_2_3 = ADS1118_CHANNEL_AIN2_AIN3; ///< Differential input: Vin=A2-A3
                                                                  // Single ended inputs
    const ads1118_channel_t AIN_0 = ADS1118_CHANNEL_AIN0_GND;     ///< Single ended input: Vin=A0
    const ads1118_channel_t AIN_1 = ADS1118_CHANNEL_AIN1_GND;     ///< Single ended input: Vin=A1
    const ads1118_channel_t AIN_2 = ADS1118_CHANNEL_AIN2_GND;     ///< Single ended input: Vin=A2
    const ads1118_channel_t AIN_3 = ADS1118_CHANNEL_AIN3_GND;     ///< Single ended input: Vin=A3
    union Config configRegister;                                  ///< Config register

    // Bit constants
    const uint32_t SCLK = 2000000; ///< ADS1118 SCLK frequency: 4000000 Hz Maximum for ADS1118

    // Used by "SS" bit
    const uint8_t START_NOW = 1; ///< Start of conversion in single-shot mode

    // Used by "TS_MODE" bit
    const ads1118_mode_t ADC_MODE = ADS1118_MODE_ADC;          ///< External (inputs) voltage reading mode
    const ads1118_mode_t TEMP_MODE = ADS1118_MODE_TEMPERATURE; ///< Internal temperature sensor reading mode

    // Used by "MODE" bit
    const uint8_t CONTINUOUS = 0;  ///< Continuous conversion mode
    const uint8_t SINGLE_SHOT = 1; ///< Single-shot conversion and power down mode

    // Used by "PULL_UP_EN" bit
    const uint8_t DOUT_PULLUP = 1;    ///< Internal pull-up resistor enabled for DOUT ***DEFAULT
    const uint8_t DOUT_NO_PULLUP = 0; ///< Internal pull-up resistor disabled

    // Used by "NOP" bits
    const uint8_t VALID_CFG = 0b01;    ///< Data will be written to Config register
    const uint8_t NO_VALID_CFG = 0b00; ///< Data won't be written to Config register

    // Used by "Reserved" bit
    const uint8_t RESERVED = 1; ///< Its value is always 1, reserved

    /*Full scale range (FSR) selection by "PGA" bits.
             [Warning: this could increase the noise and the effective number of bits (ENOB). See tables above]*/
    const ads1118_range_t FSR_6144 = ADS1118_RANGE_6P144V; ///< Range: ±6.144 v. LSB SIZE = 187.5μV
    const ads1118_range_t FSR_4096 = ADS1118_RANGE_4P096V; ///< Range: ±4.096 v. LSB SIZE = 125μV
    const ads1118_range_t FSR_2048 = ADS1118_RANGE_2P048V; ///< Range: ±2.048 v. LSB SIZE = 62.5μV ***DEFAULT
    const ads1118_range_t FSR_1024 = ADS1118_RANGE_1P024V; ///< Range: ±1.024 v. LSB SIZE = 31.25μV
    const ads1118_range_t FSR_0512 = ADS1118_RANGE_0P512V; ///< Range: ±0.512 v. LSB SIZE = 15.625μV
    const ads1118_range_t FSR_0256 = ADS1118_RANGE_0P256V; ///< Range: ±0.256 v. LSB SIZE = 7.8125μV

    /*Sampling rate selection by "DR" bits.
            [Warning: this could increase the noise and the effective number of bits (ENOB). See tables above]*/
    const ads1118_rate_t RATE_8SPS = ADS1118_RATE_8SPS;     ///< 8 samples/s, Tconv=125ms
    const ads1118_rate_t RATE_16SPS = ADS1118_RATE_16SPS;   ///< 16 samples/s, Tconv=62.5ms
    const ads1118_rate_t RATE_32SPS = ADS1118_RATE_32SPS;   ///< 32 samples/s, Tconv=31.25ms
    const ads1118_rate_t RATE_64SPS = ADS1118_RATE_64SPS;   ///< 64 samples/s, Tconv=15.625ms
    const ads1118_rate_t RATE_128SPS = ADS1118_RATE_128SPS; ///< 128 samples/s, Tconv=7.8125ms
    const ads1118_rate_t RATE_250SPS = ADS1118_RATE_250SPS; ///< 250 samples/s, Tconv=4ms
    const ads1118_rate_t RATE_475SPS = ADS1118_RATE_475SPS; ///< 475 samples/s, Tconv=2.105ms
    const ads1118_rate_t RATE_860SPS = ADS1118_RATE_860SPS; ///< 860 samples/s, Tconv=1.163ms

  private:
#if defined(ESP32)
    SPIClass *pSpi;
#endif
    uint8_t lastSensorMode = 3; ///< Last sensor mode selected (ADC_MODE or TEMP_MODE or none)
    uint8_t cs;                 ///< Chip select pin (choose one)
    const float pgaFSR[8] = {6.144, 4.096, 2.048, 1.024, 0.512, 0.256, 0.256, 0.256};
    const uint8_t CONV_TIME[8] = {125, 63, 32, 16, 8, 4, 3, 2}; ///< Array containing the conversions time in ms

    /*
                                                            Table 1. Noise in μVRMS (μVPP) at VDD = 3.3 V   [1]
                                                                    DATA RATE FSR (Full-Scale Range)
      (SPS) ±6.144 V 		±4.096 V 	 ±2.048 V 		±1.024 V 		±0.512 V 		±0.256 V
            8 	187.5 (187.5) 	125 (125) 	 62.5 (62.5) 	31.25 (31.25) 	15.62 (15.62) 	7.81 (7.81)
            16 	187.5 (187.5) 	125 (125) 	 62.5 (62.5) 	31.25 (31.25) 	15.62 (15.62) 	7.81 (7.81)
            32 	187.5 (187.5) 	125 (125) 	 62.5 (62.5) 	31.25 (31.25) 	15.62 (15.62) 	7.81 (7.81)
            64 	187.5 (187.5) 	125 (125) 	 62.5 (62.5) 	31.25 (31.25) 	15.62 (15.62) 	7.81 (7.81)
            128 187.5 (187.5) 	125 (125) 	 62.5 (62.5) 	31.25 (31.25) 	15.62 (15.62) 	7.81 (12.35)
            250 187.5 (252.09) 	125 (148.28) 62.5 (84.03) 	31.25 (39.54) 	15.62 (16.06) 	7.81 (18.53)
            475 187.5 (266.92) 	125 (227.38) 62.5 (79.08) 	31.25 (56.84) 	15.62 (32.13) 	7.81 (25.95)
            860 187.5 (430.06) 	125 (266.93) 62.5 (118.63) 	31.25 (64.26) 	15.62 (40.78) 	7.81 (35.83)


                            Table 2. ENOB from RMS Noise (Noise-Free Bits from Peak-to-Peak Noise) at VDD = 3.3 V
                                                                    DATA RATE FSR (Full-Scale Range)
      (SPS) 	±6.144 V 	±4.096 V 	 ±2.048 V 		±1.024 V 		±0.512 V 		±0.256 V
            8 		16 (16) 	16 (16) 	 16 (16) 		16 (16) 		16 (16) 		16
      (16) 16 		16 (16) 	16 (16) 	 16 (16) 		16 (16) 		16 (16) 		16
      (16) 32 		16 (16) 	16 (16) 	 16 (16) 		16 (16) 		16 (16) 		16
      (16) 64 		16 (16) 	16 (16) 	 16 (16) 		16 (16) 		16 (16) 		16
      (16) 128 	16 (16) 	16 (16) 	 16 (16) 		16 (16) 		16 (16) 		16 (15.33)
            250 	16 (15.57) 	16 (15.75) 	 16 (15.57) 	16 (15.66) 		16 (15.96) 		16 (14.75)
            475 	16 (15.49) 	16 (15.13) 	 16 (15.66)  	16 (15.13) 		16 (14.95) 		16 (14.26)
            860 	16 (14.8) 	16 (14.9) 	 16 (15.07) 	16 (14.95) 		16 (14.61) 		16 (13.8)


            [1] Texas Instruments, "ADS1118 Ultrasmall, Low-Power, SPI™-Compatible, 16-Bit Analog-to-Digital
            Converter with Internal Reference and Temperature Sensor", ADS1118 datasheet, SBAS457E [OCTOBER 2010–REVISED OCTOBER
      2015].

            Note: This information is taken from http://www.ti.com
                  Copyright © 2010–2015, Texas Instruments Incorporated
    */
};

#endif