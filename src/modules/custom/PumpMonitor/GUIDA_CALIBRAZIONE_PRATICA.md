# 🔧 Guida Pratica alla Calibrazione del PumpMonitor

## 📋 Obiettivo

Calibrare i parametri EWMA basandosi su **test reali** della pompa nelle tue condizioni operative, sia durante la fase di **accensione** (transitorio) che durante il **lavoro normale** (regime stazionario).

---

## 🎯 Procedura di Test Completa

### FASE 1: Preparazione (5 minuti)

#### 1.1 Abilita i log dettagliati

In `PumpMonitor.h`, assicurati che il log alla riga 331-334 sia attivo:

```cpp
if (now - lastPrint > 100) {
   LOG_INFO("PumpMonitor: current: %.3f, ewma: %.3f, diff: %.3f threshold: %.3f", 
            current, ewma, diffFromBaseline, baselineK * std::max(ewmaStd, minStdFloor));
   lastPrint = now;
}
```

#### 1.2 Usa configurazione iniziale conservativa

```cpp
// Configurazione di partenza per i test
constexpr double EWMA_ALPHA = 0.05;       // Moderato
constexpr double EWMA_VAR_ALPHA = 0.02;   // Stabile
constexpr double MIN_CURRENT_THRESHOLD = 0.05;  // 50mA
const double baselineK = 4.0;             // Conservativo
const double minStdFloor = 0.05;          // 50mA
```

#### 1.3 Prepara il sistema di logging

```bash
# Su serial monitor, salva l'output su file
screen /dev/ttyUSB0 115200 | tee calibration_test_$(date +%Y%m%d_%H%M%S).log

# Oppure usa PlatformIO
pio device monitor > calibration_test_$(date +%Y%m%d_%H%M%S).log
```

---

### FASE 2: Test Pompa Spenta (3 minuti)

**Obiettivo:** Misurare il rumore elettrico di fondo

#### 2.1 Esecuzione
- ✅ Pompa completamente SPENTA
- ✅ Sistema alimentato e operativo
- ✅ Lascia registrare per 3 minuti

#### 2.2 Cosa osservare nei log

```
PumpMonitor: current: 0.001, ewma: 0.001, diff: 0.000, threshold: 0.200
PumpMonitor: current: 0.002, ewma: 0.001, diff: 0.001, threshold: 0.200
PumpMonitor: current: -0.001, ewma: 0.001, diff: -0.002, threshold: 0.200
```

#### 2.3 Dati da raccogliere

Cerca nei log e annota:

```
RUMORE_MAX = ___________ A    (massimo valore assoluto di 'current')
RUMORE_TIPICO = ___________ A  (valore tipico di 'current')
```

**Esempio:**
```
RUMORE_MAX = 0.003A (3mA)
RUMORE_TIPICO = 0.001A (1mA)
```

#### 2.4 Analisi

```bash
# Estrai statistiche dal log
grep "PumpMonitor: current:" calibration_test_*.log | \
  awk '{print $3}' | \
  sort -n | \
  awk 'BEGIN {sum=0; n=0; max=0}
       {sum+=$1; n++; if($1>max) max=$1}
       END {print "Media:", sum/n, "Max:", max}'
```

**Azione:** Se RUMORE_MAX > MIN_CURRENT_THRESHOLD, aumenta la soglia:
```cpp
constexpr double MIN_CURRENT_THRESHOLD = 0.10;  // Da 0.05 a 0.10
```

---

### FASE 3: Test Accensione Pompa - Transitorio (2 minuti)

**Obiettivo:** Misurare il comportamento durante startup

#### 3.1 Esecuzione
- ✅ Cancella i log precedenti o aggiungi un marker
- ✅ ACCENDI la pompa
- ✅ Registra per 2 minuti dall'accensione
- ✅ Annota il timestamp esatto dell'accensione

#### 3.2 Cosa osservare

