#include "arduinoFFT.h"

class AdvancedPumpMonitor
{
  private:
    arduinoFFT<float> FFT;
    float *vReal;
    float *vImag;

  public:
    void analyzeFrequencySpectrum()
    {
        // Campiona corrente per 1 secondo
        for (int i = 0; i < SAMPLES; i++) {
            vReal[i] = readCurrent();
            vImag[i] = 0;
            delay(1000 / SAMPLES);
        }

        // Analisi FFT
        FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
        FFT.complexToMagnitude(vReal, vImag, SAMPLES);

        // Analizza frequenze caratteristiche
        analyzeHarmonics();
    }

    void analyzeHarmonics()
    {
        // Frequenza fondamentale (50Hz o 60Hz)
        float fundamental = vReal[50]; // 50Hz

        // Armoniche (100Hz, 150Hz, 200Hz...)
        float harmonic2 = vReal[100];
        float harmonic3 = vReal[150];

        // Calcola distorsione armonica
        float THD = sqrt(harmonic2 * harmonic2 + harmonic3 * harmonic3) / fundamental;

        if (THD > 0.1) {
            // Distorsione eccessiva = problema motore
            LOG_WARN("Pump: High harmonic distortion detected");
        }
    }
};