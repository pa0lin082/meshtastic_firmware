#ifndef ADS1118_h
#define ADS1118_h

#include "Arduino.h"
#include "driver_ads1118.h"
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
        uint8_t reserved : 1;             ///< "Reserved" bit
        uint8_t noOperation : 2;          ///< "NOP" bits
        ads1118_bool_t pullUp : 1;        ///< "PULL_UP_EN" bit
        ads1118_mode_t sensorMode : 1;    ///< "TS_MODE" bit
        ads1118_rate_t rate : 3;          ///< "DR" bits
        ads1118_bool_t operatingMode : 1; ///< "MODE" bit
        ads1118_range_t pga : 3;          ///< "PGA" bits
        ads1118_channel_t mux : 3;        ///< "MUX" bits
        uint8_t singleStart : 1;          ///< "SS" bit
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
        union Config configRegister); ///< Decoding a configRegister structure and then print it out to the Serial port
    uint8_t setSamplingRate(ads1118_rate_t samplingRate); ///< Setting the sampling rate specified in the config register
    uint8_t setFullScaleRange(ads1118_range_t fsr);       ///< Setting the full scale range in the config register
    uint8_t setContinuousMode();                          ///< Setting to continuous adquisition mode
    uint8_t setSingleShotMode();                          ///< Setting to single shot adquisition and power down mode
    uint8_t disablePullup();                              ///< Disabling the internal pull-up resistor of the DOUT pin
    uint8_t enablePullup();                               ///< Enabling the internal pull-up resistor of the DOUT pin
    uint8_t setInputSelected(ads1118_channel_t input);    ///< Setting the inputs to be adquired in the config register.
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
    const ads1118_bool_t CONTINUOUS = ADS1118_BOOL_FALSE; ///< Continuous conversion mode
    const ads1118_bool_t SINGLE_SHOT = ADS1118_BOOL_TRUE; ///< Single-shot conversion and power down mode

    // Used by "PULL_UP_EN" bit
    const ads1118_bool_t DOUT_PULLUP = ADS1118_BOOL_TRUE;     ///< Internal pull-up resistor enabled for DOUT ***DEFAULT
    const ads1118_bool_t DOUT_NO_PULLUP = ADS1118_BOOL_FALSE; ///< Internal pull-up resistor disabled

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
    ads1118_handle_t gs_handle;
    uint8_t lastSensorMode = 3; ///< Last sensor mode selected (ADC_MODE or TEMP_MODE or none)
    uint8_t cs;                 ///< Chip select pin (choose one)
    const float pgaFSR[8] = {6.144, 4.096, 2.048, 1.024, 0.512, 0.256, 0.256, 0.256};
    const uint8_t CONV_TIME[8] = {125, 63, 32, 16, 8, 4, 3, 2}; ///< Array containing the conversions time in ms

    uint8_t setupModule(void);
    void setDefaultConfig(void);
    // Metodi di istanza per l'interfaccia SPI
    uint8_t spi_init_impl(void);
    uint8_t spi_deinit_impl(void);
    uint8_t spi_transmit_impl(uint8_t *tx, uint8_t *rx, uint16_t len);
    void delay_ms_impl(uint32_t ms);
    void debug_print_impl(const char *const fmt, ...);

    // Funzioni wrapper statiche per il driver
    static uint8_t spi_init_wrapper(void);
    static uint8_t spi_deinit_wrapper(void);
    static uint8_t spi_transmit_wrapper(uint8_t *tx, uint8_t *rx, uint16_t len);
    static void delay_ms_wrapper(uint32_t ms);
    static void debug_print_wrapper(const char *const fmt, ...);

    // Puntatore all'istanza corrente per le funzioni statiche
    static ADS1118 *current_instance;
};

#endif