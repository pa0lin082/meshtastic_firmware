#include "CustomSensorModule.h"
#include "CustomMetricsSender.h"
// #include "DHT.h"
#include "DebugConfiguration.h"
#include "Default.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "RTC.h"
#include "RadioLibInterface.h"
#include "Router.h"
#include "configuration.h"
#include "detect/ScanI2CTwoWire.h"
// #include "driver/adc.h"
// #include "esp32-hal-adc.h"
// #include "esp_adc_cal.h"
// #include "hal/gpio_types.h"
#include "main.h"
#include "mesh/generated/meshtastic/portnums.pb.h"
#include "mesh/generated/meshtastic/telemetry.pb.h"
#include "meshtastic/mesh.pb.h"
#include "meshtastic/telemetry.pb.h"
#include "serialization/JSON.h"
#include "sleep.h"
#include <Arduino.h>
#include <Wire.h>
#include "modules/Telemetry/EnvironmentTelemetry.h"


#define MAGIC_USB_BATTERY_LEVEL 101

// Dichiarazioni delle variabili globali necessarie
extern Router *router;
extern MeshService *service;
#if HAS_SCREEN
extern graphics::Screen *screen;
#endif // HAS_SCREEN

// Default sleep time se CUSTOM_SENSOR_MODULE_SLEEP_TIME non è definito
#ifndef CUSTOM_SENSOR_MODULE_SLEEP_TIME
#define CUSTOM_SENSOR_MODULE_SLEEP_TIME 10 * 60 * 1000; // 2 secondi di default
#endif

#ifndef CUSTOM_SENSOR_MODULE_WAIT_FIRST_TIME
#define CUSTOM_SENSOR_MODULE_WAIT_FIRST_TIME 0 * 1000; // 2 secondi di default
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
    : concurrency::OSThread("CustomSensorModule"), initialized(false)
{
    initialized = true;
}

CustomSensorModule::~CustomSensorModule()
{
    LOG_INFO("CustomSensorModule: Modulo ADC distrutto\n");
}

void CustomSensorModule::setup() {
  LOG_INFO("CustomSensorModule: setup\n");
    if (!getSensors().empty()) {
        LOG_INFO("Sensor is not empty\n");
    }

    for (TelemetrySensor *sensor : getSensors()) {
        LOG_INFO("CustomEnvironmentTelemetryModule: sensor->sensorName => %s", sensor->sensorName);
    }

}





bool CustomSensorModule::sendEnvironmentTelemetry()
{

    bool valid = true;
    bool hasSensor = false;
    meshtastic_Telemetry m = meshtastic_Telemetry_init_zero;
    m.which_variant = meshtastic_Telemetry_environment_metrics_tag;
    m.time = getTime();
    m.variant.environment_metrics = meshtastic_EnvironmentMetrics_init_zero;

    for (TelemetrySensor *sensor : getSensors()) {
        valid = valid && sensor->getMetrics(&m);
        hasSensor = true;
    }

    if(!hasSensor) {
        LOG_ERROR("CustomSensorModule: No sensor found");
        return false;
    }
    if(!valid) {
        LOG_ERROR("CustomSensorModule: No valid sensor found");
        return false;
    }
    // if (!customEnvironmentTelemetryModule) {
    //     // LOG_ERROR("CustomSensorModule: EnvironmentTelemetryModule non inizializzato\n");
    //     return false;
    // }

    // if (!customEnvironmentTelemetryModule->getEnvironmentTelemetry(&m)) {
    //     // LOG_ERROR("CustomSensorModule: Non riesco a ottenere i dati dell'EnvironmentTelemetry\n");
    //     // LOG_INFO("Check: barometric_pressure=%f, current=%f, gas_resistance=%f, "
    //     //     "relative_humidity=%f, temperature=%f , lux=%f",
    //     //     m.variant.environment_metrics.barometric_pressure, m.variant.environment_metrics.current,
    //     //     m.variant.environment_metrics.gas_resistance, m.variant.environment_metrics.relative_humidity,
    //     //     m.variant.environment_metrics.temperature, m.variant.environment_metrics.lux);


    //     return false;
    // }

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
    return true;
}

bool CustomSensorModule::sendDeviceTelemetry()
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

    return true;
}

void CustomSensorModule::sendSensorFoundMessage()
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

    // LOG_INFO("TestModule: Scritto contatore %d sul display", remainingTime);
}
#endif // HAS_SCREEN

int32_t CustomSensorModule::runOnce()
{

    for (TelemetrySensor *sensor : getSensors()) {
        sensor->runOnce();
    }

    // Se è la prima esecuzione dopo l'inizializzazione, salva il timestamp
    if (firstExecutionTime == 0) {
        setup();
        firstExecutionTime = millis();
        LOG_INFO("CustomSensorModule: Prima esecuzione MinActiveTime: %dms, SleepTime: %dms", MIN_ACTIVE_TIME, SLEEP_TIME / 1000);

        sendSensorFoundMessage();
        return CUSTOM_SENSOR_MODULE_WAIT_FIRST_TIME;
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

      if (sendEnvironmentTelemetry() && sendDeviceTelemetry()) {
        lastSentToMesh = millis();
      }
    }

    return 100;
}
