// /**
//  *  Arduino Library for Texas Instruments ADS1118 - 16-Bit Analog-to-Digital Converter with
//  *  Internal Reference and Temperature Sensor
//  *
//  *  @author Alvaro Salazar <alvaro@denkitronik.com>
//  *  http://www.denkitronik.com
//  *
//  */

// /**
//  * The MIT License
//  *
//  * Copyright 2018 Alvaro Salazar <alvaro@denkitronik.com>.
//  *
//  * Permission is hereby granted, free of charge, to any person obtaining a copy
//  * of this software and associated documentation files (the "Software"), to deal
//  * in the Software without restriction, including without limitation the rights
//  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  * copies of the Software, and to permit persons to whom the Software is
//  * furnished to do so, subject to the following conditions:
//  *
//  * The above copyright notice and this permission notice shall be included in
//  * all copies or substantial portions of the Software.
//  *
//  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  * THE SOFTWARE.
//  */
// #include "ADS1118SW.h"
// #include "Arduino.h"
// #include "DebugConfiguration.h"

// /**
//  * Debugging:
//  *   Uncomment some of the 3 lines above to debug the method you want.
//  * Warning:
//  *   Keep them commented in production
//  */
// // #define DEBUG_BEGIN  			//Debug begin() method
// // #define DEBUG_GETADCVALUE  	//Debug getADCValue() method
// // #define DEBUG_GETTEMPERATURE  //Debug getTemperature() method

// #ifdef DEBUG_BEGIN
// #define DEBUG_BEGIN(x) decodeConfigRegister(x)
// #else
// #define DEBUG_BEGIN(x)
// #endif

// #ifdef DEBUG_GETADCVALUE
// #define DEBUG_GETADCVALUE(x) decodeConfigRegister(x)
// #else
// #define DEBUG_GETADCVALUE(x)
// #endif

// #ifdef DEBUG_GETTEMPERATURE
// #define DEBUG_GETTEMPERATURE(x) decodeConfigRegister(x)
// #else
// #define DEBUG_GETTEMPERATURE(x)
// #endif

// #define BIT_ORDER MSBFIRST
// // #define BIT_ORDER LSBFIRST

// #if defined(__AVR__)
// /**
//  * Constructor of the class
//  * @param io_pin_cs a byte indicating the pin to be use as the chip select pin (CS)
//  */
// ADS1118::ADS1118(uint8_t io_pin_cs)
// {
//     cs = io_pin_cs;
// } ///< This method initialize the SPI port and the config register
// #elif defined(ESP32)
// /**
//  * Constructor of the class
//  * @param io_pin_cs a byte indicating the pin to be use as the chip select pin (CS)
//  */
// ADS1118::ADS1118(uint8_t io_pin_cs, uint8_t sclk, uint8_t miso, uint8_t mosi)
// {
//     cs = io_pin_cs;
//     pSpi = new SoftSPIB(mosi, miso, sclk);
//     LOG_INFO("ADS1118: Constructor SoftSPIB initialized with - CS:%d, SCLK:%d, MISO:%d, MOSI:%d", cs, sclk, miso, mosi);
// }
// #endif

// #if defined(__AVR__)
// /**
//  * This method initialize the SPI port and the config register
//  */
// void ADS1118::begin()
// {
//     pinMode(cs, OUTPUT);
//     digitalWrite(cs, HIGH);
//     SPI.begin();
//     SPI.beginTransaction(SPISettings(SCLK, BIT_ORDER, SPI_MODE1));
//     configRegister.bits = {RESERVED,    VALID_CFG, DOUT_PULLUP, ADC_MODE, RATE_8SPS,
//                            SINGLE_SHOT, FSR_0256,  DIFF_0_1,    START_NOW}; // Default values
//     DEBUG_BEGIN(configRegister); // Debug this method: print the config register in the Serial port
// } ///< This method initialize the SPI port and the config register
// #elif defined(ESP32)
// /**
//  * This method initialize the SPI port and the config register
//  */
// void ADS1118::begin()
// {
//     LOG_INFO("ADS1118: begin() - Initializing ADS1118");

