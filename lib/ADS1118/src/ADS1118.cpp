/**
 *  Arduino Library for Texas Instruments ADS1118 - 16-Bit Analog-to-Digital Converter with
 *  Internal Reference and Temperature Sensor
 *
 *  @author Alvaro Salazar <alvaro@denkitronik.com>
 *  http://www.denkitronik.com
 *
 */

/**
 * The MIT License
 *
 * Copyright 2018 Alvaro Salazar <alvaro@denkitronik.com>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ADS1118.h"
#include "Arduino.h"
#include "driver_ads1118.h"

// Definizione della variabile statica per l'istanza corrente
ADS1118 *ADS1118::current_instance = nullptr;

/**
 * Debugging:
 *   Uncomment some of the 3 lines above to debug the method you want.
 * Warning:
 *   Keep them commented in production
 */
// #define DEBUG_BEGIN  			//Debug begin() method
// #define DEBUG_GETADCVALUE  	//Debug getADCValue() method
// #define DEBUG_GETTEMPERATURE  //Debug getTemperature() method

#define DEBUG_BEGIN true
#define DEBUG_GETADCVALUE true
#define DEBUG_GETTEMPERATURE true

#ifdef DEBUG_BEGIN
#define DEBUG_BEGIN(x) decodeConfigRegister(x)
#else
#define DEBUG_BEGIN(x)
#endif

#ifdef DEBUG_GETADCVALUE
#define DEBUG_GETADCVALUE(x) decodeConfigRegister(x)
#else
#define DEBUG_GETADCVALUE(x)
#endif

#ifdef DEBUG_GETTEMPERATURE
#define DEBUG_GETTEMPERATURE(x) decodeConfigRegister(x)
#else
#define DEBUG_GETTEMPERATURE(x)
#endif

#if defined(__AVR__)
/**
 * Constructor of the class
 * @param io_pin_cs a byte indicating the pin to be use as the chip select pin (CS)
 */
ADS1118::ADS1118(uint8_t io_pin_cs)
{
    cs = io_pin_cs;
} ///< This method initialize the SPI port and the config register
#elif defined(ESP32)
/**
 * Constructor of the class
 * @param io_pin_cs a byte indicating the pin to be use as the chip select pin (CS)
 */
ADS1118::ADS1118(uint8_t io_pin_cs, SPIClass *spi)
{
    cs = io_pin_cs;
    pSpi = spi;

    // Imposta questa istanza come corrente per le funzioni statiche
    current_instance = this;

    uint8_t res;
    DRIVER_ADS1118_LINK_INIT(&gs_handle, ads1118_handle_t);
    DRIVER_ADS1118_LINK_SPI_INIT(&gs_handle, spi_init_wrapper);
    DRIVER_ADS1118_LINK_SPI_DEINIT(&gs_handle, spi_deinit_wrapper);
    DRIVER_ADS1118_LINK_SPI_TRANSMIT(&gs_handle, spi_transmit_wrapper);
    DRIVER_ADS1118_LINK_DELAY_MS(&gs_handle, delay_ms_wrapper);
    DRIVER_ADS1118_LINK_DEBUG_PRINT(&gs_handle, debug_print_wrapper);

    res = ads1118_init(&gs_handle);
    if (res != 0) {
        debug_print_impl("ads1118: init failed.\n");
    }
}
#endif

#if defined(__AVR__)
/**
 * This method initialize the SPI port and the config register
 */
void ADS1118::begin()
{
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);
    SPI.begin();
    SPI.beginTransaction(SPISettings(SCLK, MSBFIRST, SPI_MODE1));
    setDefaultConfig();
} ///< This method initialize the SPI port and the config register
#elif defined(ESP32)
/**
 * This method initialize the SPI port and the config register
 */
void ADS1118::begin()
{
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);
    pSpi->begin();
    setDefaultConfig();

    double res = setupModule();
    if (res != 0) {
        debug_print_impl("ads1118: setupModule failed.\n");
    }
}

void ADS1118::begin(uint8_t sclk, uint8_t miso, uint8_t mosi)
{
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);
    pSpi->begin(sclk, miso, mosi, cs);
    setDefaultConfig();

    double res = setupModule();
    if (res != 0) {
        debug_print_impl("ads1118: setupModule failed.\n");
    }
}

void ADS1118::setDefaultConfig()
{
    configRegister.bits = {RESERVED,   VALID_CFG, DOUT_NO_PULLUP, ADC_MODE, RATE_8SPS,
                           CONTINUOUS, FSR_4096,  AIN_3,          START_NOW}; // Default values
    DEBUG_BEGIN(configRegister); // Debug this method: print the config register in the Serial port
}

