#pragma once
#include "arduinoFFT.h"
#include "DebugConfiguration.h"
#include <Arduino.h>

#define SAMPLES 256  // Numero di campioni per l'analisi FFT

class FFTPumpMonitor
{
  private:
    ArduinoFFT<double> FFT;
    double vReal[SAMPLES];
    double vImag[SAMPLES];
    
    // Puntatore al valore corrente letto esternamente
    float *currentValuePtr;
    
    // Buffer circolare per accumulare campioni
    int sampleIndex;
    bool bufferFull;
    
    // Tracking temporale
    unsigned long lastPickTime;
    float samplingFrequency;  // Frequenza di campionamento in Hz
    bool samplingWarning;
    
    // Risultati ultima analisi
    float lastTHD;
    bool anomalyDetected;

  public:
    // Costruttore: riceve il puntatore al float della corrente
    FFTPumpMonitor(float *currentPtr) 
        : currentValuePtr(currentPtr), 
          sampleIndex(0), 
          bufferFull(false),
          lastPickTime(0),
          samplingFrequency(0.0f),
          samplingWarning(false),
          lastTHD(0.0f),
          anomalyDetected(false)
    {
        // Inizializza array immaginari a zero
        for (int i = 0; i < SAMPLES; i++) {
            vImag[i] = 0;
        }
    }

    // Chiamata dall'esterno quando c'è un nuovo valore di corrente
    void pick()
    {
        if (currentValuePtr == nullptr) {
            return;
        }
        
        // Calcola la frequenza di campionamento
        unsigned long now = millis();
        if (sampleIndex > 0) {
            unsigned long dt = now - lastPickTime;
            if (dt > 0) {
                samplingFrequency = 1000.0f / dt;  // Hz
                
                // Verifica che la frequenza sia sufficiente (minimo 400 Hz per armoniche fino a 200Hz)
                if (samplingFrequency < 400.0f && !samplingWarning) {
                    samplingWarning = true;
                    LOG_WARN("Pump Monitor: Sampling too slow (%.1f Hz). Need >400Hz for accurate analysis", 
                             samplingFrequency);
                } else if (samplingFrequency >= 400.0f) {
                    samplingWarning = false;  // Reset warning se torna normale
                }
            }
        }
        lastPickTime = now;
        
        // Aggiungi il campione corrente al buffer
        vReal[sampleIndex] = *currentValuePtr;
        vImag[sampleIndex] = 0;
        
        sampleIndex++;
        
        // Quando il buffer è pieno, esegui l'analisi
        if (sampleIndex >= SAMPLES) {
            bufferFull = true;
            analyzeFrequencySpectrum();
            sampleIndex = 0;  // Reset per il prossimo ciclo
        }
    }
    
    // Esegue l'analisi FFT sui campioni raccolti
    void analyzeFrequencySpectrum()
    {
        if (!bufferFull) {
            return;  // Non abbastanza campioni
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
        if (samplingFrequency < 10.0f) {
            return;  // Frequenza di campionamento non ancora calcolata
        }
        
        // Calcola risoluzione frequenziale
        float freqResolution = samplingFrequency / SAMPLES;  // Hz per bin
        
        // Calcola gli indici corrispondenti alle frequenze di interesse
        // Frequenza fondamentale: 50Hz (rete elettrica europea)
        int idx_50Hz  = round(50.0f / freqResolution);
        int idx_100Hz = round(100.0f / freqResolution);
        int idx_150Hz = round(150.0f / freqResolution);
        
        // Verifica che gli indici siano validi
        if (idx_150Hz >= SAMPLES/2) {
            LOG_WARN("Pump Monitor: Sampling frequency too low (%.1f Hz) - cannot analyze up to 150Hz", 
                     samplingFrequency);
            return;
        }
        
        // Leggi le magnitudini alle frequenze di interesse
        float fundamental = vReal[idx_50Hz];
        float harmonic2   = vReal[idx_100Hz];
        float harmonic3   = vReal[idx_150Hz];
        
        if (fundamental < 0.01f) {
            return;  // Evita divisione per zero
        }

        // Calcola distorsione armonica totale (THD)
        lastTHD = sqrt(harmonic2 * harmonic2 + harmonic3 * harmonic3) / fundamental;

        if (lastTHD > 0.1f) {
            // Distorsione eccessiva = problema motore
            anomalyDetected = true;
            LOG_WARN("Pump: High harmonic distortion detected - THD: %.2f%% (Fs=%.1fHz, bins: %d,%d,%d)", 
                     lastTHD * 100.0f, samplingFrequency, idx_50Hz, idx_100Hz, idx_150Hz);
        } else {
            anomalyDetected = false;
        }
    }
    
    // Getter per accedere ai risultati
    float getTHD() const { return lastTHD; }
    bool hasAnomaly() const { return anomalyDetected; }
    int getSampleCount() const { return sampleIndex; }
    bool isBufferFull() const { return bufferFull; }
    
    // Getter per informazioni sul campionamento
    float getSamplingFrequency() const { return samplingFrequency; }
    bool hasSamplingWarning() const { return samplingWarning; }
    
    // Calcola tempo stimato per completare il buffer
    float getEstimatedTimeToAnalysis() const { 
        if (samplingFrequency < 1.0f || sampleIndex >= SAMPLES) {
            return 0.0f;
        }
        int remainingSamples = SAMPLES - sampleIndex;
        return remainingSamples / samplingFrequency;  // secondi
    }
};