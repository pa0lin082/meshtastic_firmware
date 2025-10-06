#pragma once
#include "GPS.h"
#include "GPSStatus.h"
#include "concurrency/OSThread.h"

extern meshtastic::GPSStatus *gpsStatus;
extern GPS *gps;

class TestModule : private concurrency::OSThread
{
  public:
    TestModule() : concurrency::OSThread("TestModule")
    {
        LOG_INFO("TestModule: Inizializzazione modulo Test");
        initialized = true;
    };

    ~TestModule() { LOG_INFO("TestModule: Distruzione modulo Test"); };

    void setup() { LOG_INFO("TestModule: setup() => Configurazione modulo Test"); }

  protected:
    virtual int32_t runOnce() override
    {
        LOG_INFO("TestModule: GPS Stato Lock: %d", gpsStatus->getHasLock());
        LOG_INFO("TestModule: GPS Latitudine: %d", gpsStatus->getLatitude());
        LOG_INFO("TestModule: GPS Longitudine: %d", gpsStatus->getLongitude());
        LOG_INFO("TestModule: GPS Altitudine: %d", gpsStatus->getAltitude());
        LOG_INFO("TestModule: GPS DOP: %d", gpsStatus->getDOP());
        LOG_INFO("TestModule: GPS Heading: %d", gpsStatus->getHeading());
        LOG_INFO("TestModule: GPS Numero Satelliti: %d", gpsStatus->getNumSatellites());
        LOG_INFO("TestModule: GPS Ultimo Fix: %d", gpsStatus->getLastFixMillis());
        LOG_INFO("TestModule: GPS Stato Potenza: %d", gpsStatus->getIsPowerSaving());

        gps->up();
        LOG_INFO("TestModule: GPS svegliato");
        LOG_INFO("TestModule: GPS Stato Lock: %d", gps->hasLock());
        LOG_INFO("TestModule: GPS Connesso: %d", gps->isConnected());
        return 1000;
    };

  private:
    bool initialized = false;
};

TestModule *testModule = nullptr;