//     pinMode(cs, OUTPUT);
//     digitalWrite(cs, HIGH);
//     LOG_INFO("ADS1118: CS pin %d configured as OUTPUT, set HIGH", cs);

//     pSpi->begin();
//     LOG_INFO("ADS1118: SoftSPIB begin() completed");

//     // Configurazione SPI
//     pSpi->setDataMode(SPI_MODE1);
//     pSpi->setBitOrder(MSBFIRST);
//     pSpi->setClockDivider(SPI_CLOCK_DIV16); // Velocità più stabile
//     LOG_INFO("ADS1118: SPI configured - Mode1, MSBFIRST, DIV16");

//     configRegister.bits = {0,           VALID_CFG, DOUT_PULLUP, ADC_MODE, RATE_8SPS,
//                            SINGLE_SHOT, FSR_4096,  AIN_0,       START_NOW}; // Default values

//     LOG_INFO("ADS1118: Config register initialized - MSB: 0x%02X, LSB: 0x%02X", configRegister.byte.msb,
//     configRegister.byte.lsb);

//     DEBUG_BEGIN(configRegister); // Debug this method: print the config register in the Serial port

//     // Test di connessione semplice
//     LOG_INFO("ADS1118: Testing connection...");
//     digitalWrite(cs, LOW);
//     delayMicroseconds(10);
//     uint16_t testWord = pSpi->transfer16(0x0000); // Invia 0x0000 e leggi risposta
//     digitalWrite(cs, HIGH);
//     LOG_INFO("ADS1118: Connection test - Sent 0x0000, received 0x%04X", testWord);

//     // Test con configurazione reale
//     LOG_INFO("ADS1118: Testing with real configuration...");
//     uint16_t configWord = (configRegister.byte.msb << 8) | configRegister.byte.lsb;
//     LOG_INFO("ADS1118: Sending config: 0x%04X", configWord);

//     digitalWrite(cs, LOW);
//     delayMicroseconds(10);
//     uint16_t response1 = pSpi->transfer16(configWord);
//     uint16_t response2 = pSpi->transfer16(configWord);
//     digitalWrite(cs, HIGH);

//     LOG_INFO("ADS1118: Config test - Response1: 0x%04X, Response2: 0x%04X", response1, response2);

//     // Test forzato con conversione - Modalità CONTINUA
//     LOG_INFO("ADS1118: Forced conversion test - CONTINUOUS MODE...");
//     configRegister.bits.singleStart = 0; // Non necessario in modalità continua
//     configRegister.bits.mux = AIN_0;
//     configRegister.bits.pga = FSR_4096;
//     configRegister.bits.rate = RATE_8SPS;
//     configRegister.bits.operatingMode = CONTINUOUS; // Modalità continua
//     configRegister.bits.sensorMode = ADC_MODE;
//     configRegister.bits.pullUp = DOUT_PULLUP;
//     configRegister.bits.noOperation = VALID_CFG;
//     configRegister.bits.reserved = 0;

//     uint16_t forcedConfig = (configRegister.byte.msb << 8) | configRegister.byte.lsb;
//     LOG_INFO("ADS1118: Forced config: 0x%04X", forcedConfig);

//     digitalWrite(cs, LOW);
//     delayMicroseconds(10);
//     uint16_t forced1 = pSpi->transfer16(forcedConfig);
//     digitalWrite(cs, HIGH);
//     delay(150); // Aspetta conversione completa

//     digitalWrite(cs, LOW);
//     delayMicroseconds(10);
//     uint16_t forced2 = pSpi->transfer16(forcedConfig);
//     digitalWrite(cs, HIGH);

//     LOG_INFO("ADS1118: Forced test - Response1: 0x%04X, Response2: 0x%04X", forced1, forced2);

//     // Test completo: lettura di tutti i canali e temperatura
//     LOG_INFO("ADS1118: Complete test - Reading all channels and temperature...");

//     // Test temperatura
//     LOG_INFO("ADS1118: Testing temperature reading...");
//     configRegister.bits.sensorMode = TEMP_MODE;
//     configRegister.bits.singleStart = START_NOW;
//     uint16_t tempConfig = (configRegister.byte.msb << 8) | configRegister.byte.lsb;

