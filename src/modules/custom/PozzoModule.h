#pragma once
#include "concurrency/OSThread.h"
#include "graphics/Screen.h"
#include "mesh/generated/meshtastic/mesh.pb.h"
#include "mesh/generated/meshtastic/portnums.pb.h"
#include "serialization/JSON.h"
#include <Adafruit_ADS1X15.h>

/**
 * Modulo per la gestione dell'ADS1118 (ADC ad alta precisione)
 * Comunica via Software SPI e legge 4 canali analogici
 */
class PozzoModule : private concurrency::OSThread
{
  public:
    /** Constructor */
    PozzoModule();

    /** Destructor */
    ~PozzoModule();

    /** Inizializza il modulo */
    void setup();

  protected:
    /** Metodo principale del thread che viene chiamato periodicamente */
    virtual int32_t runOnce() override;

    Adafruit_ADS1115 *ads = NULL;

  private:
    bool initialized;
    uint32_t lastSentToMesh;


    // Configurazione ADS1118
    int _gain;
    int _dataRate;
    String message;

    /** Inizializza l'ADS1118 */
    bool initADS1115();


    /** Test di comunicazione ADS1118 */
    bool testADS1115Connection();

    /** Invia telemetria ADS1118 */
    void sendADS1118Telemetry();

#if HAS_SCREEN
    /** Scrive il valore del ADS1118 sul display */
    void writeToDisplay();
#endif // HAS_SCREEN
};

extern PozzoModule *pozzoModule;