**Nei primi 5-10 secondi (corrente di spunto):**
```
T=0s:  PumpMonitor: current: 0.001, ewma: 0.001, diff: 0.000, threshold: 0.200
T=1s:  PumpMonitor: current: 7.234, ewma: 0.362, diff: 6.872, threshold: 0.200  ← Picco!
T=2s:  PumpMonitor: current: 6.123, ewma: 0.650, diff: 5.473, threshold: 0.200
T=3s:  PumpMonitor: current: 5.456, ewma: 0.890, diff: 4.566, threshold: 0.200
```

**Dopo 10-30 secondi (stabilizzazione):**
```
T=15s: PumpMonitor: current: 5.123, ewma: 2.450, diff: 2.673, threshold: 0.250
T=20s: PumpMonitor: current: 5.098, ewma: 3.120, diff: 1.978, threshold: 0.280
T=30s: PumpMonitor: current: 5.089, ewma: 4.200, diff: 0.889, threshold: 0.320
```

**Dopo 60 secondi (regime):**
```
T=60s:  PumpMonitor: current: 5.100, ewma: 4.950, diff: 0.150, threshold: 0.400
T=90s:  PumpMonitor: current: 5.098, ewma: 5.020, diff: 0.078, threshold: 0.400
T=120s: PumpMonitor: current: 5.105, ewma: 5.045, diff: 0.060, threshold: 0.400
```

#### 3.3 Dati da raccogliere

```
CORRENTE_SPUNTO = ___________ A     (picco massimo nei primi 5 secondi)
CORRENTE_STABILIZZAZIONE = ___________ A  (valore dopo 30 secondi)
CORRENTE_REGIME = ___________ A     (valore dopo 120 secondi)
TEMPO_STABILIZZAZIONE = ___________ s    (secondi per arrivare vicino a regime)
```

**Esempio:**
```
CORRENTE_SPUNTO = 7.2A
CORRENTE_STABILIZZAZIONE = 5.4A
CORRENTE_REGIME = 5.1A
TEMPO_STABILIZZAZIONE = 45 secondi
```

#### 3.4 Analisi

**Calcola il numero di campioni per stabilizzazione:**
```
N_campioni = TEMPO_STABILIZZAZIONE / 2 secondi
           = 45 / 2 = 22.5 ≈ 23 campioni
```

**Azione:** Se vuoi che EWMA si stabilizzi in questo tempo:
```cpp
// Formula: α = 3 / N_campioni
EWMA_ALPHA = 3 / 23 ≈ 0.13

// Arrotonda a valore pratico
constexpr double EWMA_ALPHA = 0.10;  // Più veloce per seguire transitorio
```

**Verifica spike detection:**
- ✅ Se vedi "Spike prolungato" durante accensione → È OK, è normale
- ❌ Se NON vuoi allarmi durante startup → Aumenta `STARTUP_SAMPLES`:
```cpp
constexpr size_t STARTUP_SAMPLES = 30; // Da 20 a 30
```

---

### FASE 4: Test Regime Normale - Lavoro Continuo (15 minuti)

**Obiettivo:** Misurare la variabilità durante operazione normale

#### 4.1 Esecuzione
- ✅ Pompa in funzione normale (acqua presente, profondità tipica)
- ✅ Lascia lavorare per 15 minuti
- ✅ Registra continuamente

#### 4.2 Cosa osservare

**Operazione stabile:**
```
T=5min:  PumpMonitor: current: 5.123, ewma: 5.100, diff: 0.023, threshold: 0.400
T=6min:  PumpMonitor: current: 5.098, ewma: 5.099, diff: -0.001, threshold: 0.400
T=7min:  PumpMonitor: current: 5.156, ewma: 5.102, diff: 0.054, threshold: 0.400
T=8min:  PumpMonitor: current: 5.089, ewma: 5.101, diff: -0.012, threshold: 0.400
```

**Piccole oscillazioni normali:**
```
T=10min: PumpMonitor: current: 5.234, ewma: 5.108, diff: 0.126, threshold: 0.400
T=11min: PumpMonitor: current: 4.987, ewma: 5.102, diff: -0.115, threshold: 0.400
T=12min: PumpMonitor: current: 5.145, ewma: 5.104, diff: 0.041, threshold: 0.400
```

#### 4.3 Dati da raccogliere

Analizza i log e calcola:

```
CORRENTE_MEDIA = ___________ A
CORRENTE_MIN = ___________ A
CORRENTE_MAX = ___________ A
VARIABILITA = (MAX - MIN) / 2 = ___________ A

DIFF_TIPICO = ___________ A      (valore tipico di |diff|)
DIFF_MAX = ___________ A         (massimo |diff| osservato)

THRESHOLD_NEI_LOG = ___________ A (valore tipico di 'threshold')
```

**Esempio:**
```
CORRENTE_MEDIA = 5.105A
CORRENTE_MIN = 4.987A
CORRENTE_MAX = 5.234A
VARIABILITA = (5.234 - 4.987) / 2 = 0.124A

DIFF_TIPICO = 0.05A
DIFF_MAX = 0.145A

THRESHOLD_NEI_LOG = 0.400A
```

#### 4.4 Analisi con Script

```bash
# Estrai statistiche dettagliate
grep "PumpMonitor: current:" calibration_test_*.log | \
  awk '{
    current=$3; 
    ewma=$5; 
    diff=$7; 
    threshold=$9;
    
    # Rimuovi virgole
    gsub(/,/, "", current);
    gsub(/,/, "", ewma);
    gsub(/,/, "", diff);
    
    sum_current += current;
    sum_diff += (diff < 0 ? -diff : diff);
    
    if(current > max_current) max_current = current;
    if(current < min_current || min_current == 0) min_current = current;
    if((diff < 0 ? -diff : diff) > max_diff) max_diff = (diff < 0 ? -diff : diff);
    
    count++;
  }
  END {
    print "=== STATISTICHE REGIME NORMALE ===";
    print "Corrente media:", sum_current/count, "A";
    print "Corrente min:", min_current, "A";
    print "Corrente max:", max_current, "A";
    print "Variabilità:", (max_current - min_current)/2, "A";
    print "";
    print "|diff| medio:", sum_diff/count, "A";
    print "|diff| max:", max_diff, "A";
    print "";
    print "Campioni analizzati:", count;
  }'
```

#### 4.5 Verifica Allarmi

**Conta gli allarmi durante operazione normale:**
```bash
grep -i "deviazione prolungata\|spike prolungato" calibration_test_*.log | wc -l
```

**Target:** **0 allarmi** durante operazione normale!

- ✅ 0 allarmi → Configurazione ottima
- ⚠️ 1-2 allarmi → Accettabile, ma puoi migliorare
- ❌ >3 allarmi → baselineK troppo basso o EWMA_ALPHA troppo alto

---

### FASE 5: Test Variazioni di Carico (10 minuti)

**Obiettivo:** Verificare come risponde a cambiamenti normali

#### 5.1 Esecuzione

Simula condizioni normali ma variabili:

**Test A: Livello Acqua Calante (5 min)**
- ✅ Lascia la pompa lavorare mentre l'acqua si abbassa
- ✅ La corrente dovrebbe aumentare gradualmente
- ✅ Osserva come EWMA segue il cambiamento

**Test B: Accensione/Spegnimento Rapidi (5 min)**
- ✅ Spegni la pompa
- ✅ Aspetta 30 secondi
- ✅ Riaccendi
- ✅ Ripeti 2-3 volte

#### 5.2 Cosa osservare

**Livello acqua che scende:**
```
T=0:  current: 5.100, ewma: 5.100, diff: 0.000  ← Inizio
T=2m: current: 5.234, ewma: 5.107, diff: 0.127  ← Inizia ad aumentare
T=4m: current: 5.456, ewma: 5.124, diff: 0.332  ← Aumenta ancora
T=6m: current: 5.612, ewma: 5.149, diff: 0.463  ← Supera threshold? ⚠️
```

**Azione:** Se supera threshold durante un cambio normale:
```cpp
// Aumenta baselineK per tollerare cambi graduali
const double baselineK = 5.0;  // Da 4.0 a 5.0
```