//     digitalWrite(cs, LOW);
//     delayMicroseconds(10);
//     uint16_t temp1 = pSpi->transfer16(tempConfig);
//     digitalWrite(cs, HIGH);
//     delay(150);

//     digitalWrite(cs, LOW);
//     delayMicroseconds(10);
//     uint16_t temp2 = pSpi->transfer16(tempConfig);
//     digitalWrite(cs, HIGH);

//     LOG_INFO("ADS1118: Temperature test - Response1: 0x%04X, Response2: 0x%04X", temp1, temp2);

//     // Test tutti i canali ADC
//     uint8_t channels[] = {AIN_0, AIN_1, AIN_2, AIN_3};
//     const char *channelNames[] = {"AIN_0", "AIN_1", "AIN_2", "AIN_3"};

//     configRegister.bits.sensorMode = ADC_MODE;

//     for (int i = 0; i < 4; i++) {
//         LOG_INFO("ADS1118: Testing channel %s...", channelNames[i]);
//         configRegister.bits.mux = channels[i];
//         configRegister.bits.singleStart = 0; // Modalità continua
//         configRegister.bits.operatingMode = CONTINUOUS;
//         uint16_t channelConfig = (configRegister.byte.msb << 8) | configRegister.byte.lsb;

//         digitalWrite(cs, LOW);
//         delayMicroseconds(10);
//         uint16_t ch1 = pSpi->transfer16(channelConfig);
//         digitalWrite(cs, HIGH);
//         delay(200); // Tempo più lungo per modalità continua

//         digitalWrite(cs, LOW);
//         delayMicroseconds(10);
//         uint16_t ch2 = pSpi->transfer16(channelConfig);
//         digitalWrite(cs, HIGH);

//         LOG_INFO("ADS1118: %s test - Response1: 0x%04X, Response2: 0x%04X", channelNames[i], ch1, ch2);

//         // Calcola millivolt se la conversione è valida
//         if (ch2 != 0xFFFF) {
//             int16_t rawValue = (int16_t)ch2;
//             if (rawValue >= 0x8000) {
//                 rawValue = ((~rawValue) + 1);
//                 double volts = ((float)(rawValue * 4.096 / 32768) * -1) * 1000;
//                 LOG_INFO("ADS1118: %s = %.3f mV (raw: %d)", channelNames[i], volts, rawValue);
//             } else {
//                 double volts = (float)(rawValue * 4.096 / 32768) * 1000;
//                 LOG_INFO("ADS1118: %s = %.3f mV (raw: %d)", channelNames[i], volts, rawValue);
//             }
//         } else {
//             LOG_WARN("ADS1118: %s = No valid conversion (0xFFFF)", channelNames[i]);
//         }
//     }

//     LOG_INFO("ADS1118: Complete test finished");

//     // Test hardware diretto - verifica se è davvero un ADS1118
//     LOG_INFO("ADS1118: Hardware verification test...");

//     // Test 1: Verifica che i pin siano configurati correttamente
//     LOG_INFO("ADS1118: Pin verification:");
//     LOG_INFO("ADS1118: CS=%d, SCLK=%d, MISO=%d, MOSI=%d", cs, 39, 41, 40);

//     // Test 2: Verifica alimentazione (se possibile)
//     LOG_INFO("ADS1118: Power supply test - sending multiple 0x0000...");
//     for (int i = 0; i < 5; i++) {
//         digitalWrite(cs, LOW);
//         delayMicroseconds(10);
//         uint16_t test = pSpi->transfer16(0x0000);
//         digitalWrite(cs, HIGH);
//         LOG_INFO("ADS1118: Test %d - Sent 0x0000, received 0x%04X", i, test);
//         delay(10);
//     }

//     // Test 3: Verifica con pattern diversi
//     LOG_INFO("ADS1118: Pattern test...");
//     uint16_t patterns[] = {0x0000, 0xFFFF, 0xAAAA, 0x5555, 0x1234};
//     for (int i = 0; i < 5; i++) {
//         digitalWrite(cs, LOW);
//         delayMicroseconds(10);
//         uint16_t result = pSpi->transfer16(patterns[i]);
//         digitalWrite(cs, HIGH);
//         LOG_INFO("ADS1118: Pattern %d - Sent 0x%04X, received 0x%04X", i, patterns[i], result);
//         delay(10);
//     }

