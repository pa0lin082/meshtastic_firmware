#include "RunningAverage.h"

class StatisticalPumpMonitor
{
  private:
    RunningAverage currentAvg;
    RunningAverage powerAvg;

  public:
    StatisticalPumpMonitor() : currentAvg(100), powerAvg(100) {}

    void update()
    {
        float current = readCurrent();
        float power = current * 220; // Assumendo 220V

        currentAvg.addValue(current);
        powerAvg.addValue(power);

        // Analizza trend
        analyzeTrend();
    }

    void analyzeTrend()
    {
        float avgCurrent = currentAvg.getAverage();
        float stdDev = currentAvg.getStandardDeviation();

        // Calo progressivo = fine acqua
        if (avgCurrent < baselineCurrent * 0.7) {
            LOG_WARN("Pump: Possible dry run detected");
        }

        // Variazione eccessiva = instabilità
        if (stdDev > baselineCurrent * 0.1) {
            LOG_WARN("Pump: Unstable operation detected");
        }
    }
};