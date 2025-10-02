#pragma once
#include "concurrency/OSThread.h"
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

    /** Inizializza i pin SPI */
    bool initSPI();

    /** Inizializza l'ADS1118 */
    bool initADS1118();

    /** Trasferimento SPI software - invia 16 bit e ricevi 16 bit */
    uint16_t transfer16(uint16_t data);

    /** Legge un canale specifico */
    float readChannel(int channel);

    /** Legge un canale in formato raw */
    int16_t readChannelRaw(int channel);

    /** Converte valore raw in voltaggio */
    float rawToVoltage(int16_t rawValue);

    /** Test di comunicazione SPI */
    bool testSPICommunication();

    /** Test di comunicazione ADS1118 */
    bool testADS1118Connection();

    /** Invia telemetria ADS1118 */
    void sendADS1118Telemetry();

    /** Configura il gain dell'ADS1118 */
    void setGain(int gain);

    /** Configura il data rate dell'ADS1118 */
    void setDataRate(int rate);

    void blinkPin(int pin, const int count = 2, const int delayTime = 1000);
};

extern Ads118Module *ads118Module;