uint8_t ADS1118::setupModule()
{
    uint8_t res;
    res = setInputSelected(configRegister.bits.mux);
    if (res != 0) {
        debug_print_impl("ads1118: setInputSelected failed.\n");
    }
    res = setSamplingRate(configRegister.bits.rate);
    if (res != 0) {
        debug_print_impl("ads1118: setSamplingRate failed.\n");
    }
    res = setFullScaleRange(configRegister.bits.pga);
    if (res != 0) {
        debug_print_impl("ads1118: setFullScaleRange failed.\n");
    }

    if (configRegister.bits.operatingMode == CONTINUOUS) {
        res = setContinuousMode();
    } else {
        res = setSingleShotMode();
    }
    if (res != 0) {
        debug_print_impl("ads1118: setContinuousMode or setSingleShotMode failed.\n");
    }
    if (configRegister.bits.pullUp == DOUT_PULLUP) {
        res = enablePullup();
    } else {
        res = disablePullup();
    }
    if (res != 0) {
        debug_print_impl("ads1118: enablePullup or disablePullup failed.\n");
    }
    return res;
}

/**
 * Getting a sample from the specified input if data is ready
 * @param pin_drdy io pin connected to ADS1118 DOUT/DRDY. value Reference of ADC value to be fetched
 * @return True if ADC data is ready
 */
bool ADS1118::getADCValueNoWait(ads1118_channel_t pin_drdy, uint16_t &value)
{
    // byte dataMSB, dataLSB;
    // pSpi->beginTransaction(SPISettings(SCLK, MSBFIRST, SPI_MODE1));
    // digitalWrite(cs, LOW);
    // if (digitalRead(pin_drdy)) {
    //     digitalWrite(cs, HIGH);
    //     pSpi->endTransaction();
    //     return false;
    // }

    // dataMSB = pSpi->transfer(configRegister.byte.msb);
    // dataLSB = pSpi->transfer(configRegister.byte.lsb);
    // digitalWrite(cs, HIGH);
    // pSpi->endTransaction();

    // value = (dataMSB << 8) | (dataLSB);
    // return true;
}

/**
 * Getting the millivolts from the settled inputs
 * @return A double (32bits) containing the ADC value in millivolts
 */
bool ADS1118::getMilliVoltsNoWait(ads1118_channel_t pin_drdy, double &volts)
{
    // float fsr = pgaFSR[configRegister.bits.pga];
    // uint16_t value;
    // bool dataReady = getADCValueNoWait(pin_drdy, value);
    // if (!dataReady)
    //     return false;
    // if (value >= 0x8000) {
    //     value = ((~value) + 1); // Applying binary twos complement format
    //     volts = ((float)(value * fsr / 32768) * -1);
    // } else {
    //     volts = (float)(value * fsr / 32768);
    // }
    // volts = volts * 1000;
    // return true;
}
#endif

/**
 * Getting a sample from the specified input
 * @param inputs Sets the input of the ADC: Diferential inputs: DIFF_0_1, DIFF_0_3, DIFF_1_3, DIFF_2_3. Single ended input: AIN_0,
 * AIN_1, AIN_2, AIN_3
 * @return A word containing the ADC value
 */
uint16_t ADS1118::getADCValue(ads1118_channel_t inputs)
{
    //     uint16_t value;
    //     byte dataMSB, dataLSB, configMSB, configLSB, count = 0;
    //     if (lastSensorMode == ADC_MODE) // Lucky you! We don't have to read twice the sensor
    //         count = 1;
    //     else
    //         configRegister.bits.sensorMode = ADC_MODE; // Sorry but we will have to read twice the sensor
    //     configRegister.bits.mux = inputs;
    //     do {
    // #if defined(ESP32)
    //         pSpi->beginTransaction(SPISettings(SCLK, MSBFIRST, SPI_MODE1));
    // #endif
    //         digitalWrite(cs, LOW);
    // #if defined(__AVR__)
    //         dataMSB = SPI.transfer(configRegister.byte.msb);
    //         dataLSB = SPI.transfer(configRegister.byte.lsb);
    //         configMSB = SPI.transfer(configRegister.byte.msb);
    //         configLSB = SPI.transfer(configRegister.byte.lsb);
    // #elif defined(ESP32)
    //         dataMSB = pSpi->transfer(configRegister.byte.msb);
    //         dataLSB = pSpi->transfer(configRegister.byte.lsb);
    //         configMSB = pSpi->transfer(configRegister.byte.msb);
    //         configLSB = pSpi->transfer(configRegister.byte.lsb);
    // #endif

    //         digitalWrite(cs, HIGH);
    // #if defined(ESP32)
    //         pSpi->endTransaction();
    // #endif
    //         for (int i = 0; i < CONV_TIME[configRegister.bits.rate]; i++) // Lets wait the conversion time
    //             delayMicroseconds(1000);
    //         count++;
    //     } while (count <= 1);              // We make two readings because the second reading is the ADC conversion.
    //     DEBUG_GETADCVALUE(configRegister); // Debug this method: print the config register in the Serial port
    //     value = (dataMSB << 8) | (dataLSB);
    //     return value;
}