//     // Test 4: Verifica se il chip risponde a comandi specifici ADS1118
//     LOG_INFO("ADS1118: ADS1118-specific command test...");

//     // Comando di reset (se supportato)
//     uint16_t resetCmd = 0x0000; // Comando di reset
//     digitalWrite(cs, LOW);
//     delayMicroseconds(10);
//     uint16_t resetResult = pSpi->transfer16(resetCmd);
//     digitalWrite(cs, HIGH);
//     delay(100); // Aspetta reset

//     LOG_INFO("ADS1118: Reset command - Sent 0x%04X, received 0x%04X", resetCmd, resetResult);

//     // Test dopo reset
//     digitalWrite(cs, LOW);
//     delayMicroseconds(10);
//     uint16_t afterReset = pSpi->transfer16(0x0000);
//     digitalWrite(cs, HIGH);
//     LOG_INFO("ADS1118: After reset - Sent 0x0000, received 0x%04X", afterReset);

//     LOG_INFO("ADS1118: Hardware verification completed");

//     // Test con timing molto lento - potrebbe essere un problema di velocità
//     LOG_INFO("ADS1118: Slow timing test - trying very slow SPI...");

//     // Configura SPI molto lento
//     pSpi->setClockDivider(SPI_CLOCK_DIV16); // Molto lento
//     LOG_INFO("ADS1118: SPI configured for very slow speed (DIV128)");

//     // Test con timing lento
//     digitalWrite(cs, LOW);
//     delay(10); // 10ms invece di 10μs
//     uint16_t slow1 = pSpi->transfer16(0x0000);
//     digitalWrite(cs, HIGH);
//     delay(10);

//     digitalWrite(cs, LOW);
//     delay(10);
//     uint16_t slow2 = pSpi->transfer16(0x0000);
//     digitalWrite(cs, HIGH);
//     delay(10);

//     LOG_INFO("ADS1118: Slow timing test - Response1: 0x%04X, Response2: 0x%04X", slow1, slow2);

//     // Test con configurazione ADS1118 molto semplice
//     LOG_INFO("ADS1118: Simple ADS1118 configuration test...");

//     // Configurazione minima per ADS1118
//     configRegister.bits = {0, VALID_CFG, DOUT_PULLUP, ADC_MODE, RATE_8SPS, SINGLE_SHOT, FSR_4096, AIN_0, START_NOW};

//     uint16_t simpleConfig = (configRegister.byte.msb << 8) | configRegister.byte.lsb;
//     LOG_INFO("ADS1118: Simple config: 0x%04X", simpleConfig);

//     // Test con timing molto lento
//     digitalWrite(cs, LOW);
//     delay(10);
//     uint16_t simple1 = pSpi->transfer16(simpleConfig);
//     digitalWrite(cs, HIGH);
//     delay(200); // Aspetta molto tempo per conversione

//     digitalWrite(cs, LOW);
//     delay(10);
//     uint16_t simple2 = pSpi->transfer16(simpleConfig);
//     digitalWrite(cs, HIGH);
//     delay(10);

//     LOG_INFO("ADS1118: Simple config test - Response1: 0x%04X, Response2: 0x%04X", simple1, simple2);

//     // Test con pull-up su MISO
//     LOG_INFO("ADS1118: Testing with MISO pull-up...");
//     pinMode(41, INPUT_PULLUP); // MISO con pull-up

//     digitalWrite(cs, LOW);
//     delay(10);
//     uint16_t pullup1 = pSpi->transfer16(0x0000);
//     digitalWrite(cs, HIGH);
//     delay(10);

//     LOG_INFO("ADS1118: Pull-up test - Response: 0x%04X", pullup1);

//     LOG_INFO("ADS1118: Slow timing test completed");
// }

