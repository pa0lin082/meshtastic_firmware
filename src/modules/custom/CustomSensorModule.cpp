#include "CustomSensorModule.h"
#include "DHT.h"
#include "DebugConfiguration.h"
#include "Default.h"
#include "MeshService.h"
#include "RTC.h"
#include "RadioLibInterface.h"
#include "Router.h"
#include "configuration.h"
#include "detect/ScanI2CTwoWire.h"
#include "driver/adc.h"
#include "esp32-hal-adc.h"
#include "esp_adc_cal.h"
#include "hal/gpio_types.h"
#include "main.h"
#include "mesh/generated/meshtastic/portnums.pb.h"
#include "mesh/generated/meshtastic/telemetry.pb.h"
#include "meshtastic/mesh.pb.h"
#include "meshtastic/telemetry.pb.h"
#include "serialization/JSON.h"
#include "sleep.h"
#include <Arduino.h>
#include <Wire.h>

#include "modules/Telemetry/Sensor/nullSensor.h"
#if __has_include(<Adafruit_BME280.h>)
#include "modules/Telemetry/Sensor/BME280Sensor.h"
extern BME280Sensor bme280Sensor;
#else
NullSensor bme280Sensor;
#endif

#if __has_include(<bsec2.h>)
#include "modules/Telemetry/Sensor/BME680Sensor.h"
extern BME680Sensor bme680Sensor;
#else
NullSensor bme680Sensor;
#endif

#if __has_include(<BH1750_WE.h>)
#include "modules/Telemetry/Sensor/BH1750Sensor.h"
extern BH1750Sensor bh1750Sensor;
#else
NullSensor bh1750Sensor;
#endif

#if __has_include(<Adafruit_SHT31.h>)
#include "modules/Telemetry/Sensor/SHT31Sensor.h"
extern SHT31Sensor sht31Sensor;
#else
NullSensor sht31Sensor;
#endif

#if __has_include(<Adafruit_SHTC3.h>)
#include "modules/Telemetry/Sensor/SHTC3Sensor.h"
extern SHTC3Sensor shtc3Sensor;
#else
NullSensor shtc3Sensor;
#endif

#if __has_include(<Adafruit_BMP280.h>)
#include "modules/Telemetry/Sensor/BMP280Sensor.h"
extern BMP280Sensor bmp280Sensor;
#else
NullSensor bmp280Sensor;
#endif

#define MAGIC_USB_BATTERY_LEVEL 101

#include "Adafruit_BME680.h"
#include <Adafruit_Sensor.h>
#include <BH1750_WE.h>
#include <DS3231.h>
#include <Wire.h>

DS3231 myRTC(Wire1);

// BME280 sarà gestito come membro della classe

// Dichiarazioni delle variabili globali necessarie
extern Router *router;
extern MeshService *service;
#if HAS_SCREEN
extern graphics::Screen *screen;
#endif // HAS_SCREEN

// Pin ADC1_H6 - questo è tipicamente il pin GPIO 7 su ESP32

#define ADC_Ctrl 37
#define ADC_Pin 6

// Pin DHT - configurabile da file di configurazione esterno
#define DHT_Pin 5
#define DHTTYPE DHT11

// Default sleep time se CUSTOM_SENSOR_MODULE_SLEEP_TIME non è definito
#ifndef CUSTOM_SENSOR_MODULE_SLEEP_TIME
#define CUSTOM_SENSOR_MODULE_SLEEP_TIME 10 * 60 * 1000; // 2 secondi di default
#endif

#ifndef CUSTOM_SENSOR_MODULE_MIN_ACTIVE_TIME
#define CUSTOM_SENSOR_MODULE_MIN_ACTIVE_TIME 5;
#endif

#ifndef CUSTOM_SENSOR_SEND_SENSOR_FOUND_MESSAGE
#define CUSTOM_SENSOR_SEND_SENSOR_FOUND_MESSAGE 0
#endif

const static int SLEEP_TIME = CUSTOM_SENSOR_MODULE_SLEEP_TIME;           // 5 minuti in millisecondi
const static int MIN_ACTIVE_TIME = CUSTOM_SENSOR_MODULE_MIN_ACTIVE_TIME; // 0.25 minuti in millisecondi
const static int MIN_SEND_INTERVAL = 60000;                              // 60 secondi in millisecondi
const int SAMPLES = 100;

Adafruit_BME680 bme(&Wire1); // I2C

#define BH1750_ADDRESS 0x23
#define BH1750_ADDRESS_ALT 0x5C

CustomSensorModule *customSensorModule;