/**
 * Getting the millivolts from the specified inputs
 * @param inputs Sets the inputs to be adquired. Diferential inputs: DIFF_0_1, DIFF_0_3, DIFF_1_3, DIFF_2_3. Single ended input:
 * AIN_0, AIN_1, AIN_2, AIN_3
 * @return A double (32bits) containing the ADC value in millivolts
 */
double ADS1118::getMilliVolts(ads1118_channel_t inputs)
{
    if (configRegister.bits.sensorMode != ADC_MODE) {
        configRegister.bits.sensorMode = ADC_MODE;
        ads1118_set_mode(&gs_handle, configRegister.bits.sensorMode);
    }
    int16_t raw;
    float volts;
    uint8_t res;

    // Sempre imposta il canale per garantire che sia corretto
    setInputSelected(inputs);
    // Aspetta che la configurazione sia applicata
    delay(150);

    if (configRegister.bits.operatingMode == SINGLE_SHOT) {
        if ((res = ads1118_single_read(&gs_handle, &raw, &volts)) != 0) {
            Serial.println("getMilliVolts single read failed with error: ");
            // throw std::runtime_error("getMilliVolts single read failed with error: " + std::to_string(res));
        }
    } else {
        if ((res = ads1118_continuous_read(&gs_handle, &raw, &volts)) != 0) {
            Serial.println("getMilliVolts continuous read failed with error: ");
            // throw std::runtime_error("getMilliVolts continuous read failed with error: " + std::to_string(res));
        }
    }

    return volts * 1000;
    // float volts;
    // float fsr = pgaFSR[configRegister.bits.pga];
    // uint16_t value;
    // value = getADCValue(inputs);
    // if (value >= 0x8000) {
    //     value = ((~value) + 1); // Applying binary twos complement format
    //     volts = ((float)(value * fsr / 32768) * -1);
    // } else {
    //     volts = (float)(value * fsr / 32768);
    // }
    // return volts * 1000;
}

/**
 * Getting the millivolts from the settled inputs
 * @return A double (32bits) containing the ADC value in millivolts
 */
double ADS1118::getMilliVolts()
{
    // Leggi il canale attualmente configurato dal chip invece di usare il registro locale
    ads1118_channel_t current_channel;
    uint8_t res = ads1118_get_channel(&gs_handle, &current_channel);
    if (res != 0) {
        // Se fallisce, usa il registro locale come fallback
        return getMilliVolts(configRegister.bits.mux);
    }
    return getMilliVolts(current_channel);
    // if (configRegister.bits.sensorMode != ADC_MODE) {
    //     configRegister.bits.sensorMode = ADC_MODE;
    //     ads1118_set_mode(&gs_handle, configRegister.bits.sensorMode);
    // }
    // float volts;
    // float fsr = pgaFSR[configRegister.bits.pga];
    // uint16_t value;
    // value = getADCValue(configRegister.bits.mux);
    // if (value >= 0x8000) {
    //     value = ((~value) + 1); // Applying binary twos complement format
    //     volts = ((float)(value * fsr / 32768) * -1);
    // } else {
    //     volts = (float)(value * fsr / 32768);
    // }
    // return volts * 1000;
}

/**
 * Getting the temperature in degrees celsius from the internal sensor of the ADS1118
 * @return A double (32bits) containing the temperature in degrees celsius of the internal sensor
 */
