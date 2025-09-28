#include "CustomSensorModule.h"
#include "DHT.h"
#include "DebugConfiguration.h"
#include "Default.h"
#include "MeshService.h"
#include "RTC.h"
#include "RadioLibInterface.h"
#include "Router.h"
#include "configuration.h"
#include "driver/adc.h"
#include "esp32-hal-adc.h"
#include "esp_adc_cal.h"
#include "hal/gpio_types.h"
#include "main.h"
#include "mesh/generated/meshtastic/portnums.pb.h"
#include "meshtastic/mesh.pb.h"
#include "meshtastic/telemetry.pb.h"
#include "serialization/JSON.h"
#include "sleep.h"
#include <Arduino.h>

#include "modules/Telemetry/Sensor/nullSensor.h"
#if __has_include(<Adafruit_BME280.h>)
#include "modules/Telemetry/Sensor/BME280Sensor.h"
extern BME280Sensor bme280Sensor;
#else
NullSensor bme280Sensor;
#endif
#define MAGIC_USB_BATTERY_LEVEL 101

// BME280 sarà gestito come membro della classe

// Dichiarazioni delle variabili globali necessarie
extern Router *router;
extern MeshService *service;

// Pin ADC1_H6 - questo è tipicamente il pin GPIO 7 su ESP32

#define ADC_Ctrl 37
#define ADC_Pin 6

// Pin DHT - configurabile da file di configurazione esterno
#define DHT_Pin 5
#define DHTTYPE DHT11

const static int SLEEP_TIME = 10 * 60 * 1000;     // 5 minuti in millisecondi
const static int MIN_ACTIVE_TIME = 0 * 60 * 1000; // 0.25 minuti in millisecondi
const int SAMPLES = 100;

CustomSensorModule *customSensorModule;

CustomSensorModule::CustomSensorModule()
    : concurrency::OSThread("CustomSensorModule"), initialized(false), adcPin(ADC_Pin), dht(nullptr)
{
    LOG_INFO("CustomSensorModule: Inizializzazione modulo ADC per pin %d\n", adcPin);

    // Inizializza il DHT
    dht = new DHT(DHT_Pin, DHTTYPE);
    dht->begin();

    // Inizializza BME280
    // initBME280();

    if (bme280Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: BME280 sensor found");

        uint32_t result = bme280Sensor.runOnce();
        LOG_INFO("CustomSensorModule: BME280 sensor result: %d", result);

    } else {
        LOG_INFO("CustomSensorModule: BME280 sensor not found");
    }

    // Inizializza il pin ADC
    if (initADC()) {
        LOG_INFO("CustomSensorModule: Pin ADC %d inizializzato con successo\n", adcPin);
        initialized = true;
    } else {
        LOG_ERROR("CustomSensorModule: Errore nell'inizializzazione del pin ADC %d\n", adcPin);
        initialized = false;
    }
    LOG_INFO("CustomSensorModule: setIntervalFromNow 40*1000");
    // setIntervalFromNow( 40*1000); // Wait until NodeInfo is sent
}

CustomSensorModule::~CustomSensorModule()
{
    if (dht) {
        delete dht;
        dht = nullptr;
    }
    LOG_INFO("CustomSensorModule: Modulo ADC distrutto\n");
}

void CustomSensorModule::setup()
{
    LOG_INFO("CustomSensorModule: setup() => Inizializzazione modulo ADC\n");
}

bool CustomSensorModule::initADC()
{
    // Configura il pin come input analogico
    pinMode(adcPin, INPUT);
    // Disabilita eventuali pull-up/pull-down interni
    // Su ESP32, questo è gestito automaticamente per i pin analogici

    LOG_INFO("CustomSensorModule: Pin %d configurato come input analogico\n", adcPin);

    return true;
}

int CustomSensorModule::readADCValue()
{
    if (!initialized) {
        LOG_ERROR("CustomSensorModule: Modulo non inizializzato\n");
        return -1;
    }

    // Legge il valore analogico dal pin
    int value = analogReadMilliVolts(adcPin);

    return value;
}

