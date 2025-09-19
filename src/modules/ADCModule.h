#pragma once
#include "concurrency/OSThread.h"
#include "DHT.h"
/**
 * Modulo ADC per la lettura di valori analogici dal pin ADC1_H6.
 * Questo modulo legge periodicamente il valore dal pin ADC1_H6 e lo stampa nel log.
 */
class ADCModule : private concurrency::OSThread
{
  public:
    /** Constructor */
    ADCModule();

    /** Destructor */
    ~ADCModule();

    /** Inizializza il modulo */
    void setup();

  protected:
    /** Metodo principale del thread che viene chiamato periodicamente */
    virtual int32_t runOnce() override;

  private:
    bool initialized;
    DHT* dht;
    int adcPin;
    
    /** Inizializza il pin ADC */
    bool initADC();
    
    /** Legge il valore dal pin ADC */
    int readADCValue();

    /** Legge il valore del DHT */
    float readDHTValue();
};

extern ADCModule *adcModule;
