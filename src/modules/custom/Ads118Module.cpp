#include "Ads118Module.h"
// #include "ADS1118SW.h"
#include "DebugConfiguration.h"
#include "MeshService.h"
#include "Router.h"
#include "sleep.h"
#include <ADS1118.h>
#include <Arduino.h>
#include <SPI.h>

// Pin SPI per ADS1118 (Software SPI)
#define ADS1118_SCLK 35        // Clock
#define ADS1118_MOSI 36        // Master Out Slave In
#define ADS1118_MISO 37        // Master In Slave Out
#define ADS1118_CS 38          // Chip Select
#define ADS1118_MOSI SUBSPID   // Master Out Slave In
#define ADS1118_MISO SUBSPIQ   // Master In Slave Out
#define ADS1118_SCLK SUBSPICLK // Clock
#define ADS1118_CS SUBSPICS0   // Chip Select

// Costanti per la configurazione ADS1118
#define ADS1118_GAIN_6_144V 0
#define ADS1118_GAIN_4_096V 1
#define ADS1118_GAIN_2_048V 2
#define ADS1118_GAIN_1_024V 3
#define ADS1118_GAIN_0_512V 4
#define ADS1118_GAIN_0_256V 5

#define ADS1118_DR_8SPS 0
#define ADS1118_DR_16SPS 1
#define ADS1118_DR_32SPS 2
#define ADS1118_DR_64SPS 3
#define ADS1118_DR_128SPS 4
#define ADS1118_DR_250SPS 5
#define ADS1118_DR_475SPS 6
#define ADS1118_DR_860SPS 7

// Dichiarazioni delle variabili globali necessarie
extern Router *router;
extern MeshService *service;

Ads118Module *ads118Module;

Ads118Module::Ads118Module()
    : concurrency::OSThread("Ads118Module"), initialized(false), lastSentToMesh(0), _mosi(ADS1118_MOSI), _miso(ADS1118_MISO),
      _sclk(ADS1118_SCLK), _cs(ADS1118_CS), _gain(ADS1118_GAIN_4_096V), _dataRate(ADS1118_DR_128SPS)
{
    LOG_INFO("Ads118Module: Inizializzazione modulo ADS1118");

    if (initSPI() && initADS1118()) {
        LOG_INFO("Ads118Module: ADS1118 inizializzato con successo");
        initialized = true;
    } else {
        LOG_ERROR("Ads118Module: Errore nell'inizializzazione dell'ADS1118");
        initialized = false;
    }
}

Ads118Module::~Ads118Module()
{
    LOG_INFO("Ads118Module: Modulo ADS1118 distrutto");
}

void Ads118Module::setup()
{
    LOG_INFO("Ads118Module: setup() => Inizializzazione modulo ADS1118");
}

bool Ads118Module::initSPI()
{
    LOG_INFO("Ads118Module: Inizializzazione SPI sui pin JTAG...");

    spi = new SPIClass(HSPI);
    spi->begin(_sclk, _miso, _mosi, _cs);
    spi->setFrequency(4000000);

    // Configura un dispositivo SPI (esempio: sensore)
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH); // CS alto = dispositivo disabilitato

    LOG_INFO("Ads118Module: Pin SPI configurati - MOSI:%d, MISO:%d, SCLK:%d, CS:%d", _mosi, _miso, _sclk, _cs);

    // Test di funzionamento SPI
    if (testSPICommunication()) {
        LOG_INFO("Ads118Module: Test SPI completato con successo");
        return true;
    } else {
        LOG_ERROR("Ads118Module: Test SPI fallito");
        return false;
    }
}

