#pragma once
#include "concurrency/OSThread.h"
#include "SinglePortModule.h"
#include "graphics/Screen.h"
#include "mesh/generated/meshtastic/mesh.pb.h"
#include "mesh/generated/meshtastic/portnums.pb.h"
#include "serialization/JSON.h"
#include <Adafruit_ADS1X15.h>
#include <jm_LCM2004A_I2C.h>
#include "PumpMonitor/FFTPumpMonitor.h"
#include "PumpMonitor/PumpMonitor.h"
/**
 * Modulo per la gestione dell'ADS1118 (ADC ad alta precisione)
 * Comunica via Software SPI e legge 4 canali analogici
 */
class PozzoModule : public SinglePortModule, private concurrency::OSThread
{
  public:
    /** Constructor */
    PozzoModule();

    /** Destructor */
    ~PozzoModule();

    /** Inizializza il modulo */
    void setup();

  protected:
    /** Gestisce la ricezione di messaggi */
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    
    /** Determina se il modulo vuole ricevere questo pacchetto */
    virtual bool wantPacket(const meshtastic_MeshPacket *p) override;
    typedef enum ReadingMode {
      NONE,
      WATER_LEVEL,
      PUMP_CURRENT,
    } ReadingMode;
    ReadingMode readingMode = NONE;
    /** Metodo principale del thread che viene chiamato periodicamente */
    virtual int32_t runOnce() override;

    Adafruit_ADS1115 *ads = NULL;
    jm_LCM2004A_I2C *lcd = NULL; // addr: 0x27, Wire1
    FFTPumpMonitor *fftPumpMonitor = NULL;
    PumpMonitor *pumpMonitor = NULL;

    
  private:
    bool initialized = false;
    uint32_t initializationTime = 0;
    uint32_t lastTelemetrySentTime = 0;
    uint32_t lastPumpSamplingCheck = 0;
    uint32_t lastDisplayUpdateTime = 0;
    uint32_t lastWaterLevelReadTime = 0;



    int16_t waterLevelAdcValue = 0;
    float waterLevelMilliVolts = 0.0f;
    float waterLevelMillimeters = 0.0f;

    int16_t pumpCurrentAdcValue = 0;
    float pumpCurrentMilliVolts = 0.0f;
    float pumpCurrentAmps = 0.0f;
    float pumpCurrentPower = 0.0f;

    // Gestione stato pompa con deviatore
    bool pumpDesiredState = false;     // Stato desiderato dal software
    bool pumpActualState = false;      // Stato reale rilevato dal monitor corrente
    bool pumpExternalControl = false;  // Flag che indica se la pompa è controllata esternamente
    bool pumpRelayState = false;       // Stato del relay della pompa
    uint32_t lastPumpStateCheck = 0;   // Ultimo controllo dello stato pompa
    

    String message;

    /** Inizializza l'ADS1118 */
    bool initADS1115();

    /** Inizializza il display */
    bool initDisplay();


    /** Invia telemetria  */
    void sendTelemetry();

    void readWaterLevel();
    void readPumpCurrent();

    /** Gestione stato pompa con deviatore */
    void updatePumpActualState();      // Aggiorna lo stato reale leggendo il monitor corrente
    void setPumpState(bool turnOn);    // Imposta lo stato desiderato della pompa
    bool isPumpOn();                   // Ritorna true se la pompa è accesa (stato reale)
    void syncPumpState();              // Sincronizza stato reale con desiderato
    void resetExternalControlFlag();   // Resetta il flag di controllo esterno
    bool isUnderExternalControl();     // Ritorna true se la pompa è sotto controllo esterno

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