// // void ADS1118::begin(uint8_t sclk, uint8_t miso, uint8_t mosi)
// // {
// //     pinMode(cs, OUTPUT);
// //     digitalWrite(cs, HIGH);
// //     pSpi->begin(sclk, miso, mosi, cs);
// //     configRegister.bits = {RESERVED,    VALID_CFG, DOUT_PULLUP, ADC_MODE, RATE_8SPS,
// //                            SINGLE_SHOT, FSR_0256,  DIFF_0_1,    START_NOW}; // Default values
// //     DEBUG_BEGIN(configRegister); // Debug this method: print the config register in the Serial port
// // }

// /**
//  * Getting a sample from the specified input if data is ready
//  * @param pin_drdy io pin connected to ADS1118 DOUT/DRDY. value Reference of ADC value to be fetched
//  * @return True if ADC data is ready
//  */
// bool ADS1118::getADCValueNoWait(uint8_t pin_drdy, uint16_t &value)
// {
//     // LOG_INFO("ADS1118: getADCValueNoWait() => pin_drdy:%d", pin_drdy);
//     byte dataMSB, dataLSB;
//     pSpi->setClockDivider(SPI_CLOCK_DIV16);
//     pSpi->setDataMode(SPI_MODE1);
//     pSpi->setBitOrder(BIT_ORDER);
//     // pSpi->beginTransaction(SPISettings(SCLK, BIT_ORDER, SPI_MODE1));
//     digitalWrite(cs, LOW);
//     if (digitalRead(pin_drdy)) {
//         digitalWrite(cs, HIGH);
//         // pSpi->end();
//         return false;
//     }

//     dataMSB = pSpi->transfer(configRegister.byte.msb);
//     dataLSB = pSpi->transfer(configRegister.byte.lsb);
//     digitalWrite(cs, HIGH);
//     // pSpi->end();

//     value = (dataMSB << 8) | (dataLSB);
//     return true;
// }

// /**
//  * Getting the millivolts from the settled inputs
//  * @return A double (32bits) containing the ADC value in millivolts
//  */
// bool ADS1118::getMilliVoltsNoWait(uint8_t pin_drdy, double &volts)
// {
//     float fsr = pgaFSR[configRegister.bits.pga];
//     uint16_t value;
//     bool dataReady = getADCValueNoWait(pin_drdy, value);
//     if (!dataReady)
//         return false;
//     if (value >= 0x8000) {
//         value = ((~value) + 1); // Applying binary twos complement format
//         volts = ((float)(value * fsr / 32768) * -1);
//     } else {
//         volts = (float)(value * fsr / 32768);
//     }
//     volts = volts * 1000;
//     return true;
// }
// #endif

// /**
//  * Getting a sample from the specified input
//  * @param inputs Sets the input of the ADC: Diferential inputs: DIFF_0_1, DIFF_0_3, DIFF_1_3, DIFF_2_3. Single ended input:
//  AIN_0,
//  * AIN_1, AIN_2, AIN_3
//  * @return A word containing the ADC value
//  */
// uint16_t ADS1118::getADCValue(uint8_t inputs)
// {
//     LOG_INFO("ADS1118: getADCValue() - Input: %d", inputs);

//     uint16_t value;
//     byte dataMSB, dataLSB, configMSB, configLSB, count = 0;

//     if (lastSensorMode == ADC_MODE)
//         count = 1;
//     else
//         configRegister.bits.sensorMode = ADC_MODE;

//     configRegister.bits.mux = inputs;

//     LOG_INFO("ADS1118: Config register - MSB: 0x%02X, LSB: 0x%02X", configRegister.byte.msb, configRegister.byte.lsb);

//     do {
//         // Configura SPI con timing lento come nel test che funziona
//         pSpi->setClockDivider(SPI_CLOCK_DIV16); // Timing lento come nel test
//         pSpi->setDataMode(SPI_MODE1);
//         pSpi->setBitOrder(BIT_ORDER);

//         LOG_INFO("ADS1118: Transfer %d - CS LOW", count);
//         digitalWrite(cs, LOW);
//         delay(10); // Usa delay(10) come nel test che funziona

//         // Usa transfer16() per trasferimenti più efficienti
//         uint16_t configWord = (configRegister.byte.msb << 8) | configRegister.byte.lsb;
//         uint16_t dataWord = pSpi->transfer16(configWord);
//         uint16_t configResponse = pSpi->transfer16(configWord);

