#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && __has_include(<ClosedCube_OPT3001.h>)

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "BH1750Sensor.h"
#include "TelemetrySensor.h"
#include <BH1750_WE.h>

BH1750Sensor::BH1750Sensor() : TelemetrySensor(meshtastic_TelemetrySensorType_BH1750, "BH1750") {}

int32_t BH1750Sensor::runOnce()
{
    if (!hasSensor()) {
        return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
    }
    bh1750 = BH1750_WE(nodeTelemetrySensorsMap[sensorType].second, nodeTelemetrySensorsMap[sensorType].first);
    status = bh1750.init();

    return initI2CSensor();
}

void BH1750Sensor::setup()
{
    bh1750.setMode(CHM); // sets mode and starts measurement
}

bool BH1750Sensor::getMetrics(meshtastic_Telemetry *measurement)
{

    // bh1750.setMode(OTH);
    delay(140); // wait for measurement to be completed, change for OTL
    measurement->variant.environment_metrics.has_lux = true;
    float result = bh1750.getLux();

    measurement->variant.environment_metrics.lux = result;
    return true;
}

#endif