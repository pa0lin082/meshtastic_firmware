class PumpMonitor
{
  private:
    ADS1118 *adc;
    float baselineCurrent; // Corrente di riferimento
    float thresholdLow;    // Soglia calo corrente
    float thresholdHigh;   // Soglia sovracorrente

  public:
    PumpMonitor(ADS1118 *adc) : adc(adc)
    {
        baselineCurrent = 0.0;
        thresholdLow = 0.8;  // 80% della corrente normale
        thresholdHigh = 1.2; // 120% della corrente normale
    }

    float readCurrent()
    {
        // Legge tensione dall'ADS1118
        double voltage = adc->getMilliVolts() / 1000.0; // Converti in Volt

        // Calcola corrente (ACS712-20A: 100 mV/A)
        float current = (voltage - 1.65) / 0.100; // Per 3.3V supply

        return current;
    }

    PumpStatus checkPumpStatus()
    {
        float current = readCurrent();

        if (current < baselineCurrent * thresholdLow) {
            return PUMP_DRY_RUN; // Pompa a vuoto
        } else if (current > baselineCurrent * thresholdHigh) {
            return PUMP_BLOCKED; // Pompa bloccata
        } else {
            return PUMP_NORMAL; // Funzionamento normale
        }
    }

    void calibrate()
    {
        // Calibra la corrente di riferimento
        float sum = 0;
        for (int i = 0; i < 100; i++) {
            sum += readCurrent();
            delay(10);
        }
        baselineCurrent = sum / 100.0;
    }
};