//         // Estrai i byte
//         dataMSB = (dataWord >> 8) & 0xFF;
//         dataLSB = dataWord & 0xFF;
//         configMSB = (configResponse >> 8) & 0xFF;
//         configLSB = configResponse & 0xFF;

//         LOG_INFO("ADS1118: Transfer %d - Config sent: 0x%04X, Data received: 0x%04X, Config received: 0x%04X", count,
//         configWord,
//                  dataWord, configResponse);

//         digitalWrite(cs, HIGH);
//         delay(10); // Usa delay(10) come nel test che funziona

//         // Aspetta il tempo di conversione corretto (in millisecondi)
//         delay(CONV_TIME[configRegister.bits.rate]);
//         count++;
//     } while (count <= 1);

//     value = (dataMSB << 8) | (dataLSB);
//     LOG_INFO("ADS1118: Final value: %d (0x%04X)", value, value);
//     return value;

//     //     uint16_t value;
//     //     byte dataMSB, dataLSB, configMSB, configLSB, count = 0;
//     //     if (lastSensorMode == ADC_MODE) // Lucky you! We don't have to read twice the sensor
//     //         count = 1;
//     //     else
//     //         configRegister.bits.sensorMode = ADC_MODE; // Sorry but we will have to read twice the sensor
//     //     configRegister.bits.mux = inputs;
//     //     do {
//     // #if defined(ESP32)
//     //         //   pSpi->beginTransaction(SPISettings(SCLK, BIT_ORDER, SPI_MODE1));
//     //         pSpi->setClockDivider(SPI_CLOCK_DIV16);
//     //         pSpi->setDataMode(SPI_MODE1);
//     //         pSpi->setBitOrder(BIT_ORDER);
//     // #endif
//     //         digitalWrite(cs, LOW);
//     // #if defined(__AVR__)
//     //         dataMSB = SPI.transfer(configRegister.byte.msb);
//     //         dataLSB = SPI.transfer(configRegister.byte.lsb);
//     //         configMSB = SPI.transfer(configRegister.byte.msb);
//     //         configLSB = SPI.transfer(configRegister.byte.lsb);
//     // #elif defined(ESP32)
//     //         dataMSB = pSpi->transfer(configRegister.byte.msb);
//     //         dataLSB = pSpi->transfer(configRegister.byte.lsb);
//     //         configMSB = pSpi->transfer(configRegister.byte.msb);
//     //         configLSB = pSpi->transfer(configRegister.byte.lsb);
//     // #endif

//     //         digitalWrite(cs, HIGH);
//     // #if defined(ESP32)
//     //         // pSpi->end();
//     // #endif
//     //         for (int i = 0; i < CONV_TIME[configRegister.bits.rate]; i++) // Lets wait the conversion time
//     //             delayMicroseconds(1000);
//     //         count++;
//     //     } while (count <= 1);              // We make two readings because the second reading is the ADC conversion.
//     //     DEBUG_GETADCVALUE(configRegister); // Debug this method: print the config register in the Serial port
//     //     value = (dataMSB << 8) | (dataLSB);
//     //     return value;
// }

// /**
//  * Getting the millivolts from the specified inputs
//  * @param inputs Sets the inputs to be adquired. Diferential inputs: DIFF_0_1, DIFF_0_3, DIFF_1_3, DIFF_2_3. Single ended
//  input:
//  * AIN_0, AIN_1, AIN_2, AIN_3
//  * @return A double (32bits) containing the ADC value in millivolts
//  */
// double ADS1118::getMilliVolts(uint8_t inputs)
// {
//     // LOG_INFO("ADS1118: getMilliVolts() => inputs:%d", inputs);
//     float volts;
//     float fsr = pgaFSR[configRegister.bits.pga];
//     uint16_t value;
//     value = getADCValue(inputs);
//     if (value >= 0x8000) {
//         value = ((~value) + 1); // Applying binary twos complement format
//         volts = ((float)(value * fsr / 32768) * -1);
//     } else {
//         volts = (float)(value * fsr / 32768);
//     }
//     return volts * 1000;
// }

