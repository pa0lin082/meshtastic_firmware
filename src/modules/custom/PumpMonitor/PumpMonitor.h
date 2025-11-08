#pragma once
#include "DebugConfiguration.h"
#include "input/InputBroker.h"
#include "Observer.h"
#include <Arduino.h>
#include <vector>
#include <deque>
#include <algorithm>
#include <cmath>

constexpr size_t HISTORY_SIZE = 256;  // Numero di campioni storici da tenere in memoria
constexpr size_t STARTUP_SAMPLES = 20; // tollera più rumore all'accensione
constexpr double SPIKE_K = 6.0; // soglia MAD per spike
constexpr double EWMA_ALPHA = 0.05; // peso EWMA per baseline
constexpr double EWMA_VAR_ALPHA = 0.02; // peso per varianza EWMA
constexpr uint32_t PAUSE_THRESHOLD_MS = 500;
constexpr uint32_t DWELL_TIME_MS = 2000;


static double median(std::vector<float>& v) {
    if (v.empty()) return 0.0;
    size_t n = v.size();
    std::sort(v.begin(), v.end());
    if (n % 2 == 1) return v[n/2];
    return 0.5 * (v[n/2 - 1] + v[n/2]);
}

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
    float *currentValuePtr; // Puntatore al valore corrente letto esternamente

    
    // Observer per eventi input (bottoni)
    CallbackObserver<PumpMonitor, const InputEvent *> inputObserver =
        CallbackObserver<PumpMonitor, const InputEvent *>(this, &PumpMonitor::handleInputEvent);
    
    //history management
    std::deque<float> currentHistory;
    std::deque<unsigned long> interArrival;
    double sum = 0.0;
    double sumSq = 0.0;

    // timing
    unsigned long lastTime;
    bool hasLastTime = false;

     // startup
    size_t samplesSeen = 0;


     // out of band detection
     bool outOfBand = false;
     unsigned long outOfBandStart;

    // spike handling
    int spikeCounter = 0;
    const int spikeTransientLimit = 2; // quanti campioni considerare "transiente"

    // thresholds
    const double baselineK = 4.0; // soglia in sigma per considerare "out of baseline"
    const double minStdFloor = 0.01; // pavimento per std per non avere divisioni per 0

    // EWMA baseline
    bool hasEwma = false;
    double ewma = 0.0;
    double ewmaVar = 0.0;
    
    // Baseline e soglie
    float baselineCurrent;      // Corrente di riferimento (calibrata)
    float thresholdLow;         // Soglia calo corrente (0.8 = 80%)
    float thresholdHigh;        // Soglia sovracorrente (1.2 = 120%)
    float degradationThreshold; // Soglia degrado graduale (0.15A)
    float oscillationThreshold; // Soglia oscillazioni (30% campioni variabili)
    
    // Statistiche calcolate
    float currentAverage;
    float currentStdDev;
    float currentVariance;
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
        if (currentHistory.size() < 10) {
            return;  // Non abbastanza dati
        }
        
        size_t count = currentHistory.size();
        
        // Media, min, max
        float sum = 0;
        currentMin = currentHistory.front();
        currentMax = currentHistory.front();
        
        for (const auto& val : currentHistory) {
            sum += val;
            if (val < currentMin) currentMin = val;
            if (val > currentMax) currentMax = val;
        }
        currentAverage = sum / count;
        
        // Deviazione standard
        float variance = 0;
        for (const auto& val : currentHistory) {
            float diff = val - currentAverage;
            variance += diff * diff;
        }
        variance = variance / count;
        currentStdDev = sqrt(variance);
    }
    
    // Calcola media di un intervallo specifico (da inizio deque)
    float averageRange(size_t start, size_t end)
    {
        if (end > currentHistory.size() || start >= end) {
            return 0.0f;
        }
        
        float sum = 0;
        size_t count = end - start;
        for (size_t i = start; i < end; i++) {
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

    void printStats(float value, double median, double approxStd, double ewmaVal, double ewmaStd) {
        size_t n = currentHistory.size();
        double avg = (n > 0) ? sum / n : 0.0;
        double variance = (n > 0) ? (sumSq / n - avg*avg) : 0.0;
        double stddev = (variance > 0.0) ? std::sqrt(variance) : 0.0;

        LOG_INFO("PumpMonitor: Stats - N=%d, val=%.2f, avg=%.2f, min=%.2f, "
                 "max=%.2f, std=%.2f, med=%.2f, MAD*1.4826~std=%.2f, "
                 "EWMA=%.2f, EWMAstd~%.2f",
                 n, value, avg, (n ? *std::min_element(currentHistory.begin(), currentHistory.end()) : 0.0), (n ? *std::max_element(currentHistory.begin(), currentHistory.end()) : 0.0), stddev, median, approxStd, ewmaVal, ewmaStd);
    }

  public:
    // Costruttore: riceve il puntatore al float della corrente
    PumpMonitor(float *currentPtr) 
        : currentValuePtr(currentPtr),
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
    }

    // Aggiunge un nuovo campione di corrente
    void pick()
    {
        if (currentValuePtr == nullptr) {
            return;
        }

        auto now = millis();
        float current = *currentValuePtr;


        // --- inter-arrival time
        if (hasLastTime) {
            auto deltaMs = now - lastTime;
            interArrival.push_back(deltaMs);
            if (interArrival.size() > HISTORY_SIZE) interArrival.pop_front();

            if (deltaMs > PAUSE_THRESHOLD_MS) {
              LOG_WARN("PumpMonitor: Pausa lunga: %lu ms", deltaMs);
            }
        }
        lastTime = now;
        hasLastTime = true;


         // --- gestione finestra dati (sum / sumSq per stddev)
        if (currentHistory.size() >= HISTORY_SIZE) {
            float old = currentHistory.front();
            currentHistory.pop_front();
            sum -= old;
            sumSq -= old * old;
        }
        currentHistory.push_back(current);

        
        sum += current;
        sumSq += current * current;

        currentAverage = sum / currentHistory.size();
        currentVariance = sumSq / currentHistory.size() - currentAverage * currentAverage;
        currentStdDev = sqrt(currentVariance);

        currentMin = *std::min_element(currentHistory.begin(), currentHistory.end());
        currentMax = *std::max_element(currentHistory.begin(), currentHistory.end());


         // --- rolling window for robust stats: compute median & MAD from copy (cost O(n log n))
        // For MAX_SIZE ~100 è accettabile; se alto, usare struttura specializzata.
        std::vector<float> tmp(currentHistory.begin(), currentHistory.end());
        double med = median(tmp);

        // MAD
        for (auto &x : tmp) x = static_cast<float>(std::abs(x - med));
        double mad = median(tmp);
        // converti MAD -> approx stddev: std ≈ 1.4826 * MAD for normal dist
        double approxStd = mad * 1.4826;


         // --- EWMA baseline + variance (per rilevazione deviazioni prolungate)
         if (!hasEwma) {
            ewma = current;
            ewmaVar = 0.0;
            hasEwma = true;
        } else {
            double delta = current - ewma;
            ewma += EWMA_ALPHA * delta;
            // ewma of squared error
            ewmaVar += EWMA_VAR_ALPHA * (delta*delta - ewmaVar);
        }
        double ewmaStd = std::sqrt(std::max(0.0, ewmaVar));


         // --- Gestione stato startup (più tollerante)
         if (samplesSeen < STARTUP_SAMPLES) {
            ++samplesSeen;
            // non valutare come anomalia reale; ma possiamo ancora stampare statistica
            printStats(current, med, approxStd, ewma, ewmaStd);
            return;
         }


          // --- Spike detection (breve outlier)
        bool isSpike = false;
        if (mad == 0.0) {
            // caso in cui tutti i valori uguali -> se valore diverso anche leggermente, consideralo
            isSpike = (std::abs(current - med) > 1e-6);
        } else {
            isSpike = (std::abs(current - med) > SPIKE_K * mad);
        }


        // Se è spike ma dura un solo campione (o pochi), lo ignoriamo:
        if (isSpike) {
            spikeCounter++;
            // se spike prolungato oltre soglia temporale, consideralo anomalia
            if (spikeCounter <= spikeTransientLimit) {
                LOG_INFO("PumpMonitor: Spike transiente rilevato (sample %d) valore=%.2f", spikeCounter, current);
                // non aggiornare stato anomalia prolungata; stampa stats comunque
                printStats(current, med, approxStd, ewma, ewmaStd);
                // return;
            } else {
                // prolungato -> treat as sustained anomaly
                LOG_WARN("PumpMonitor: Spike prolungato: valore=%.2f", current);
                // caduta intenzionale nel flusso di controllo per segnalarlo come anomalia
            }
        } else {
            spikeCounter = 0; // reset conto spike consecutivi
        }


         // --- Rilevazione deviazione prolungata rispetto alla EWMA
         double diffFromBaseline = current - ewma;
         bool deviateHigh = (diffFromBaseline > baselineK * std::max(ewmaStd, minStdFloor));
         bool deviateLow = (diffFromBaseline < -baselineK * std::max(ewmaStd, minStdFloor));



         // manteniamo timer per quanto tempo siamo "fuori soglia"
        if (deviateHigh || deviateLow) {
            if (!outOfBand) {
                outOfBand = true;
                outOfBandStart = now;
            } else {
                auto dur = now - outOfBandStart;
                if (dur >= DWELL_TIME_MS) {
                  LOG_WARN("PumpMonitor: Deviazione prolungata di %lu ms. Valore=%.2f EWMA=%.2f diff=%.2f", dur, current, ewma, diffFromBaseline);
                    // qui puoi attivare log, allarme, shutdown, ecc.
                }
            }
        } else {
            outOfBand = false;
        }

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
        if (currentHistory.size() >= 60) {
            // Ultimi 20 campioni (più recenti sono alla fine del deque)
            size_t size = currentHistory.size();
            float avgRecent = averageRange(size - 20, size);
            
            // Primi 20 campioni (più vecchi)
            float avgOld = averageRange(0, 20);
            
            if (avgOld - avgRecent > degradationThreshold) {
                LOG_WARN("Pump: Gradual degradation detected - Old avg: %.2fA, Recent avg: %.2fA (drop: %.2fA)", 
                         avgOld, avgRecent, avgOld - avgRecent);
                currentStatus = PUMP_DEGRADED;
                return currentStatus;
            }
        }
        
        // 5. Controllo oscillazioni eccessive (cavitazione/aria)
        size_t histSize = currentHistory.size();
        if (histSize >= 2) {
            int significantChanges = 0;
            size_t checkCount = (histSize > 50) ? 50 : histSize - 1;
            
            // Controlla le ultime 'checkCount' transizioni
            size_t startIdx = histSize - checkCount - 1;
            for (size_t i = startIdx; i < histSize - 1; i++) {
                float diff = abs(currentHistory[i + 1] - currentHistory[i]);
                
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
    size_t getSampleCount() const { return currentHistory.size(); }
    bool isHistoryFull() const { return currentHistory.size() >= HISTORY_SIZE; }
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