double ADS1118::getTemperature()
{
    if (configRegister.bits.sensorMode != TEMP_MODE) {
        configRegister.bits.sensorMode = TEMP_MODE;
        ads1118_set_mode(&gs_handle, configRegister.bits.sensorMode);
    }

    int16_t raw;
    float deg;

    uint8_t res;
    if (configRegister.bits.operatingMode == SINGLE_SHOT) {
        if ((res = ads1118_single_read(&gs_handle, &raw, &deg)) != 0) {
            Serial.println("getTemperature single read failed with error: ");
            // throw std::runtime_error("getTemperature single read failed with error: " + std::to_string(res));
        }
    } else {
        if ((res = ads1118_continuous_read(&gs_handle, &raw, &deg)) != 0) {
            Serial.println("getTemperature continuous read failed with error: ");
            // throw std::runtime_error("getTemperature continuous read failed with error: " + std::to_string(res));
        }
    }

    if (ads1118_temperature_convert(&gs_handle, raw, &deg) != 0) {
        Serial.println("getTemperature temperature convert failed with error: ");
        // throw std::runtime_error("getTemperature temperature convert failed with error: " + std::to_string(res));
    }
    return deg;
}

/**
 * Setting the sampling rate specified in the config register
 * @param samplingRate It's the sampling rate: RATE_8SPS, RATE_16SPS, RATE_32SPS, RATE_64SPS, RATE_128SPS, RATE_250SPS,
 * RATE_475SPS, RATE_860SPS
 */
uint8_t ADS1118::setSamplingRate(ads1118_rate_t samplingRate)
{
    configRegister.bits.rate = samplingRate;
    Serial.print("SetSamplingRate: ");
    Serial.println(samplingRate);
    return ads1118_set_rate(&gs_handle, configRegister.bits.rate);
}

/**
 * Setting the full scale range in the config register
 * @param fsr The full scale range: FSR_6144 (±6.144V)*, FSR_4096(±4.096V)*, FSR_2048(±2.048V), FSR_1024(±1.024V),
 * FSR_0512(±0.512V), FSR_0256(±0.256V). (*) No more than VDD + 0.3 V must be applied to this device.
 */
uint8_t ADS1118::setFullScaleRange(ads1118_range_t fsr)
{
    configRegister.bits.pga = fsr;
    Serial.print("SetFullScaleRange: ");
    Serial.println(fsr);
    return ads1118_set_range(&gs_handle, configRegister.bits.pga);
}

/**
 * Setting the inputs to be adquired in the config register.
 * @param input The input selected: Diferential inputs: DIFF_0_1, DIFF_0_3, DIFF_1_3, DIFF_2_3. Single ended input: AIN_0, AIN_1,
 * AIN_2, AIN_3
 */
uint8_t ADS1118::setInputSelected(ads1118_channel_t input)
{
    ads1118_channel_t current_channel;
    ads1118_get_channel(&gs_handle, &current_channel);
    Serial.print("Current channel: ");
    Serial.println(current_channel);

    // Se siamo in modalità continuous, fermala temporaneamente
    bool was_continuous = (configRegister.bits.operatingMode == CONTINUOUS);
    if (was_continuous) {
        ads1118_stop_continuous_read(&gs_handle);
        delay(10);
    }

    uint8_t res = ads1118_set_channel(&gs_handle, input);
    if (res == 0) {
        // Aggiorna il registro locale solo se il comando è andato a buon fine
        configRegister.bits.mux = input;
        Serial.print("SetInputSelected: ");
        Serial.print(input);
        Serial.print(" result: ");
        Serial.println(res);

        // Se eravamo in modalità continuous, riavviala
        if (was_continuous) {
            delay(10);
            ads1118_start_continuous_read(&gs_handle);
        }
    }
    return res;
}

/**
 * Setting to continuous adquisition mode
 */
uint8_t ADS1118::setContinuousMode()
{

    configRegister.bits.operatingMode = CONTINUOUS;
    Serial.println("setContinuousMode");
    return ads1118_start_continuous_read(&gs_handle);
}

/**
 * Setting to single shot adquisition and power down mode
 */
uint8_t ADS1118::setSingleShotMode()
{
    configRegister.bits.operatingMode = SINGLE_SHOT;
    Serial.println("setSingleShotMode");
    return ads1118_stop_continuous_read(&gs_handle);
}

/**
 * Disabling the internal pull-up resistor of the DOUT pin
 */
uint8_t ADS1118::disablePullup()
{
    configRegister.bits.operatingMode = DOUT_NO_PULLUP;
    return ads1118_set_dout_pull_up(&gs_handle, configRegister.bits.operatingMode);
}

/**
 * Enabling the internal pull-up resistor of the DOUT pin
 */
uint8_t ADS1118::enablePullup()
{
    configRegister.bits.operatingMode = DOUT_PULLUP;
    return ads1118_set_dout_pull_up(&gs_handle, configRegister.bits.operatingMode);
}