// Helper function to convert sensor type enum to string
const char *getSensorTypeName(meshtastic_TelemetrySensorType sensorType)
{
    switch (sensorType) {
    case meshtastic_TelemetrySensorType_SENSOR_UNSET:
        return "SENSOR_UNSET";
    case meshtastic_TelemetrySensorType_BME280:
        return "BME280";
    case meshtastic_TelemetrySensorType_BME680:
        return "BME680";
    case meshtastic_TelemetrySensorType_MCP9808:
        return "MCP9808";
    case meshtastic_TelemetrySensorType_INA260:
        return "INA260";
    case meshtastic_TelemetrySensorType_INA219:
        return "INA219";
    case meshtastic_TelemetrySensorType_BMP280:
        return "BMP280";
    case meshtastic_TelemetrySensorType_SHTC3:
        return "SHTC3";
    case meshtastic_TelemetrySensorType_LPS22:
        return "LPS22";
    case meshtastic_TelemetrySensorType_QMC6310:
        return "QMC6310";
    case meshtastic_TelemetrySensorType_QMI8658:
        return "QMI8658";
    case meshtastic_TelemetrySensorType_QMC5883L:
        return "QMC5883L";
    case meshtastic_TelemetrySensorType_SHT31:
        return "SHT31";
    case meshtastic_TelemetrySensorType_PMSA003I:
        return "PMSA003I";
    case meshtastic_TelemetrySensorType_INA3221:
        return "INA3221";
    case meshtastic_TelemetrySensorType_BMP085:
        return "BMP085";
    case meshtastic_TelemetrySensorType_RCWL9620:
        return "RCWL9620";
    case meshtastic_TelemetrySensorType_SHT4X:
        return "SHT4X";
    case meshtastic_TelemetrySensorType_VEML7700:
        return "VEML7700";
    case meshtastic_TelemetrySensorType_MLX90632:
        return "MLX90632";
    case meshtastic_TelemetrySensorType_OPT3001:
        return "OPT3001";
    case meshtastic_TelemetrySensorType_LTR390UV:
        return "LTR390UV";
    case meshtastic_TelemetrySensorType_TSL25911FN:
        return "TSL25911FN";
    case meshtastic_TelemetrySensorType_AHT10:
        return "AHT10";
    case meshtastic_TelemetrySensorType_DFROBOT_LARK:
        return "DFROBOT_LARK";
    case meshtastic_TelemetrySensorType_NAU7802:
        return "NAU7802";
    case meshtastic_TelemetrySensorType_BMP3XX:
        return "BMP3XX";
    case meshtastic_TelemetrySensorType_ICM20948:
        return "ICM20948";
    case meshtastic_TelemetrySensorType_MAX17048:
        return "MAX17048";
    case meshtastic_TelemetrySensorType_CUSTOM_SENSOR:
        return "CUSTOM_SENSOR";
    case meshtastic_TelemetrySensorType_MAX30102:
        return "MAX30102";
    case meshtastic_TelemetrySensorType_MLX90614:
        return "MLX90614";
    case meshtastic_TelemetrySensorType_SCD4X:
        return "SCD4X";
    case meshtastic_TelemetrySensorType_RADSENS:
        return "RADSENS";
    case meshtastic_TelemetrySensorType_INA226:
        return "INA226";
    case meshtastic_TelemetrySensorType_DFROBOT_RAIN:
        return "DFROBOT_RAIN";
    case meshtastic_TelemetrySensorType_DPS310:
        return "DPS310";
    case meshtastic_TelemetrySensorType_RAK12035:
        return "RAK12035";
    case meshtastic_TelemetrySensorType_PCT2075:
        return "PCT2075";
    case meshtastic_TelemetrySensorType_ADS1X15:
        return "ADS1X15";
    case meshtastic_TelemetrySensorType_ADS1X15_ALT:
        return "ADS1X15_ALT";
    case meshtastic_TelemetrySensorType_SFA30:
        return "SFA30";
    case meshtastic_TelemetrySensorType_SEN5X:
        return "SEN5X";
    case meshtastic_TelemetrySensorType_TSL2561:
        return "TSL2561";
    case meshtastic_TelemetrySensorType_BH1750:
        return "BH1750";
    default:
        return "UNKNOWN";
    }
}

