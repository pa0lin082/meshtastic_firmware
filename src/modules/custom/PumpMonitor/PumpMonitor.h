#pragma once
#include "DebugConfiguration.h"
#include "input/InputBroker.h"
#include "Observer.h"
#include <Arduino.h>

#define HISTORY_SIZE 256  // Numero di campioni storici da tenere in memoria

enum PumpStatus {
    PUMP_NORMAL,
    PUMP_DRY_RUN,      // Pompa a vuoto (corrente troppo bassa)
    PUMP_BLOCKED,      // Pompa bloccata (corrente troppo alta)
    PUMP_DEGRADED,     // Pompa in degrado (calo prestazioni graduale)
    PUMP_UNSTABLE,     // Pompa instabile (oscillazioni eccessive)
    PUMP_SPIKE         // Picco anomalo rilevato
};

class PumpMonitor
{
  private:
    float *currentValuePtr;  // Puntatore al valore corrente letto esternamente
    
    // Observer per eventi input (bottoni)
    CallbackObserver<PumpMonitor, const InputEvent *> inputObserver =
        CallbackObserver<PumpMonitor, const InputEvent *>(this, &PumpMonitor::handleInputEvent);
    
    // Storia delle letture
    float currentHistory[HISTORY_SIZE];
    int historyIndex;
    bool historyFilled;
    
    // Baseline e soglie
    float baselineCurrent;      // Corrente di riferimento (calibrata)
    float thresholdLow;         // Soglia calo corrente (0.8 = 80%)
    float thresholdHigh;        // Soglia sovracorrente (1.2 = 120%)
    float degradationThreshold; // Soglia degrado graduale (0.15A)
    float oscillationThreshold; // Soglia oscillazioni (30% campioni variabili)
    
    // Statistiche calcolate
    float currentAverage;
    float currentStdDev;
    float currentMin;
    float currentMax;
    
    // Stato rilevato
    PumpStatus currentStatus;
    unsigned long lastAnalysisTime;
    
    // Stato calibrazione non-bloccante
    bool isCalibrating;
    int calibrationSamples;
    int calibrationTargetSamples;
    float calibrationSum;
    int calibrationValidSamples;
    
    // Calcola statistiche sul buffer
    void calculateStatistics()
    {
        if (!historyFilled && historyIndex < 10) {
            return;  // Non abbastanza dati
        }
        
        int count = historyFilled ? HISTORY_SIZE : historyIndex;
        
        // Media
        float sum = 0;
        currentMin = currentHistory[0];
        currentMax = currentHistory[0];
        
        for (int i = 0; i < count; i++) {
            sum += currentHistory[i];
            if (currentHistory[i] < currentMin) currentMin = currentHistory[i];
            if (currentHistory[i] > currentMax) currentMax = currentHistory[i];
        }
        currentAverage = sum / count;
        
        // Deviazione standard
        float variance = 0;
        for (int i = 0; i < count; i++) {
            float diff = currentHistory[i] - currentAverage;
            variance += diff * diff;
        }
        variance = variance / count;
        currentStdDev = sqrt(variance);
    }
    
    // Calcola media di un intervallo specifico
    float averageRange(int start, int end)
    {
        if (end > (historyFilled ? HISTORY_SIZE : historyIndex)) {
            return 0.0f;
        }
        
        float sum = 0;
        int count = end - start;
        for (int i = start; i < end; i++) {
            sum += currentHistory[i];
        }
        return sum / count;
    }
    
    // Finalizza la calibrazione (chiamato automaticamente)
    void endCalibrate()
    {
        if (calibrationValidSamples > calibrationTargetSamples / 2) {
            baselineCurrent = calibrationSum / calibrationValidSamples;
            LOG_INFO("Pump Monitor: Calibration complete - Baseline: %.2fA (%d/%d valid samples)", 
                     baselineCurrent, calibrationValidSamples, calibrationSamples);
        } else {
            LOG_ERROR("Pump Monitor: Calibration failed - too few valid samples (%d/%d)", 
                      calibrationValidSamples, calibrationSamples);
        }
        
        isCalibrating = false;
        calibrationSamples = 0;
        calibrationSum = 0;
        calibrationValidSamples = 0;
    }

  public:
    // Costruttore: riceve il puntatore al float della corrente
    PumpMonitor(float *currentPtr) 
        : currentValuePtr(currentPtr),
          historyIndex(0),
          historyFilled(false),
          baselineCurrent(0.0f),
          thresholdLow(0.8f),
          thresholdHigh(1.2f),
          degradationThreshold(0.15f),
          oscillationThreshold(0.3f),
          currentAverage(0.0f),
          currentStdDev(0.0f),
          currentMin(0.0f),
          currentMax(0.0f),
          currentStatus(PUMP_NORMAL),
          lastAnalysisTime(0),
          isCalibrating(false),
          calibrationSamples(0),
          calibrationTargetSamples(0),
          calibrationSum(0.0f),
          calibrationValidSamples(0)
    {
        // Registra questo oggetto come observer degli eventi input
        if (inputBroker)
            inputObserver.observe(inputBroker);
        
        // Inizializza history a zero
        for (int i = 0; i < HISTORY_SIZE; i++) {
            currentHistory[i] = 0.0f;
        }
    }