/**
 * Decoding a configRegister structure and then print it out to the Serial port
 * @param configRegister The config register in "union Config" format
 */
void ADS1118::decodeConfigRegister(union Config configRegister)
{
    // Array di stringhe per ogni campo
    const char *singleStartStr[] = {"NOINI", "START"};
    const char *muxStr[] = {"A0-A1", "A0-A3", "A1-A3", "A2-A3", "A0-GD", "A1-GD", "A2-GD", "A3-GD"};
    const char *pgaStr[] = {"6.144", "4.096", "2.048", "1.024", "0.512", "0.256", "0.256", "0.256"};
    const char *operatingModeStr[] = {"CONT.", "SSHOT"};
    const char *rateStr[] = {"8 SPS", "16SPS", "32SPS", "64SPS", "128SP", "250SP", "475SP", "860SP"};
    const char *sensorModeStr[] = {"ADC_M", "TMP_M"};
    const char *pullUpStr[] = {"DISAB", "ENABL"};
    const char *noOperationStr[] = {"INVAL", "VALID", "INVAL", "INVAL"};
    const char *reservedStr[] = {"RSRV0", "RSRV1"};

    // Costruzione del messaggio usando gli array
    String message =
        String(singleStartStr[configRegister.bits.singleStart]) + " " + String(muxStr[configRegister.bits.mux]) + " " +
        String(pgaStr[configRegister.bits.pga]) + " " + String(operatingModeStr[configRegister.bits.operatingMode]) + " " +
        String(rateStr[configRegister.bits.rate]) + " " + String(sensorModeStr[configRegister.bits.sensorMode]) + " " +
        String(pullUpStr[configRegister.bits.pullUp]) + " " + String(noOperationStr[configRegister.bits.noOperation]) + " " +
        String(reservedStr[configRegister.bits.reserved]);

    // Stampa header e messaggio
    Serial.println("\nSTART MXSEL PGASL MODES RATES ADTMP PLLUP NOOPE RESER");
    Serial.println(message);
}

// Implementazioni dei metodi di istanza per l'interfaccia SPI
uint8_t ADS1118::spi_init_impl(void)
{
#if defined(ESP32)
    if (pSpi) {
        pSpi->begin();
        pinMode(cs, OUTPUT);
        digitalWrite(cs, HIGH);
        return 0; // Successo
    }
#endif
    return 1; // Errore
}

uint8_t ADS1118::spi_deinit_impl(void)
{
#if defined(ESP32)
    if (pSpi) {
        pSpi->end();
        return 0; // Successo
    }
#endif
    return 1; // Errore
}

uint8_t ADS1118::spi_transmit_impl(uint8_t *tx, uint8_t *rx, uint16_t len)
{
#if defined(ESP32)
    if (pSpi) {
        digitalWrite(cs, LOW);
        pSpi->beginTransaction(SPISettings(SCLK, MSBFIRST, SPI_MODE1));

        for (uint16_t i = 0; i < len; i++) {
            rx[i] = pSpi->transfer(tx[i]);
        }

        pSpi->endTransaction();
        digitalWrite(cs, HIGH);
        return 0; // Successo
    }
#endif
    return 1; // Errore
}

void ADS1118::delay_ms_impl(uint32_t ms)
{
    delay(ms);
}

void ADS1118::debug_print_impl(const char *const fmt, ...)
{
    // Implementazione semplice per debug - può essere migliorata
    Serial.print("ADS1118 Debug: ");
    Serial.println(fmt);
}

// Implementazioni delle funzioni wrapper statiche per il driver ADS1118
uint8_t ADS1118::spi_init_wrapper(void)
{
    if (current_instance) {
        return current_instance->spi_init_impl();
    }
    return 1; // Errore se non c'è istanza corrente
}

uint8_t ADS1118::spi_deinit_wrapper(void)
{
    if (current_instance) {
        return current_instance->spi_deinit_impl();
    }
    return 1; // Errore se non c'è istanza corrente
}

uint8_t ADS1118::spi_transmit_wrapper(uint8_t *tx, uint8_t *rx, uint16_t len)
{
    if (current_instance) {
        return current_instance->spi_transmit_impl(tx, rx, len);
    }
    return 1; // Errore se non c'è istanza corrente
}

void ADS1118::delay_ms_wrapper(uint32_t ms)
{
    if (current_instance) {
        current_instance->delay_ms_impl(ms);
    }
}

void ADS1118::debug_print_wrapper(const char *const fmt, ...)
{
    if (current_instance) {
        current_instance->debug_print_impl(fmt);
    }
}