// /**
//  * Getting the millivolts from the settled inputs
//  * @return A double (32bits) containing the ADC value in millivolts
//  */
// double ADS1118::getMilliVolts()
// {
//     // LOG_INFO("ADS1118: getMilliVolts() input from configRegister.bits.mux:%d", configRegister.bits.mux);
//     float volts;
//     float fsr = pgaFSR[configRegister.bits.pga];
//     uint16_t value;
//     value = getADCValue(configRegister.bits.mux);
//     // LOG_INFO("ADS1118: getADCValue() result value:%d", value);
//     if (value >= 0x8000) {
//         value = ((~value) + 1); // Applying binary twos complement format
//         volts = ((float)(value * fsr / 32768) * -1);
//     } else {
//         volts = (float)(value * fsr / 32768);
//     }
//     return volts * 1000;
// }

// /**
//  * Getting the temperature in degrees celsius from the internal sensor of the ADS1118
//  * @return A double (32bits) containing the temperature in degrees celsius of the internal sensor
//  */
// double ADS1118::getTemperature()
// {
//     uint16_t convRegister;
//     uint8_t dataMSB, dataLSB, configMSB, configLSB, count = 0;
//     if (lastSensorMode == TEMP_MODE)
//         count = 1; // Lucky you! We don't have to read twice the sensor
//     else
//         configRegister.bits.sensorMode = TEMP_MODE; // Sorry but we will have to read twice the sensor
//     do {
// #if defined(ESP32)
//         //   pSpi->beginTransaction(SPISettings(SCLK, BIT_ORDER, SPI_MODE1));
//         pSpi->setClockDivider(SPI_CLOCK_DIV16);
//         pSpi->setDataMode(SPI_MODE1);
//         pSpi->setBitOrder(BIT_ORDER);
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
//         // pSpi->end();
// #endif
//         for (int i = 0; i < CONV_TIME[configRegister.bits.rate]; i++) // Lets wait the conversion time
//             delayMicroseconds(1000);
//         count++;
//     } while (count <= 1);                 // We make two readings because the second reading is the temperature.
//     DEBUG_GETTEMPERATURE(configRegister); // Debug this method: print the config register in the Serial port
//     convRegister = ((dataMSB << 8) | (dataLSB)) >> 2;
//     if ((convRegister << 2) >= 0x8000) {
//         convRegister = ((~convRegister) >> 2) + 1; // Converting to right-justified and applying binary twos complement format
//         return (double)(convRegister * 0.03125 * -1);
//     }
//     return (double)convRegister * 0.03125;
// }

// /**
//  * Setting the sampling rate specified in the config register
//  * @param samplingRate It's the sampling rate: RATE_8SPS, RATE_16SPS, RATE_32SPS, RATE_64SPS, RATE_128SPS, RATE_250SPS,
//  * RATE_475SPS, RATE_860SPS
//  */
// void ADS1118::setSamplingRate(uint8_t samplingRate)
// {
//     configRegister.bits.rate = samplingRate;
// }

// /**
//  * Setting the full scale range in the config register
//  * @param fsr The full scale range: FSR_6144 (±6.144V)*, FSR_4096(±4.096V)*, FSR_2048(±2.048V), FSR_1024(±1.024V),
//  * FSR_0512(±0.512V), FSR_0256(±0.256V). (*) No more than VDD + 0.3 V must be applied to this device.
//  */
// void ADS1118::setFullScaleRange(uint8_t fsr)
// {
//     configRegister.bits.pga = fsr;
// }

// /**
//  * Setting the inputs to be adquired in the config register.
//  * @param input The input selected: Diferential inputs: DIFF_0_1, DIFF_0_3, DIFF_1_3, DIFF_2_3. Single ended input: AIN_0,
//  AIN_1,
//  * AIN_2, AIN_3
//  */
// void ADS1118::setInputSelected(uint8_t input)
// {
//     configRegister.bits.mux = input;
// }

// /**
//  * Setting to continuous adquisition mode
//  */
// void ADS1118::setContinuousMode()
// {
//     configRegister.bits.operatingMode = CONTINUOUS;
// }

// /**
//  * Setting to single shot adquisition and power down mode
//  */
// void ADS1118::setSingleShotMode()
// {
//     configRegister.bits.operatingMode = SINGLE_SHOT;
// }