void CustomSensorModule::sendDht11Telemetry()
{
    if (!dht) {
        LOG_ERROR("CustomSensorModule: DHT non inizializzato\n");
        return;
    }

    float h = dht->readHumidity();
    float t = dht->readTemperature();
    LOG_INFO("CustomSensorModule: Humidity: %.3f\n", h);
    LOG_INFO("CustomSensorModule: Temperature: %.3f\n", t);

    meshtastic_Telemetry m = meshtastic_Telemetry_init_zero;
    m.which_variant = meshtastic_Telemetry_environment_metrics_tag;
    m.variant.environment_metrics.has_relative_humidity = true;
    m.variant.environment_metrics.relative_humidity = h;
    m.variant.environment_metrics.has_temperature = true;
    m.variant.environment_metrics.temperature = t;

    meshtastic_MeshPacket *p = router->allocForSending();
    p->decoded.portnum = meshtastic_PortNum_TELEMETRY_APP;
    p->decoded.payload.size =
        pb_encode_to_bytes(p->decoded.payload.bytes, sizeof(p->decoded.payload.bytes), &meshtastic_Telemetry_msg, &m);
    p->to = NODENUM_BROADCAST;
    p->decoded.want_response = false;
    p->priority = meshtastic_MeshPacket_Priority_RELIABLE;
    service->sendToMesh(p, RX_SRC_LOCAL);
}

void CustomSensorModule::sendAdcTelemetry()
{
    // Legge il valore dal pin ADC

    // analogSetAttenuation(ADC_ATTENDB_MAX);
    // analogSetAttenuation(ADC_ATTEN_DB_11);
    digitalWrite(ADC_Ctrl, LOW);
    delay(100);

    int adcValue = 0;
    int adcRawValue = 0;
    float adcRawMilliVoltsValue = 0;

    for (int i = 0; i < SAMPLES; i++) {
        adcValue += analogRead(adcPin);
        adcRawValue += analogReadRaw(adcPin);
        adcRawMilliVoltsValue += analogReadMilliVolts(adcPin); // per ora migliori letture
        delayMicroseconds(500);                                // ~50Hz sampling
    }
    adcValue = adcValue / SAMPLES;
    adcRawValue = adcRawValue / SAMPLES;
    adcRawMilliVoltsValue = adcRawMilliVoltsValue / SAMPLES / 1000.0f;

    float voltage = (adcValue * 3.3) / 4095.0;
    float voltageRaw = (adcRawValue * 3.3) / 4095.0;
    float voltageRawMilliVolts = adcRawMilliVoltsValue;
    // delay(100);
    // //Characterize ADC at particular atten
    // esp_adc_cal_characteristics_t *adc_chars = calloc(1,
    // sizeof(esp_adc_cal_characteristics_t));
    // // esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten,
    // ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    // // //Check type of calibration value used to characterize ADC
    // // if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    // //     printf("eFuse Vref");
    // // } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    // //     printf("Two Point");
    // // } else {
    // //     printf("Default");
    // // }
    // uint32_t reading2 =  adc1_get_raw(ADC1_CHANNEL_5);
    // uint32_t voltage2 = esp_adc_cal_raw_to_voltage(reading2, adc_chars);

    digitalWrite(ADC_Ctrl, HIGH);

    if (adcValue >= 0) {
        meshtastic_MeshPacket *p = router->allocForSending();
        p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

        // Creazione di un oggetto JSON strutturato
        JSONObject jsonObj;
        jsonObj["type"] = new JSONValue("custom_metrics");

        // Creazione dell'array di metriche
        JSONArray metricsArray;

        // Creazione dell'oggetto metrica per voltage
        JSONObject voltageMetric;
        voltageMetric["name"] = new JSONValue("voltage_raw_milli_volts");
        voltageMetric["value"] = new JSONValue(voltageRawMilliVolts);
        voltageMetric["unit"] = new JSONValue("voltage");

        // Log per verificare la precisione
        LOG_INFO("CustomSensorModule: Valore float originale: %.6f", voltageRawMilliVolts);

        // Aggiungi la metrica all'array
        metricsArray.push_back(new JSONValue(voltageMetric));

        // Aggiungi l'array delle metriche all'oggetto principale
        jsonObj["metrics"] = new JSONValue(metricsArray);

        // Converti l'oggetto JSON in una stringa
        JSONValue *jsonValue = new JSONValue(jsonObj);
        std::string jsonData = jsonValue->Stringify();
        LOG_INFO("CustomSensorModule: JSON generato: %s", jsonData.c_str());
        delete jsonValue;

        memcpy(p->decoded.payload.bytes, jsonData.c_str(), jsonData.length());
        p->decoded.payload.size = jsonData.length();
        service->sendToMesh(p, RX_SRC_LOCAL);

        // Stampa il valore letto
        LOG_INFO("--------------------------------");
        LOG_INFO("CustomSensorModule: ADC       dal pin %d: %d   Voltage: %.3f V\n", adcPin, adcValue, voltage);
        LOG_INFO("CustomSensorModule: ADC (raw) dal pin %d: %d   Voltage: %.3f V\n", adcPin, adcRawValue, voltageRaw);
        LOG_INFO("CustomSensorModule: ADC (mv)  dal pin %d: %.3f   Voltage: %.3f V\n", adcPin, adcRawMilliVoltsValue,
                 voltageRawMilliVolts);
        // LOG_INFO("CustomSensorModule: reading: %d  voltage: %.3f V\n", reading2,
        // voltage2);

        // Opzionalmente, converte in voltaggio (per ESP32 con 3.3V di riferimento)
        // float voltage = (adcValue * 3.3) / 4095.0; // 12-bit ADC
        // LOG_INFO("CustomSensorModule: Voltaggio stimato: %.3f V\n", voltage);
    } else {
        LOG_ERROR("CustomSensorModule: Errore nella lettura del valore ADC\n");
    }
}