**Accensione/spegnimento ripetuti:**
```
T=0:  current: 5.100, ewma: 5.100, diff: 0.000
[SPEGNI]
T=1:  current: 0.001, ewma: 4.845, diff: -4.844  ← Grande diff, normale!
T=30: current: 0.001, ewma: 2.450, diff: -2.449  ← EWMA sta scendendo
[ACCENDI]
T=31: current: 6.234, ewma: 2.639, diff: 3.595  ← Spike accensione
T=45: current: 5.123, ewma: 3.563, diff: 1.560  ← Si stabilizza
```

**Azione:** Se vuoi EWMA più veloce a seguire ON/OFF:
```cpp
constexpr double EWMA_ALPHA = 0.10;  // Da 0.05 a 0.10
```

---

### FASE 6: Test Anomalia Simulata (5 minuti)

**Obiettivo:** Verificare che rilevi anomalie reali

#### 6.1 Esecuzione

Simula un'anomalia controllata (scegli una):

**Opzione A: Riduzione Livello Acqua**
- ✅ Lascia la pompa andare quasi a secco (controllato!)
- ✅ La corrente dovrebbe calare o aumentare

**Opzione B: Ostruzione Parziale** (se sicuro)
- ✅ Chiudi parzialmente una valvola
- ✅ La corrente dovrebbe aumentare

**Opzione C: Carico Aggiuntivo**
- ✅ Aggiungi resistenza al circuito idraulico
- ✅ La corrente dovrebbe cambiare

#### 6.2 Cosa osservare

**Target: Deve rilevare l'anomalia!**

```
[CONDIZIONE NORMALE]
T=0: current: 5.100, ewma: 5.100, diff: 0.023, threshold: 0.400

[INIZIA ANOMALIA - es. ostruzione]
T=30s:  current: 5.456, ewma: 5.118, diff: 0.338, threshold: 0.400
T=60s:  current: 5.612, ewma: 5.143, diff: 0.469, threshold: 0.410  ← Supera!
T=70s:  WARN | PumpMonitor: Deviazione prolungata dalla baseline rilevata!  ✅
```

#### 6.3 Dati da raccogliere

```
TEMPO_RILEVAMENTO = ___________ s    (secondi dall'inizio anomalia all'allarme)
CORRENTE_ANOMALA = ___________ A
DIFFERENZA_DA_NORMALE = ___________ A
```

#### 6.4 Analisi

**Tempo di rilevamento accettabile?**
- ✅ <60s → Ottimo (α probabilmente ≥0.10)
- ✅ 60-120s → Buono (α probabilmente ~0.05)
- ⚠️ 120-300s → Lento ma OK (α probabilmente ~0.02)
- ❌ >300s → Troppo lento! Aumenta α

**Azione se troppo lento:**
```cpp
constexpr double EWMA_ALPHA = 0.10;  // Aumenta da 0.05
```

**Azione se NON rileva:**
```cpp
const double baselineK = 3.0;  // Diminuisci da 4.0
```

---

## 📊 Analisi Finale e Decisioni

### Step 1: Compila la Tabella dei Risultati

```
┌────────────────────────────────────────────────────────────────┐
│ RISULTATI TEST CALIBRAZIONE                                    │
├────────────────────────────────────────────────────────────────┤
│ FASE 2: Pompa Spenta                                           │
│   Rumore max:           _________ A                            │
│   Rumore tipico:        _________ A                            │
│                                                                 │
│ FASE 3: Accensione (Transitorio)                               │
│   Corrente spunto:      _________ A                            │
│   Tempo stabilizz.:     _________ s                            │
│   Corrente regime:      _________ A                            │
│                                                                 │
│ FASE 4: Regime Normale                                         │
│   Corrente media:       _________ A                            │
│   Variabilità:          _________ A                            │
│   |diff| tipico:        _________ A                            │
│   |diff| max:           _________ A                            │
│   Threshold tipico:     _________ A                            │
│   Falsi allarmi:        _________ (target: 0)                  │
│                                                                 │
│ FASE 6: Test Anomalia                                          │
│   Tempo rilevamento:    _________ s                            │
│   Anomalia rilevata:    [ ] SÌ  [ ] NO                         │
└────────────────────────────────────────────────────────────────┘
```

### Step 2: Calcola Parametri Ottimali

#### MIN_CURRENT_THRESHOLD