bool Ads118Module::initADS1118()
{
    LOG_INFO("Ads118Module: Inizializzazione ADS1118...");
    // ads1118 = new ADS1118(ADS1118_CS, ADS1118_SCLK, ADS1118_MISO, ADS1118_MOSI);
    ads1118 = new ADS1118(ADS1118_CS, spi);
    ads1118->begin();
    // // Test di comunicazione con l'ADS1118
    // if (!) {
    //     LOG_ERROR("Ads118Module: Errore nell'inizializzazione dell'ADS1118");
    //     return false;
    // }

    LOG_INFO("Ads118Module: ADS1118.begin() completato con successo");

    const uint8_t samplingRate = ads1118->RATE_8SPS;
    const uint8_t fullScaleRange = ads1118->FSR_6144; // Cambiato a 4.096V per test

    /* Changing the sampling rate.
Available values: RATE_8SPS, RATE_16SPS, RATE_32SPS, RATE_64SPS, RATE_128SPS, RATE_250SPS, RATE_475SPS, RATE_860SPS */
    ads1118->setSamplingRate(samplingRate); // Using the setter method to change the sampling rate
                                            // ads1118->configRegister.bits.rate=ads1118->RATE_8SPS;   //Driving the
                                            // config register directly. Uncomment if you want to use this way
    /* Changing the full scale range.
       Available values: FSR_6144 (±6.144V)*, FSR_4096(±4.096V)*, FSR_2048(±2.048V), FSR_1024(±1.024V), FSR_0512(±0.512V),
       FSR_0256(±0.256V).
       (*) No more than VDD + 0.3 V must be applied to this device. */
    ads1118->setFullScaleRange(fullScaleRange);
    ads1118->enablePullup();
    ads1118->setContinuousMode();

    LOG_INFO("Ads118Module: ADS1118 configurato - Sampling Rate: %d, Full Scale Range: %d", samplingRate, fullScaleRange);

    // Test di comunicazione per verificare se il sensore risponde
    if (testADS1118Connection()) {
        LOG_INFO("Ads118Module: Test di connessione ADS1118 completato con successo");
        return true;
    } else {
        LOG_ERROR("Ads118Module: Test di connessione ADS1118 fallito");
        return false;
    }
}

bool Ads118Module::testSPICommunication()
{
    LOG_INFO("Ads118Module: Esecuzione test SPI sui pin JTAG...");

    // Test 1: Verifica configurazione pin
    LOG_INFO("Ads118Module: Test 1 - Verifica configurazione pin");
    LOG_INFO("Ads118Module: MOSI=%d, MISO=%d, SCLK=%d, CS=%d", _mosi, _miso, _sclk, _cs);

    // Test 2: Test di clock SPI
    LOG_INFO("Ads118Module: Test 2 - Test clock SPI");
    for (int i = 0; i < 5; i++) {
        digitalWrite(_sclk, HIGH);
        delayMicroseconds(10);
        digitalWrite(_sclk, LOW);
        delayMicroseconds(10);
    }

    // Test 3: Test chip select
    LOG_INFO("Ads118Module: Test 3 - Test chip select");
    digitalWrite(_cs, LOW);
    delayMicroseconds(10);
    digitalWrite(_cs, HIGH);
    delayMicroseconds(10);

    // Test 4: Test con oscilloscopio/logic analyzer
    LOG_INFO("Ads118Module: Test 4 - Pattern di test per oscilloscopio");
    for (int pattern = 0; pattern < 3; pattern++) {
        digitalWrite(_cs, LOW);
        for (int bit = 0; bit < 8; bit++) {
            digitalWrite(_mosi, (pattern % 2) ? HIGH : LOW);
            digitalWrite(_sclk, HIGH);
            delayMicroseconds(50);
            digitalWrite(_sclk, LOW);
            delayMicroseconds(50);
        }
        digitalWrite(_cs, HIGH);
        delay(100);
    }

    LOG_INFO("Ads118Module: Test SPI completato - Controlla con oscilloscopio/logic analyzer");
    return true;
}