void CustomSensorModule::sendEnvironmentTelemetry()
{

    if (bme280Sensor.hasSensor()) {
        meshtastic_Telemetry m = meshtastic_Telemetry_init_zero;
        m.which_variant = meshtastic_Telemetry_environment_metrics_tag;
        m.time = getTime();
        m.variant.environment_metrics = meshtastic_EnvironmentMetrics_init_zero;
        bme280Sensor.getMetrics(&m);

        LOG_INFO("Send: barometric_pressure=%f, current=%f, gas_resistance=%f, "
                 "relative_humidity=%f, temperature=%f",
                 m.variant.environment_metrics.barometric_pressure, m.variant.environment_metrics.current,
                 m.variant.environment_metrics.gas_resistance, m.variant.environment_metrics.relative_humidity,
                 m.variant.environment_metrics.temperature);

        meshtastic_MeshPacket *p = router->allocForSending();
        p->to = NODENUM_BROADCAST;
        p->decoded.portnum = meshtastic_PortNum_TELEMETRY_APP;
        p->decoded.want_response = false;
        if (config.device.role == meshtastic_Config_DeviceConfig_Role_SENSOR)
            p->priority = meshtastic_MeshPacket_Priority_RELIABLE;
        else
            p->priority = meshtastic_MeshPacket_Priority_BACKGROUND;
        p->decoded.payload.size =
            pb_encode_to_bytes(p->decoded.payload.bytes, sizeof(p->decoded.payload.bytes), &meshtastic_Telemetry_msg, &m);
        LOG_INFO("Send EnvironmentTelemetry packet to mesh");
        service->sendToMesh(p, RX_SRC_LOCAL);
    }
}

void CustomSensorModule::sendDeviceTelemetry()
{
    meshtastic_Telemetry telemetry = getDeviceTelemetry();
    LOG_INFO("Send: air_util_tx=%f, channel_utilization=%f, battery_level=%i, "
             "voltage=%f, uptime=%i",
             telemetry.variant.device_metrics.air_util_tx, telemetry.variant.device_metrics.channel_utilization,
             telemetry.variant.device_metrics.battery_level, telemetry.variant.device_metrics.voltage,
             telemetry.variant.device_metrics.uptime_seconds);

    DEBUG_HEAP_BEFORE;
    meshtastic_MeshPacket *p = router->allocForSending();
    DEBUG_HEAP_AFTER("DeviceTelemetryModule::sendTelemetry", p);
    p->decoded.portnum = meshtastic_PortNum_TELEMETRY_APP;
    p->to = NODENUM_BROADCAST;
    p->decoded.want_response = false;
    p->priority = meshtastic_MeshPacket_Priority_BACKGROUND;

    nodeDB->updateTelemetry(nodeDB->getNodeNum(), telemetry, RX_SRC_LOCAL);
    p->decoded.payload.size =
        pb_encode_to_bytes(p->decoded.payload.bytes, sizeof(p->decoded.payload.bytes), &meshtastic_Telemetry_msg, &telemetry);
    LOG_INFO("Send DeviceTelemetry packet to mesh");
    service->sendToMesh(p, RX_SRC_LOCAL);

    return;
}