CustomSensorModule::CustomSensorModule()
    : concurrency::OSThread("CustomSensorModule"), initialized(false), dht(nullptr), adcPin(ADC_Pin)
{
    LOG_INFO("CustomSensorModule: Inizializzazione modulo ADC per pin %d\n", adcPin);

    //    i2cScanner->exists(ScanI2C::DeviceType::RX8130CE_RTC);

    // Inizializza il DHT
    dht = new DHT(DHT_Pin, DHTTYPE);
    dht->begin();

    if (bme.begin()) {
        LOG_INFO("Could not find a valid BME680 sensor, check wiring!");

        bool century = false;
        bool h12Flag;
        bool pmFlag;
        Serial.print("2");
        if (century) { // Won't need this for 89 years.
            Serial.print("1");
        } else {
            Serial.print("0");
        }
        Serial.print(myRTC.getYear(), DEC);
        Serial.print(' ');

        // then the month
        Serial.print(myRTC.getMonth(century), DEC);
        Serial.print(" ");

        // then the date
        Serial.print(myRTC.getDate(), DEC);
        Serial.print(" ");

        // and the day of the week
        Serial.print(myRTC.getDoW(), DEC);
        Serial.print(" ");

        // Finally the hour, minute, and second
        Serial.print(myRTC.getHour(h12Flag, pmFlag), DEC);
        Serial.print(" ");
        Serial.print(myRTC.getMinute(), DEC);
        Serial.print(" ");
        Serial.print(myRTC.getSecond(), DEC);
        // Add AM/PM indicator
        if (h12Flag) {
            if (pmFlag) {
                Serial.print(" PM ");
            } else {
                Serial.print(" AM ");
            }
        } else {
            Serial.print(" 24h ");
        }

        // Display the temperature
        Serial.print("T=");
        Serial.print(myRTC.getTemperature(), 2);

        // Tell whether the time is (likely to be) valid
        if (myRTC.oscillatorCheck()) {
            Serial.print(" O+");
        } else {
            Serial.print(" O-");
        }

        // Indicate whether an alarm went off
        if (myRTC.checkIfAlarm(1)) {
            Serial.print(" A1!");
        }

        if (myRTC.checkIfAlarm(2)) {
            Serial.print(" A2!");
        }
        while (1)
            ;
    }

    // Inizializza BME280
    // initBME280();
    if (bh1750Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: BH1750 sensor found");
        uint32_t result = bh1750Sensor.runOnce();
        LOG_INFO("CustomSensorModule: BH1750 sensor result: %d", result);
    } else {
        LOG_INFO("CustomSensorModule: BH1750 sensor not found");
    }

    if (bme280Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: BME280 sensor found");

        uint32_t result = bme280Sensor.runOnce();
        LOG_INFO("CustomSensorModule: BME280 sensor result: %d", result);

    } else {
        LOG_INFO("CustomSensorModule: BME280 sensor not found");
    }
    if (bmp280Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: BMP280 sensor found");
        uint32_t result = bmp280Sensor.runOnce();
        LOG_INFO("CustomSensorModule: BMP280 sensor result: %d", result);
    } else {
        LOG_INFO("CustomSensorModule: BMP280 sensor not found");
    }
    if (sht31Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: SHT31 sensor found");
        uint32_t result = sht31Sensor.runOnce();
        LOG_INFO("CustomSensorModule: SHT31 sensor result: %d", result);
    } else {
        LOG_INFO("CustomSensorModule: SHT31 sensor not found");
    }
    if (shtc3Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: SHTC3 sensor found");
        uint32_t result = shtc3Sensor.runOnce();
        LOG_INFO("CustomSensorModule: SHTC3 sensor result: %d", result);
    } else {
        LOG_INFO("CustomSensorModule: SHTC3 sensor not found");
    }

    // Inizializza il pin ADC
    if (initADC()) {
        LOG_INFO("CustomSensorModule: Pin ADC %d inizializzato con successo\n", adcPin);
        initialized = true;
    } else {
        LOG_ERROR("CustomSensorModule: Errore nell'inizializzazione del pin ADC %d\n", adcPin);
        initialized = false;
    }
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

    meshtastic_Telemetry m = meshtastic_Telemetry_init_zero;
    m.which_variant = meshtastic_Telemetry_environment_metrics_tag;
    m.time = getTime();
    m.variant.environment_metrics = meshtastic_EnvironmentMetrics_init_zero;

    if (bh1750Sensor.hasSensor()) {
        bh1750Sensor.getMetrics(&m);
    }

    if (bme280Sensor.hasSensor()) {
        bme280Sensor.getMetrics(&m);
    } else if (bme680Sensor.hasSensor()) {
        bme680Sensor.getMetrics(&m);
    } else if (bmp280Sensor.hasSensor()) {
        bmp280Sensor.getMetrics(&m);
    } else if (sht31Sensor.hasSensor()) {
        sht31Sensor.getMetrics(&m);
    } else if (shtc3Sensor.hasSensor()) {
        shtc3Sensor.getMetrics(&m);
    } else {
        LOG_ERROR("CustomSensorModule: No BME sensor found");
        return;
    }
    LOG_INFO("Send: barometric_pressure=%f, current=%f, gas_resistance=%f, "
             "relative_humidity=%f, temperature=%f , lux=%f",
             m.variant.environment_metrics.barometric_pressure, m.variant.environment_metrics.current,
             m.variant.environment_metrics.gas_resistance, m.variant.environment_metrics.relative_humidity,
             m.variant.environment_metrics.temperature, m.variant.environment_metrics.lux);

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

void CustomSensorModule::sendSensorFoundMesssage()
{
#if CUSTOM_SENSOR_SEND_SENSOR_FOUND_MESSAGE == 0
    return;
#endif

    meshtastic_MeshPacket *p = router->allocForSending();
    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

    char *message = new char[1000];
    sprintf(message, "Sensori trovati:");

    LOG_INFO("Sensori trovati:");
    for (int i = 0; i < _meshtastic_TelemetrySensorType_MAX + 1; i++) {
        if (nodeTelemetrySensorsMap[i].first > 0) {
            const char *sensorName = getSensorTypeName((meshtastic_TelemetrySensorType)i);
            LOG_INFO("Sensore %s (tipo %d) trovato all'indirizzo 0x%02X", sensorName, i, nodeTelemetrySensorsMap[i].first);

            sprintf(message, "Sensore %s (tipo %d) trovato all'indirizzo 0x%02X", sensorName, i,
                    nodeTelemetrySensorsMap[i].first);
        }
    }

    memcpy(p->decoded.payload.bytes, message, strlen(message));
    p->decoded.payload.size = strlen(message);
    LOG_INFO("Send SensorFoundMesssage packet to mesh");
    service->sendToMesh(p, RX_SRC_LOCAL);
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

#if HAS_SCREEN
void CustomSensorModule::writeToDisplay()
{
    // Verifica se il display è disponibile
    if (!screen || !screen->getDisplayDevice()) {
        LOG_WARN("TestModule: Display non disponibile");
        return;
    }

    uint32_t timeSinceFirstExecution = millis() - firstExecutionTime;
    uint32_t remainingTime = MIN_ACTIVE_TIME - timeSinceFirstExecution;
    // char *bannerMsg = "%d";
    // snprintf(bannerMsg, sizeof(bannerMsg), " c:%d", counter);
    // screen->showSimpleBanner(bannerMsg, 1000);
    // screen->showOverlayBanner(bannerMsg, 1000);

    OLEDDisplay *display = screen->getDisplayDevice();

    // Pulisce il display
    display->clear();

    // Imposta il colore del testo
    display->setColor(OLEDDISPLAY_COLOR::WHITE);
    display->setTextAlignment(TEXT_ALIGN_CENTER);

    // Scrive il titolo
    display->setFont(ArialMT_Plain_16);
    display->drawString(display->width() / 2, 10, "Test Counter");

    // Scrive il numero incrementale
    display->setFont(ArialMT_Plain_24);
    char counterStr[20];
    snprintf(counterStr, sizeof(counterStr), "%d", remainingTime);
    display->drawString(display->width() / 2, 0, counterStr);

    // Aggiunge informazioni aggiuntive
    display->setFont(ArialMT_Plain_10);
    char infoStr[50];
    snprintf(infoStr, sizeof(infoStr), "Uptime: %d sec", millis() / 1000);
    display->drawString(display->width() / 2, 40, infoStr);

    // Aggiorna il display
    display->display();

    LOG_INFO("TestModule: Scritto contatore %d sul display", remainingTime);
}
#endif // HAS_SCREEN

int32_t CustomSensorModule::runOnce()
{

    // Se è la prima esecuzione dopo l'inizializzazione, salva il timestamp
    if (firstExecutionTime == 0) {
        firstExecutionTime = millis();
        LOG_INFO("CustomSensorModule: Prima esecuzione MinActiveTime: %dms, SleepTime: %dms", MIN_ACTIVE_TIME, SLEEP_TIME / 1000);

        sendSensorFoundMesssage();
    }
#if HAS_SCREEN
    writeToDisplay();
#endif // HAS_SCREEN
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
        // LOG_INFO("CustomSensorModule: Attendo altri %ums prima del deep sleep", remainingTime);
    } else {
        LOG_INFO("CustomSensorModule: Tempo minimo trascorso, programmo deep sleep");
        sleepOnNextExecution = true;
        firstExecutionTime = 0; // Reset per il prossimo ciclo
    }

    if (lastSentToMesh == 0 || (millis() - lastSentToMesh) >= MIN_SEND_INTERVAL) {
        lastSentToMesh = millis();
        sendEnvironmentTelemetry();
        sendDeviceTelemetry();
    }

    return 100;
}
