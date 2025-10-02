#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && __has_include(<ClosedCube_OPT3001.h>)

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "BH1750Sensor.h"
#include "TelemetrySensor.h"
#include <BH1750_WE.h>

BH1750Sensor::BH1750Sensor() : TelemetrySensor(meshtastic_TelemetrySensorType_BH1750, "BH1750") {}

int32_t BH1750Sensor::runOnce()
{
    LOG_INFO("Init sensor: %s", sensorName);
    if (!hasSensor()) {
        LOG_ERROR("BH1750 !hasSensor");
        return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
    }
    LOG_INFO("BH1750 hasSensor at 0x%x", nodeTelemetrySensorsMap[sensorType].first);
    bh1750 = BH1750_WE(nodeTelemetrySensorsMap[sensorType].second, nodeTelemetrySensorsMap[sensorType].first);
    status = bh1750.init();
    LOG_INFO("BH1750 status: %d", status);

    float lightIntensity = bh1750.getLux();
    LOG_INFO("BH1750 light intensity: %f", lightIntensity);

    return initI2CSensor();
}

void BH1750Sensor::setup()
{
    // OPT3001_Config newConfig;

    // newConfig.RangeNumber = 0b1100;
    // newConfig.ConvertionTime = 0b0;
    // newConfig.Latch = 0b1;
    // newConfig.ModeOfConversionOperation = 0b11;

    // OPT3001_ErrorCode errorConfig = opt3001.writeConfig(newConfig);
    // if (errorConfig != NO_ERROR) {
    //     LOG_ERROR("OPT3001 configuration error #%d", errorConfig);
    // }
}

bool BH1750Sensor::getMetrics(meshtastic_Telemetry *measurement)
{
    measurement->variant.environment_metrics.has_lux = true;
    float result = bh1750.getLux();

    measurement->variant.environment_metrics.lux = result;
    LOG_INFO("Lux: %f", measurement->variant.environment_metrics.lux);

    return true;
}

#endif