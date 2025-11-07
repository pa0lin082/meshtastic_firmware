class SmartPumpMonitor
{
  private:
    ADS1118 *adc;
    PumpMonitor *basicMonitor;
    AdvancedPumpMonitor *advancedMonitor;
    StatisticalPumpMonitor *statMonitor;

  public:
    void monitorPump()
    {
        // Monitoraggio base
        PumpStatus status = basicMonitor->checkPumpStatus();

        // Analisi avanzata (ogni 10 secondi)
        if (millis() % 10000 == 0) {
            advancedMonitor->analyzeFrequencySpectrum();
        }

        // Aggiorna statistiche
        statMonitor->update();

        // Log risultati
        logPumpStatus(status);
    }

    void logPumpStatus(PumpStatus status)
    {
        switch (status) {
        case PUMP_NORMAL:
            LOG_INFO("Pump: Normal operation");
            break;
        case PUMP_DRY_RUN:
            LOG_WARN("Pump: Dry run detected - stop pump!");
            break;
        case PUMP_BLOCKED:
            LOG_ERROR("Pump: Blocked - check pump!");
            break;
        }
    }
};