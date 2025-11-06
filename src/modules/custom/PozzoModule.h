#pragma once
#include "concurrency/OSThread.h"
#include "graphics/Screen.h"
#include "mesh/generated/meshtastic/mesh.pb.h"
#include "mesh/generated/meshtastic/portnums.pb.h"
#include "serialization/JSON.h"
#include <Adafruit_ADS1X15.h>
#include <jm_LCM2004A_I2C.h>

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
    jm_LCM2004A_I2C *lcd = NULL; // addr: 0x27, Wire1
  private:
    bool initialized;
    uint32_t lastSentToMesh;
    int16_t waterLevelAdcValue = 0;
    float waterLevelMilliVolts = 0.0f;
    float waterLevelMillimeters = 0.0f;

    int16_t pumpCurrentAdcValue = 0;
    float pumpCurrentMilliVolts = 0.0f;
    float pumpCurrentAmps = 0.0f;
    float pumpCurrentPower = 0.0f;
    

    // Configurazione ADS1118
    int _gain;
    int _dataRate;
    String message;

    /** Inizializza l'ADS1118 */
    bool initADS1115();

    /** Inizializza il display */
    bool initDisplay();

    /** Test di comunicazione ADS1118 */
    bool testADS1115Connection();

    /** Invia telemetria ADS1118 */
    void sendADS1118Telemetry();

    void readWaterLevel();
    void readPumpCurrent();

    /** Inizializza il buffer del display con spazi */
    void initDisplayBuffer();

    /** Scrive una stringa nel buffer del display e aggiorna solo i caratteri cambiati */
    void _writeToDisplay(uint8_t col, uint8_t row, const char *text, bool forceUpdate = false);

#if HAS_SCREEN
    /** Scrive il valore del ADS1118 sul display */
    void writeToDisplay(bool firstUpdate=false);
#endif // HAS_SCREEN
};

extern PozzoModule *pozzoModule;