// /**
//  * Disabling the internal pull-up resistor of the DOUT pin
//  */
// void ADS1118::disablePullup()
// {
//     configRegister.bits.operatingMode = DOUT_NO_PULLUP;
// }

// /**
//  * Enabling the internal pull-up resistor of the DOUT pin
//  */
// void ADS1118::enablePullup()
// {
//     configRegister.bits.operatingMode = DOUT_PULLUP;
// }

// /**
//  * Decoding a configRegister structure and then print it out to the Serial port
//  * @param configRegister The config register in "union Config" format
//  */
// void ADS1118::decodeConfigRegister(union Config configRegister)
// {
//     String mensaje = String();
//     switch (configRegister.bits.singleStart) {
//     case 0:
//         mensaje = "NOINI";
//         break;
//     case 1:
//         mensaje = "START";
//         break;
//     }
//     mensaje += " ";
//     switch (configRegister.bits.mux) {
//     case 0:
//         mensaje += "A0-A1";
//         break;
//     case 1:
//         mensaje += "A0-A3";
//         break;
//     case 2:
//         mensaje += "A1-A3";
//         break;
//     case 3:
//         mensaje += "A2-A3";
//         break;
//     case 4:
//         mensaje += "A0-GD";
//         break;
//     case 5:
//         mensaje += "A1-GD";
//         break;
//     case 6:
//         mensaje += "A2-GD";
//         break;
//     case 7:
//         mensaje += "A3-GD";
//         break;
//     }
//     mensaje += " ";
//     switch (configRegister.bits.pga) {
//     case 0:
//         mensaje += "6.144";
//         break;
//     case 1:
//         mensaje += "4.096";
//         break;
//     case 2:
//         mensaje += "2.048";
//         break;
//     case 3:
//         mensaje += "1.024";
//         break;
//     case 4:
//         mensaje += "0.512";
//         break;
//     case 5:
//         mensaje += "0.256";
//         break;
//     case 6:
//         mensaje += "0.256";
//         break;
//     case 7:
//         mensaje += "0.256";
//         break;
//     }
//     mensaje += " ";
//     switch (configRegister.bits.operatingMode) {
//     case 0:
//         mensaje += "CONT.";
//         break;
//     case 1:
//         mensaje += "SSHOT";
//         break;
//     }
//     mensaje += " ";
//     switch (configRegister.bits.rate) {
//     case 0:
//         mensaje += "8 SPS";
//         break;
//     case 1:
//         mensaje += "16SPS";
//         break;
//     case 2:
//         mensaje += "32SPS";
//         break;
//     case 3:
//         mensaje += "64SPS";
//         break;
//     case 4:
//         mensaje += "128SP";
//         break;
//     case 5:
//         mensaje += "250SP";
//         break;
//     case 6:
//         mensaje += "475SP";
//         break;
//     case 7:
//         mensaje += "860SP";
//         break;
//     }
//     mensaje += " ";
//     switch (configRegister.bits.sensorMode) {
//     case 0:
//         mensaje += "ADC_M";
//         break;
//     case 1:
//         mensaje += "TMP_M";
//         break;
//     }
//     mensaje += " ";
//     switch (configRegister.bits.pullUp) {
//     case 0:
//         mensaje += "DISAB";
//         break;
//     case 1:
//         mensaje += "ENABL";
//         break;
//     }
//     mensaje += " ";
//     switch (configRegister.bits.noOperation) {
//     case 0:
//         mensaje += "INVAL";
//         break;
//     case 1:
//         mensaje += "VALID";
//         break;
//     case 2:
//         mensaje += "INVAL";
//         break;
//     case 3:
//         mensaje += "INVAL";
//         break;
//     }
//     mensaje += " ";
//     switch (configRegister.bits.reserved) {
//     case 0:
//         mensaje += "RSRV0";
//         break;
//     case 1:
//         mensaje += "RSRV1";
//         break;
//     }
//     Serial.println("\nSTART MXSEL PGASL MODES RATES ADTMP PLLUP NOOPE RESER");
//     Serial.println(mensaje);
// }