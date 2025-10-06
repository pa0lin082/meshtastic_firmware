#pragma once
#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "concurrency/OSThread.h"

#include "DHT.h"
#include <Adafruit_BME280.h>
#include <Wire.h>
/**
 * Modulo ADC per la lettura di valori analogici dal pin ADC1_H6.
 * Questo modulo legge periodicamente il valore dal pin ADC1_H6 e lo stampa nel log.
 * Include anche supporto per BME280 (temperatura, umidità, pressione).
 */
class CustomSensorModule : private concurrency::OSThread
{
  public:
    /** Constructor */
    CustomSensorModule();

    /** Destructor */
    ~CustomSensorModule();

    /** Inizializza il modulo */
    void setup();

    /** Reset del sensore BH1750 */
    void resetBH1750();

    /** Forza un aggiornamento GPS dal modulo hardware */
    bool forceGPSUpdate();

    /** Test della funzione GPS - stampa informazioni sulla posizione corrente */
    void testGPSPosition();

  protected:
    /** Metodo principale del thread che viene chiamato periodicamente */
    virtual int32_t runOnce() override;

    uint32_t getUptimeSeconds() { return (millis() - firstExecutionTime / 1000); }

  private:
    bool initialized;
    uint32_t firstExecutionTime = 0;
    uint32_t lastSentToMesh = 0;

    DHT *dht = nullptr;
    int adcPin = -1;

    /** Inizializza il pin ADC */
    bool initADC();

    /** Legge il valore dal pin ADC */
    int readADCValue();

    meshtastic_Telemetry getDeviceTelemetry();
    meshtastic_Telemetry getLocalStatsTelemetry();
    /** Legge il valore del DHT */
    void sendDht11Telemetry();
    void sendAdcTelemetry();
    void sendEnvironmentTelemetry();
    void sendDeviceTelemetry();
};

extern CustomSensorModule *customSensorModule;