    // Aggiunge un nuovo campione di corrente
    void pick()
    {
        if (currentValuePtr == nullptr) {
            return;
        }
        
        float current = *currentValuePtr;
        
        // Se siamo in calibrazione, accumula campioni
        if (isCalibrating) {
            calibrationSamples++;
            
            // Filtra valori anomali
            if (current > 0.0f && current < 30.0f) {
                calibrationSum += current;
                calibrationValidSamples++;
            }
            
            // Controlla se abbiamo raggiunto il target
            if (calibrationSamples >= calibrationTargetSamples) {
                endCalibrate();
            }
            
            // Durante calibrazione non aggiorniamo lo storico
            return;
        }
        
        // Aggiungi al buffer circolare
        currentHistory[historyIndex] = current;
        historyIndex++;
        
        if (historyIndex >= HISTORY_SIZE) {
            historyIndex = 0;
            historyFilled = true;
        }
        
        // Ricalcola statistiche ogni 10 campioni
        if (historyIndex % 10 == 0) {
            calculateStatistics();
        }
    }
    
    // Analizza lo stato della pompa
    PumpStatus analyze()
    {
        if (currentValuePtr == nullptr || baselineCurrent < 0.1f) {
            return PUMP_NORMAL;  // Non calibrato
        }
        
        calculateStatistics();
        lastAnalysisTime = millis();
        
        float current = *currentValuePtr;
        
        // 1. Controllo picchi anomali (spike detection)
        if (abs(current - currentAverage) > 3.0f * currentStdDev && currentStdDev > 0.05f) {
            LOG_WARN("Pump: Anomalous spike detected - Current: %.2fA, Avg: %.2fA, StdDev: %.2fA", 
                     current, currentAverage, currentStdDev);
            currentStatus = PUMP_SPIKE;
            return currentStatus;
        }
        
        // 2. Controllo corrente troppo bassa (marcia a vuoto)
        if (current < baselineCurrent * thresholdLow) {
            LOG_WARN("Pump: Dry run detected - Current: %.2fA < Baseline: %.2fA (%.0f%%)", 
                     current, baselineCurrent, thresholdLow * 100);
            currentStatus = PUMP_DRY_RUN;
            return currentStatus;
        }
        
        // 3. Controllo sovracorrente (blocco)
        if (current > baselineCurrent * thresholdHigh) {
            LOG_WARN("Pump: Overcurrent/blocked - Current: %.2fA > Baseline: %.2fA (%.0f%%)", 
                     current, baselineCurrent, thresholdHigh * 100);
            currentStatus = PUMP_BLOCKED;
            return currentStatus;
        }
        
        // 4. Controllo degrado graduale (confronta campioni recenti vs vecchi)
        if (historyFilled && historyIndex > 50) {
            float avgRecent = averageRange(0, 20);      // Ultimi 20 campioni
            int oldStart = historyIndex + 20;
            if (oldStart >= HISTORY_SIZE) oldStart -= HISTORY_SIZE;
            float avgOld = averageRange(oldStart, oldStart + 20);  // Campioni vecchi
            
            if (avgOld - avgRecent > degradationThreshold) {
                LOG_WARN("Pump: Gradual degradation detected - Old avg: %.2fA, Recent avg: %.2fA (drop: %.2fA)", 
                         avgOld, avgRecent, avgOld - avgRecent);
                currentStatus = PUMP_DEGRADED;
                return currentStatus;
            }
        }
        
        // 5. Controllo oscillazioni eccessive (cavitazione/aria)
        int significantChanges = 0;
        int checkCount = historyFilled ? 50 : (historyIndex > 50 ? 50 : historyIndex);
        
        for (int i = 1; i < checkCount; i++) {
            int idx1 = (historyIndex - i + HISTORY_SIZE) % HISTORY_SIZE;
            int idx2 = (historyIndex - i - 1 + HISTORY_SIZE) % HISTORY_SIZE;
            float diff = abs(currentHistory[idx1] - currentHistory[idx2]);
            
            if (diff > 0.2f) {  // Variazione > 0.2A
                significantChanges++;
            }
        }
        
        float changeRatio = (float)significantChanges / checkCount;
        if (changeRatio > oscillationThreshold) {
            LOG_WARN("Pump: Excessive oscillations detected - %.1f%% samples show significant changes", 
                     changeRatio * 100);
            currentStatus = PUMP_UNSTABLE;
            return currentStatus;
        }
        
        // Tutto OK
        currentStatus = PUMP_NORMAL;
        return currentStatus;
    }
    
