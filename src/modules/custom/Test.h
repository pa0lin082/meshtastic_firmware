#pragma once
#include "GPS.h"
#include "GPSStatus.h"
#include "OLEDDisplay.h"
#include "concurrency/OSThread.h"
#include "graphics/Screen.h"
#include <Arduino.h>

extern meshtastic::GPSStatus *gpsStatus;
extern GPS *gps;
extern graphics::Screen *screen;

class TestModule : private concurrency::OSThread
{
  public:
    TestModule() : concurrency::OSThread("TestModule")
    {
        LOG_INFO("TestModule: Inizializzazione modulo Test");
        initialized = true;
        counter = 0;
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
        // Incrementa il contatore
        counter++;
        gps->up();
        LOG_INFO("TestModule: GPS svegliato");
        LOG_INFO("TestModule: GPS Stato Lock: %d", gps->hasLock());
        LOG_INFO("TestModule: GPS Connesso: %d", gps->isConnected());

        // Scrive direttamente sul display
        writeToDisplay();

        LOG_INFO("TestModule: Contatore: %d", counter);
        return 1000; // Aggiorna ogni 2 secondi
    };

  private:
    bool initialized = false;
    uint32_t counter;

    /** Scrive il contatore direttamente sul display */
    void writeToDisplay()
    {
        // Verifica se il display è disponibile
        if (!screen || !screen->getDisplayDevice()) {
            LOG_WARN("TestModule: Display non disponibile");
            return;
        }

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
        snprintf(counterStr, sizeof(counterStr), "%d", counter);
        display->drawString(display->width() / 2, 40, counterStr);

        // Aggiunge informazioni aggiuntive
        display->setFont(ArialMT_Plain_10);
        char infoStr[50];
        snprintf(infoStr, sizeof(infoStr), "Uptime: %d sec", millis() / 1000);
        display->drawString(display->width() / 2, 70, infoStr);

        // Aggiorna il display
        display->display();

        LOG_INFO("TestModule: Scritto contatore %d sul display", counter);
    }
};

TestModule *testModule = nullptr;
