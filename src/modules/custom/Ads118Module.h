#pragma once
#include "concurrency/OSThread.h"
#include "graphics/Screen.h"
#include "mesh/generated/meshtastic/mesh.pb.h"
#include "mesh/generated/meshtastic/portnums.pb.h"
#include "serialization/JSON.h"
// #include "ADS1118SW.h"
#include <ADS1118.h>
#include <SPI.h>

/**
 * Modulo per la gestione dell'ADS1118 (ADC ad alta precisione)
 * Comunica via Software SPI e legge 4 canali analogici
 */
class Ads118Module : private concurrency::OSThread
{
  public:
    /** Constructor */
    Ads118Module();

    /** Destructor */
    ~Ads118Module();

    /** Inizializza il modulo */
    void setup();

  protected:
    /** Metodo principale del thread che viene chiamato periodicamente */
    virtual int32_t runOnce() override;
    SPIClass *spi = NULL;
    ADS1118 *ads1118 = NULL;

  private:
    bool initialized;
    uint32_t lastSentToMesh;

    // Pin SPI per ADS1118
    int _mosi, _miso, _sclk, _cs;

    // Configurazione ADS1118
    int _gain;
    int _dataRate;
    String message;

    /** Inizializza i pin SPI */
    bool initSPI();

    /** Inizializza l'ADS1118 */
    bool initADS1118();

    /** Test di comunicazione SPI */
    bool testSPICommunication();

    /** Test di comunicazione ADS1118 */
    bool testADS1118Connection();

    /** Invia telemetria ADS1118 */
    void sendADS1118Telemetry();

#if HAS_SCREEN
    /** Scrive il valore del ADS1118 sul display */
    void writeToDisplay();
#endif // HAS_SCREEN
};

extern Ads118Module *ads118Module;