```
MIN_CURRENT_THRESHOLD = RUMORE_MAX × 2
                      = _________ × 2 = _________ A

Arrotonda a: 0.05, 0.10, 0.15, 0.20 A
```

**Raccomandazione:**
```cpp
constexpr double MIN_CURRENT_THRESHOLD = _________; // Inserisci valore
```

---

#### EWMA_ALPHA

**Basato sul tempo di stabilizzazione desiderato:**

```
Opzione A - Veloce (segue transitorio rapidamente):
  N_campioni = TEMPO_STABILIZZAZIONE / 2
             = _________ / 2 = _________
  α = 3 / N_campioni = 3 / _________ = _________
  Arrotonda a: 0.10

Opzione B - Bilanciato:
  α = 0.05 (default)

Opzione C - Lento (filtra meglio il rumore):
  α = 0.02 o 0.03
```

**Raccomandazione:**
```cpp
constexpr double EWMA_ALPHA = _________; // 0.02, 0.05, o 0.10
```

---

#### baselineK

**Basato sui falsi allarmi e rilevamento anomalie:**

```
Se FALSI_ALLARMI > 0 durante fase 4:
  → baselineK = THRESHOLD_TIPICO / DIFF_MAX + 1.0
              = _________ / _________ + 1.0
              = _________

Se ANOMALIA NON RILEVATA in fase 6:
  → baselineK = THRESHOLD_TIPICO / DIFF_ANOMALIA
              = _________ / _________
              = _________

Altrimenti mantieni: baselineK = 4.0
```

**Raccomandazione:**
```cpp
const double baselineK = _________; // 3.0, 4.0, 5.0, o 6.0
```

---

#### EWMA_VAR_ALPHA

**Regola: β = α / 2 (approssimazione)**

```
EWMA_VAR_ALPHA = EWMA_ALPHA / 2
                = _________ / 2 = _________

Arrotonda a: 0.01, 0.02, 0.05
```

**Raccomandazione:**
```cpp
constexpr double EWMA_VAR_ALPHA = _________; // α/2
```

---

#### minStdFloor

**Basato sulla variabilità osservata:**

```
minStdFloor = VARIABILITA × 0.5
            = _________ × 0.5 = _________

Arrotonda a: 0.03, 0.05, 0.10 A
```

**Raccomandazione:**
```cpp
const double minStdFloor = _________; // 0.05 tipicamente
```

---

### Step 3: Configurazione Finale

**Inserisci i valori calcolati in `PumpMonitor.h`:**

```cpp
// === PARAMETRI CALIBRATI - [Data: __________] ===
constexpr size_t HISTORY_SIZE = 256;
constexpr size_t STARTUP_SAMPLES = 20;  // Aumenta a 30 se troppi allarmi all'accensione
constexpr double SPIKE_K = 6.0;
constexpr double EWMA_ALPHA = _________;     // [INSERISCI QUI]
constexpr double EWMA_VAR_ALPHA = _________;  // [INSERISCI QUI]
constexpr uint32_t PAUSE_THRESHOLD_MS = 500;
constexpr uint32_t DWELL_TIME_MS = 2000;
constexpr double MIN_CURRENT_THRESHOLD = _________; // [INSERISCI QUI]

// In classe PumpMonitor:
const double baselineK = _________;        // [INSERISCI QUI]
const double minStdFloor = _________;      // [INSERISCI QUI]
```

---

## ✅ Checklist Post-Calibrazione

Dopo aver applicato i nuovi parametri, **ripeti i test**:

- [ ] Test 15 minuti operazione normale → 0 falsi allarmi
- [ ] Test accensione → Stabilizzazione in tempo accettabile
- [ ] Test anomalia simulata → Rileva in <2 minuti
- [ ] Test spegnimento → Non genera allarmi continui
- [ ] Osserva per 1-2 ore → Comportamento stabile

---

## 📝 Template per Documentare i Test

Crea un file `CALIBRATION_LOG.md` per documentare i risultati:

