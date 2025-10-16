#pragma once
#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "concurrency/OSThread.h"
#include "configuration.h"

#include "DHT.h"
#if HAS_SCREEN
#include "graphics/Screen.h"
#endif // HAS_SCREEN
#include <Adafruit_BME280.h>
#include <Wire.h>
/**
 * Modulo ADC per la lettura di valori analogici dal pin ADC1_H6.
 * Questo modulo legge periodicamente il valore dal pin ADC1_H6 e lo stampa nel log.
 * Include anche supporto per BME280 (temperatura, umidità, pressione).
 */
class CustomEnvironmentTelemetry
{
  public:
    /** Constructor */
    CustomEnvironmentTelemetry();
    /** Destructor */
    ~CustomEnvironmentTelemetry();

    bool hasSensor();
    /** Inizializza il modulo */
    bool init();

  private:
    bool initialized = false;
    bool sensorFound = false;
    const char *getSensorTypeName(meshtastic_TelemetrySensorType sensorType);

    void sendEnvironmentTelemetry();
    void sendSensorFoundMesssage();
};

extern CustomEnvironmentTelemetry *customEnvironmentTelemetry;