bool Ads118Module::testADS1118Connection()
{
    LOG_INFO("Ads118Module: Esecuzione test di connessione ADS1118...");

    // Test 1: Verifica temperatura interna
    LOG_INFO("Ads118Module: Test 1 - Lettura temperatura interna");
    const double temperature = ads1118->getTemperature();
    LOG_INFO("Ads118Module: Temperatura interna: %f °C", temperature);

    if (temperature == 0.0) {
        LOG_WARN("Ads118Module: Temperatura interna = 0, possibile problema di connessione");
    } else if (temperature < -40.0 || temperature > 125.0) {
        LOG_WARN("Ads118Module: Temperatura interna fuori range (-40°C to 125°C): %f", temperature);
    } else {
        LOG_INFO("Ads118Module: Temperatura interna nel range normale: %f °C", temperature);
    }

    // Test 2: Verifica letture su tutti i canali
    LOG_INFO("Ads118Module: Test 2 - Lettura tutti i canali");
    const uint8_t inputs[] = {ads1118->AIN_0, ads1118->AIN_1, ads1118->AIN_2, ads1118->AIN_3};
    bool anyChannelActive = false;

    for (int i = 0; i < 4; i++) {
        // ads1118->setInputSelected(inputs[i]);
        delay(100); // Aspetta stabilizzazione

        const double milliVolts = ads1118->getMilliVolts(inputs[i]);
        LOG_INFO("Ads118Module: Canale AIN_%d: %f mV", i, milliVolts);

        if (milliVolts != 0.0) {
            anyChannelActive = true;
            LOG_INFO("Ads118Module: Canale AIN_%d attivo: %f mV", i, milliVolts);
        }
    }

    // Test 3: Test con tensione di riferimento (se disponibile)
    LOG_INFO("Ads118Module: Test 3 - Test con tensione di riferimento");

    // Prova a leggere la tensione di alimentazione (se il sensore supporta questa funzione)
    // Questo è un test aggiuntivo per verificare se il sensore risponde

    // Test 4: Verifica configurazione
    // LOG_INFO("Ads118Module: Test 4 - Verifica configurazione");
    // LOG_INFO("Ads118Module: Sampling Rate attuale: %d", ads1118->getSamplingRate());
    // LOG_INFO("Ads118Module: Full Scale Range attuale: %d", ads1118->getFullScaleRange());

    // Valutazione finale
    if (temperature != 0.0 || anyChannelActive) {
        LOG_INFO("Ads118Module: Test di connessione PASSATO - Sensore risponde");
        return true;
    } else {
        LOG_ERROR("Ads118Module: Test di connessione FALLITO - Nessuna risposta dal sensore");
        LOG_ERROR("Ads118Module: Possibili cause:");
        LOG_ERROR("Ads118Module: 1. Connessioni SPI errate");
        LOG_ERROR("Ads118Module: 2. Alimentazione non presente");
        LOG_ERROR("Ads118Module: 3. Sensore danneggiato");
        LOG_ERROR("Ads118Module: 4. Pin CS non collegato correttamente");
        return true;
    }
}

void Ads118Module::sendADS1118Telemetry()
{
    if (!initialized) {
        LOG_ERROR("Ads118Module: Modulo non inizializzato");
        return;
    }

    // Legge tutti e 4 i canali dell'ADS1118
    float voltages[4];
    int16_t rawValues[4];

    for (int channel = 0; channel < 4; channel++) {
        voltages[channel] = readChannel(channel);
        rawValues[channel] = readChannelRaw(channel);
        LOG_INFO("Ads118Module: Channel %d: %.6f V (raw: %d)", channel, voltages[channel], rawValues[channel]);
    }

    // Crea il pacchetto di telemetria
    meshtastic_MeshPacket *p = router->allocForSending();
    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

    // Creazione di un oggetto JSON strutturato
    JSONObject jsonObj;
    jsonObj["type"] = new JSONValue("ads1118_metrics");

    // Creazione dell'array di metriche
    JSONArray metricsArray;

    // Aggiungi le metriche per ogni canale
    for (int channel = 0; channel < 4; channel++) {
        JSONObject voltageMetric;
        voltageMetric["name"] = new JSONValue("ads1118_ch" + String(channel) + "_voltage");
        voltageMetric["value"] = new JSONValue(voltages[channel]);
        voltageMetric["unit"] = new JSONValue("voltage");
        voltageMetric["channel"] = new JSONValue(channel);

        metricsArray.push_back(new JSONValue(voltageMetric));

        JSONObject rawMetric;
        rawMetric["name"] = new JSONValue("ads1118_ch" + String(channel) + "_raw");
        rawMetric["value"] = new JSONValue(rawValues[channel]);
        rawMetric["unit"] = new JSONValue("raw");
        rawMetric["channel"] = new JSONValue(channel);

        metricsArray.push_back(new JSONValue(rawMetric));
    }

    // Aggiungi l'array delle metriche all'oggetto principale
    jsonObj["metrics"] = new JSONValue(metricsArray);

    // Converti l'oggetto JSON in una stringa
    JSONValue *jsonValue = new JSONValue(jsonObj);
    std::string jsonData = jsonValue->Stringify();
    LOG_INFO("Ads118Module: JSON generato: %s", jsonData.c_str());
    delete jsonValue;

    memcpy(p->decoded.payload.bytes, jsonData.c_str(), jsonData.length());
    p->decoded.payload.size = jsonData.length();
    p->to = NODENUM_BROADCAST;
    p->decoded.want_response = false;
    p->priority = meshtastic_MeshPacket_Priority_RELIABLE;
    service->sendToMesh(p, RX_SRC_LOCAL);
}