```markdown
# Log Calibrazione PumpMonitor

**Data test:** __________
**Pompa:** [Modello/Potenza]
**Sensore:** SCT-013-030
**Frequenza campionamento:** 2 secondi

## Configurazione Iniziale
- EWMA_ALPHA: 0.05
- EWMA_VAR_ALPHA: 0.02
- baselineK: 4.0
- MIN_CURRENT_THRESHOLD: 0.05
- minStdFloor: 0.05

## Risultati Test

### Fase 2: Pompa Spenta
- Rumore max: _____ A
- Rumore tipico: _____ A

### Fase 3: Transitorio Accensione
- Corrente spunto: _____ A
- Tempo stabilizzazione: _____ s
- Corrente regime: _____ A

### Fase 4: Regime Normale (15 min)
- Corrente media: _____ A
- Variabilità: _____ A
- |diff| max: _____ A
- Falsi allarmi: _____

### Fase 6: Test Anomalia
- Tipo anomalia: [descrivi]
- Tempo rilevamento: _____ s
- Rilevata: [SÌ/NO]

## Configurazione Finale Ottimizzata
- EWMA_ALPHA: _____
- EWMA_VAR_ALPHA: _____
- baselineK: _____
- MIN_CURRENT_THRESHOLD: _____
- minStdFloor: _____

## Note
[Annotazioni particolari, comportamenti osservati, ecc.]
```

---

## 🎯 Esempio Completo di Calibrazione

### Caso Pratico: Pompa Sommersa 1HP

**Test eseguiti:**
- Data: 10 Novembre 2024
- Durata totale: 45 minuti
- Condizioni: Livello acqua normale

**Risultati:**
```
Rumore max: 0.003A
Corrente spunto: 7.2A
Tempo stabilizzazione: 45s (≈23 campioni)
Corrente regime media: 5.1A
Variabilità: ±0.12A
|diff| max durante normale: 0.15A
Falsi allarmi con config iniziale: 3
Anomalia simulata rilevata in: 95s
```

**Calcoli:**
```
MIN_CURRENT_THRESHOLD = 0.003 × 2 = 0.006 → Arrotondo a 0.05A ✓

EWMA_ALPHA = 3 / 23 = 0.13 → Arrotondo a 0.10 (più veloce)

baselineK: Con 3 falsi allarmi → Aumento a 5.0
  Verifica: threshold = 5.0 × 0.1 = 0.5A
  |diff| max = 0.15A < 0.5A ✓ OK

EWMA_VAR_ALPHA = 0.10 / 2 = 0.05

minStdFloor = 0.12 × 0.5 = 0.06 → Arrotondo a 0.05A
```

**Configurazione finale:**
```cpp
constexpr double EWMA_ALPHA = 0.10;      // Per seguire transitorio
constexpr double EWMA_VAR_ALPHA = 0.05;  
constexpr double MIN_CURRENT_THRESHOLD = 0.05;
const double baselineK = 5.0;            // Ridotto falsi allarmi
const double minStdFloor = 0.05;
```

**Risultati dopo riapplicazione:**
- ✅ Falsi allarmi: 0
- ✅ Rilevamento anomalia: 75s (migliorato da 95s)
- ✅ Stabilizzazione: 40s (migliorato da 45s)

---

## 🚀 Quick Start: Test Rapido (5 minuti)

Se hai poco tempo, fai almeno questo test base:

```bash
# 1. Pompa SPENTA - 1 minuto
#    Osserva il rumore di fondo

# 2. ACCENDI pompa - 2 minuti
#    Conta i secondi per stabilizzazione

# 3. Operazione NORMALE - 2 minuti
#    Conta eventuali falsi allarmi

# 4. Analisi veloce
grep "PumpMonitor: current:" log.txt | tail -60 | \
  awk '{print $7}' | \
  awk 'BEGIN {max=0} 
       {v=$1; if(v<0) v=-v; if(v>max) max=v} 
       END {print "Max |diff|:", max}'

# Se max|diff| > threshold → Aumenta baselineK
# Se max|diff| << threshold → Potresti diminuire baselineK
```

---

**Buona calibrazione!** 🎯

Ricorda: la calibrazione è un processo iterativo. Non aspettarti la perfezione al primo tentativo!

