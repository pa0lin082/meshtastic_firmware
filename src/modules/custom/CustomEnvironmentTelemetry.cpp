#include "CustomEnvironmentTelemetry.h"
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

// Dichiarazioni delle variabili globali necessarie
extern Router *router;
extern MeshService *service;

CustomEnvironmentTelemetry *customEnvironmentTelemetry;
// Helper function to convert sensor type enum to string
const char *CustomEnvironmentTelemetry::getSensorTypeName(meshtastic_TelemetrySensorType sensorType)
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

CustomEnvironmentTelemetry::CustomEnvironmentTelemetry() {}

CustomEnvironmentTelemetry::~CustomEnvironmentTelemetry()
{
    LOG_INFO("CustomEnvironmentTelemetry: distrutto\n");
}

bool CustomEnvironmentTelemetry::init()
{
    LOG_INFO("CustomEnvironmentTelemetry: Inizializzazione modulo");

    if (bh1750Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: BH1750 sensor found");
        uint32_t result = bh1750Sensor.runOnce();
        sensorFound = true;
    }

    if (bme280Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: BME280 sensor found");
        uint32_t result = bme280Sensor.runOnce();
        sensorFound = true;
    }
    if (bmp280Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: BMP280 sensor found");
        uint32_t result = bmp280Sensor.runOnce();
        sensorFound = true;
    }
    if (sht31Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: SHT31 sensor found");
        uint32_t result = sht31Sensor.runOnce();
        sensorFound = true;
    }
    if (shtc3Sensor.hasSensor()) {
        LOG_INFO("CustomSensorModule: SHTC3 sensor found");
        uint32_t result = shtc3Sensor.runOnce();
        sensorFound = true;
    }

    initialized = true;
    return true;
}

void CustomEnvironmentTelemetry::sendEnvironmentTelemetry()
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

void CustomEnvironmentTelemetry::sendSensorFoundMesssage()
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