meshtastic_Telemetry CustomSensorModule::getDeviceTelemetry()
{
    meshtastic_Telemetry t = meshtastic_Telemetry_init_zero;
    t.which_variant = meshtastic_Telemetry_device_metrics_tag;
    t.time = getTime();
    t.variant.device_metrics = meshtastic_DeviceMetrics_init_zero;
    t.variant.device_metrics.has_air_util_tx = true;
    t.variant.device_metrics.has_battery_level = true;
    t.variant.device_metrics.has_channel_utilization = true;
    t.variant.device_metrics.has_voltage = true;
    t.variant.device_metrics.has_uptime_seconds = true;

    t.variant.device_metrics.air_util_tx = airTime->utilizationTXPercent();
    t.variant.device_metrics.battery_level = (!powerStatus->getHasBattery() || powerStatus->getIsCharging())
                                                 ? MAGIC_USB_BATTERY_LEVEL
                                                 : powerStatus->getBatteryChargePercent();
    t.variant.device_metrics.channel_utilization = airTime->channelUtilizationPercent();
    t.variant.device_metrics.voltage = powerStatus->getBatteryVoltageMv() / 1000.0;
    t.variant.device_metrics.uptime_seconds = getUptimeSeconds();

    return t;
}

int32_t CustomSensorModule::runOnce()
{

    // Se è la prima esecuzione dopo l'inizializzazione, salva il timestamp
    if (firstExecutionTime == 0) {
        firstExecutionTime = millis();
        LOG_INFO("CustomSensorModule: Prima esecuzione MinActiveTime: %dms, SleepTime: %dms", MIN_ACTIVE_TIME, SLEEP_TIME / 1000);
    }

    // Verifica se sono passati almeno 30 secondi dalla prima esecuzione
    uint32_t timeSinceFirstExecution = millis() - firstExecutionTime;

    if (sleepOnNextExecution == true) {
        sleepOnNextExecution = false;
        LOG_INFO("Sleep for %ims, then awake to send metrics again", SLEEP_TIME);
        doDeepSleep(SLEEP_TIME, true, false);
    }

    if (!initialized) {
        // Se non siamo inizializzati, riprova dopo 2 secondi
        LOG_INFO("CustomSensorModule: Non siamo inizializzati, riprova dopo 500 millisecondi");
        return 500;
    }

    // controllo stato e chiamata deep sleep
    if (!RadioLibInterface::instance->canSleep()) {
        LOG_INFO("CustomSensorModule: Radio canSleep: false, aspetto");
    } else if (lastSentToMesh == 0) {
        LOG_INFO("CustomSensorModule:Telemetry non inviati, aspetto");
    } else if (timeSinceFirstExecution < MIN_ACTIVE_TIME) {
        uint32_t remainingTime = MIN_ACTIVE_TIME - timeSinceFirstExecution;
        LOG_INFO("CustomSensorModule: Attendo altri %ums prima del deep sleep", remainingTime);
    } else {
        LOG_INFO("CustomSensorModule: Tempo minimo trascorso, programmo deep sleep");
        sleepOnNextExecution = true;
        firstExecutionTime = 0; // Reset per il prossimo ciclo
    }

    if (lastSentToMesh == 0 || (millis() - lastSentToMesh) >= 5000) {
        lastSentToMesh = millis();
        sendEnvironmentTelemetry();
        sendDeviceTelemetry();
    }

    return 100;
}