int32_t Ads118Module::runOnce()
{
    if (!initialized) {
        LOG_INFO("Ads118Module: Non inizializzato, riprova dopo 5 secondi");
        return 5000;
    }

    if (ads1118 != NULL) {
        const uint8_t inputs[] = {ads1118->AIN_0, ads1118->AIN_1, ads1118->AIN_2, ads1118->AIN_3}; // AIN_0, AIN_1, AIN_2, AIN_3
        const double temperature = ads1118->getTemperature();
        LOG_INFO("Ads118Module: Temperature: %f", temperature);
        for (int i = 0; i < 4; i++) {
            delay(100);
            ads1118->setInputSelected(inputs[i]);
            const double milliVolts = ads1118->getMilliVolts();
            delay(100);
            const double milliVolts2 = ads1118->getMilliVolts(4 + i);
            LOG_INFO("Ads118Module: Input AIN_%d, MilliVolts: %f MilliVolts2: %f", i, milliVolts, milliVolts2);

            // double milliVoltsNoWait;
            // const bool success = ads1118->getMilliVoltsNoWait(inputs[i], milliVoltsNoWait);
            // if (success) {
            //     LOG_INFO("Ads118Module: Input AIN_%d, MilliVoltsNoWait: %f", i, milliVoltsNoWait);
            // } else {
            //     LOG_ERROR("Ads118Module: Input AIN_%d, MilliVoltsNoWait: %f", i, milliVoltsNoWait);
            // }
        }
    } else {
        LOG_ERROR("Ads1118Module: ADS1118 non inizializzato");
    }

    // Invia telemetria ogni 30 secondi
    if (lastSentToMesh == 0 || (millis() - lastSentToMesh) >= 30000) {
        lastSentToMesh = millis();
        // sendADS1118Telemetry();
        LOG_INFO("Ads118Module: Telemetria ADS1118 inviata");
    }

    // blinkPin(ADS1118_SCLK, 4, 1000);
    // blinkPin(ADS1118_MOSI, 4, 1000);
    // blinkPin(ADS1118_MISO, 4, 1000);
    // blinkPin(ADS1118_CS, 4, 1000);

    return 5000; // Controlla ogni secondo
}

void Ads118Module::blinkPin(int pin, int count, int delayTime)
{
    pinMode(pin, OUTPUT);

    for (int i = 0; i < count; i++) {
        digitalWrite(pin, LOW);
        LOG_INFO("Ads118Module: Pin %d settato a LOW", pin);
        delay(delayTime);
        digitalWrite(pin, HIGH);
        delay(1000);
        LOG_INFO("Ads118Module: Pin %d settato a HIGH", pin);
    }
    digitalWrite(pin, LOW);
}