    // Avvia calibrazione non-bloccante della corrente di riferimento
    void startCalibrate(int samples = 100)
    {
        if (currentValuePtr == nullptr) {
            LOG_ERROR("Pump Monitor: Cannot calibrate - no current pointer set");
            return;
        }
        
        if (isCalibrating) {
            LOG_WARN("Pump Monitor: Calibration already in progress");
            return;
        }
        
        LOG_INFO("Pump Monitor: Starting non-blocking calibration with %d samples...", samples);
        
        isCalibrating = true;
        calibrationSamples = 0;
        calibrationTargetSamples = samples;
        calibrationSum = 0.0f;
        calibrationValidSamples = 0;
    }
    
    // Wrapper per compatibilità - chiama startCalibrate
    void calibrate(int samples = 100)
    {
        startCalibrate(samples);
    }
    
    // Getter per diagnostica
    float getBaseline() const { return baselineCurrent; }
    float getAverage() const { return currentAverage; }
    float getStdDev() const { return currentStdDev; }
    float getMin() const { return currentMin; }
    float getMax() const { return currentMax; }
    PumpStatus getStatus() const { return currentStatus; }
    int getSampleCount() const { return historyFilled ? HISTORY_SIZE : historyIndex; }
    bool isHistoryFull() const { return historyFilled; }
    bool isCalibrationInProgress() const { return isCalibrating; }
    int getCalibrationProgress() const { return isCalibrating ? calibrationSamples : 0; }
    int getCalibrationTarget() const { return calibrationTargetSamples; }
    
    // Ottieni stringa di stato human-readable
    const char* getStatusString() const
    {
        switch (currentStatus) {
            case PUMP_NORMAL:    return "Normal";
            case PUMP_DRY_RUN:   return "Dry Run";
            case PUMP_BLOCKED:   return "Blocked";
            case PUMP_DEGRADED:  return "Degraded";
            case PUMP_UNSTABLE:  return "Unstable";
            case PUMP_SPIKE:     return "Spike";
            default:             return "Unknown";
        }
    }
    
    // Configurazione soglie
    void setThresholds(float low, float high, float degradation = 0.15f, float oscillation = 0.3f)
    {
        thresholdLow = low;
        thresholdHigh = high;
        degradationThreshold = degradation;
        oscillationThreshold = oscillation;
        
        LOG_INFO("Pump Monitor: Thresholds updated - Low: %.0f%%, High: %.0f%%, Degradation: %.2fA, Oscillation: %.0f%%",
                 low * 100, high * 100, degradation, oscillation * 100);
    }
    
    // Cancella calibrazione in corso
    void cancelCalibration()
    {
        if (isCalibrating) {
            LOG_WARN("Pump Monitor: Calibration cancelled by user");
            isCalibrating = false;
            calibrationSamples = 0;
            calibrationSum = 0;
            calibrationValidSamples = 0;
        }
    }
    
    // Gestore eventi input (pressione bottoni)
    int handleInputEvent(const InputEvent *event)
    {
        if (!event) return 0;

       
        LOG_INFO("PumpMonitor: Input event %u from %s, kbchar: %u", 
                 event->inputEvent, event->source ? event->source : "unknown", event->kbchar);
        
        // Gestione eventi bottone
        switch (event->inputEvent) {
            case INPUT_BROKER_SELECT:
                if (isCalibrating) {
                    LOG_INFO("PumpMonitor: User select button pressed - cancelling calibration");
                    cancelCalibration();
                } else {
                    LOG_INFO("PumpMonitor: User select button pressed - starting calibration");
                    startCalibrate();
                }
                return 0;
                
            case INPUT_BROKER_USER_PRESS:
                LOG_INFO("PumpMonitor: User button pressed - printing diagnostics");
                // Stampa diagnostica dettagliata
                printDiagnostics();
                return 0;
                
            default:
                // Non gestito da questo modulo
                break;
        }
        
        return false; // Evento non gestito, passa ad altri observer
    }
    
    // Stampa diagnostica dettagliata
    void printDiagnostics()
    {
        LOG_INFO("=== Pump Monitor Diagnostics ===");
        
        if (isCalibrating) {
            LOG_INFO("CALIBRATING: %d/%d samples", calibrationSamples, calibrationTargetSamples);
            LOG_INFO("Valid samples: %d, Current sum: %.2fA", calibrationValidSamples, calibrationSum);
        } else {
          LOG_INFO("Starting analysis...");
          analyze();
          LOG_INFO("Analysis complete: %s", getStatusString());
        }
        
        LOG_INFO("==============================");
    }
};