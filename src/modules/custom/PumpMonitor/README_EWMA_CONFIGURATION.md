# 📊 Guida alla Configurazione EWMA del PumpMonitor

## Indice
1. [Matematica dell'EWMA](#matematica-dellewma)
2. [Parametri di Configurazione](#parametri-di-configurazione)
3. [Come Scegliere i Valori](#come-scegliere-i-valori)
4. [Calibrazione e Testing](#calibrazione-e-testing)
5. [Esempi Pratici](#esempi-pratici)

---

## 📐 Matematica dell'EWMA

### 1. EWMA (Exponentially Weighted Moving Average)

**Codice (righe 271-272 di PumpMonitor.h):**
```cpp
double delta = current - ewma;
ewma += EWMA_ALPHA * delta;
```

**Formula matematica:**
```
ewma(t) = ewma(t-1) + α × [current(t) - ewma(t-1)]
        = (1 - α) × ewma(t-1) + α × current(t)
```

**Dove:**
- `α = EWMA_ALPHA` = peso del nuovo valore (default: **0.05** = 5%)
- `1-α` = peso della storia passata (default: **0.95** = 95%)

**Significato:**
- EWMA è una **media mobile esponenziale** che segue i cambiamenti graduali
- Con α=0.05, ogni nuovo campione influenza solo il 5% del valore EWMA
- Il 95% rimane dalla "memoria" dei campioni precedenti
- Valori più alti di α → risposta più veloce ai cambiamenti
- Valori più bassi di α → maggiore stabilità e filtro del rumore

---

### 2. Varianza EWMA

**Codice (riga 274 di PumpMonitor.h):**
```cpp
ewmaVar += EWMA_VAR_ALPHA * (delta*delta - ewmaVar);
```

**Formula matematica:**
```
ewmaVar(t) = (1 - β) × ewmaVar(t-1) + β × [current(t) - ewma(t)]²
```

**Dove:**
- `β = EWMA_VAR_ALPHA` = peso per la varianza (default: **0.02** = 2%)
- `delta² = [current - ewma]²` = errore quadratico istantaneo

**Deviazione Standard (riga 276):**
```cpp
double ewmaStd = std::sqrt(std::max(0.0, ewmaVar));
```

**Significato:**
- `ewmaVar` stima la variabilità della corrente attorno alla baseline EWMA
- `ewmaStd` è la deviazione standard, usata per definire le soglie di anomalia
- Valori più alti di β → la stima della variabilità si adatta più velocemente

---

### 3. Rilevamento Anomalie

**Codice (righe 325-328 di PumpMonitor.h):**
```cpp
double diffFromBaseline = current - ewma;
bool deviateHigh = (diffFromBaseline > baselineK * std::max(ewmaStd, minStdFloor));
bool deviateLow  = (diffFromBaseline < -baselineK * std::max(ewmaStd, minStdFloor));
```

**Formula:**
```
Anomalia se: |current - ewma| > baselineK × max(ewmaStd, minStdFloor)
```

**Dove:**
- `baselineK` = quanti "sigma" di distanza considerare anomalo (default: **4.0**)
- `minStdFloor` = soglia minima per evitare divisione per 0 (default: **0.01**)

**Significato:**
- Il sistema considera anomalo un valore che si discosta di più di K deviazioni standard dalla baseline
- baselineK=4.0 significa che solo ~0.01% dei valori normali causerà un falso allarme
- minStdFloor evita che soglie troppo basse causino falsi allarmi quando la pompa è molto stabile

---

## ⚙️ Parametri di Configurazione

### EWMA_ALPHA (α) - Velocità di Adattamento della Media

Controlla quanto velocemente EWMA **segue i cambiamenti graduali**:

| Valore | Velocità | "Memoria" Efficace | Tempo di Adattamento (95%) |
|--------|----------|-------------------|----------------------------|
| **0.01** | 🐢 Molto lento | ~100 campioni | ~300 campioni (~10 min) |
| **0.05** | 🚶 Moderato | ~20 campioni | ~60 campioni (~2 min) |
| **0.10** | 🏃 Veloce | ~10 campioni | ~30 campioni (~1 min) |
| **0.20** | ⚡ Molto veloce | ~5 campioni | ~15 campioni (~30 sec) |

**Formula per calcolare la memoria efficace:**
```
Numero campioni per 95% peso = -ln(0.05) / α ≈ 3 / α
```

**Esempio con α=0.05:**
```
Memoria = 3 / 0.05 = 60 campioni
Con campionamento ogni 2 secondi = 120 secondi = 2 minuti
```

**Quando aumentare α:**
- ✅ Vuoi rilevare degrado pompa più velocemente
- ✅ La corrente normale varia rapidamente
- ✅ Preferisci qualche falso positivo in più ma maggiore reattività

**Quando diminuire α:**
- ✅ Vuoi filtrare meglio il rumore e gli spike temporanei
- ✅ La corrente normale è molto stabile
- ✅ Preferisci evitare falsi allarmi

---

### EWMA_VAR_ALPHA (β) - Velocità di Adattamento della Varianza

Controlla quanto velocemente **ewmaStd** si adatta ai cambiamenti di variabilità:

| Valore | Velocità | Uso Raccomandato |
|--------|----------|------------------|
| **0.01** | 🐢 Lento | Varianza molto stabile nel tempo |
| **0.02** | 🚶 Moderato | Bilanciato (raccomandato) |
| **0.05** | 🏃 Veloce | Varianza può cambiare rapidamente |

**Regola empirica:** 
```
β ≤ α  (solitamente β = α/2 o β = α/3)
```

**Perché β dovrebbe essere ≤ α:**
- La varianza è una statistica del secondo ordine (cambia più lentamente della media)
- Un β troppo alto rende le soglie instabili
- Un β troppo basso rallenta l'adattamento a cambi di regime della pompa

---

### baselineK - Sensibilità al Rilevamento

Controlla **quante deviazioni standard** di distanza considerare anomalia:

| Valore | Sensibilità | Falsi Positivi | Falsi Negativi | Uso |
|--------|-------------|----------------|----------------|-----|
| **2.0** | 🔴 Molto alta | Molti (~5%) | Pochissimi | Solo per pompe critiche |
| **3.0** | 🟡 Alta | Alcuni (~0.3%) | Pochi | Rilevamento aggressivo |
| **4.0** | 🟢 Normale | Pochi (~0.01%) | Alcuni | **Raccomandato** |
| **5.0** | 🔵 Bassa | Pochissimi (~0.0001%) | Alcuni | Pochi falsi allarmi |
| **6.0** | ⚪ Molto bassa | Quasi nessuno | Molti | Solo anomalie evidenti |

**Statistica (distribuzione normale):**
- K=2 → Rileva valori oltre il 95.5% della distribuzione normale
- K=3 → Rileva valori oltre il 99.7% (regola dei 3-sigma)
- K=4 → Rileva valori oltre il 99.99%
- K=5 → Rileva valori oltre il 99.9999%

**Quando diminuire baselineK:**
- ✅ Degrado pompa è graduale e vuoi rilevarlo presto
- ✅ Puoi tollerare qualche falso allarme
- ✅ La corrente normale ha poca variabilità

**Quando aumentare baselineK:**
- ✅ La corrente normale ha molta variabilità
- ✅ Falsi allarmi sono problematici (es. notifiche SMS costose)
- ✅ Vuoi rilevare solo anomalie molto evidenti

---

### minStdFloor - Soglia Minima di Deviazione Standard

Previene soglie troppo basse quando la pompa è molto stabile:

| Valore | Uso | Effetto |
|--------|-----|---------|
| **0.01** | 10mA | Soglia molto bassa (sensibile anche a piccole variazioni) |
| **0.05** | 50mA | **Raccomandato** - Ignora rumore elettrico tipico |
| **0.10** | 100mA | Soglia alta (solo variazioni significative) |

**Formula della soglia di anomalia:**
```
threshold = baselineK × max(ewmaStd, minStdFloor)
```

**Esempio:**
```
Se ewmaStd = 0.02A (molto stabile)
   baselineK = 4.0
   minStdFloor = 0.05A

threshold = 4.0 × max(0.02, 0.05) = 4.0 × 0.05 = 0.2A

→ Anomalia se |current - ewma| > 0.2A
```

---

## 🎯 Come Scegliere i Valori

### Scenario 1: Pompa Molto Stabile (±0.1A di variazione normale)

**Configurazione:**
```cpp
constexpr double EWMA_ALPHA = 0.05;      // Adattamento moderato
constexpr double EWMA_VAR_ALPHA = 0.02;  // Varianza stabile
const double baselineK = 3.0;            // Più sensibile (3 sigma)
const double minStdFloor = 0.05;         // 50mA minimo
```

**Perché:**
- Poca variabilità normale → puoi essere più sensibile (K=3.0)
- α moderato è sufficiente per seguire cambiamenti graduali
- Rileverà anomalie anche piccole senza troppi falsi allarmi

---

### Scenario 2: Pompa Instabile (±0.5A di variazione normale)

**Configurazione:**
```cpp
constexpr double EWMA_ALPHA = 0.10;      // Adattamento veloce
constexpr double EWMA_VAR_ALPHA = 0.05;  // Varianza può cambiare
const double baselineK = 5.0;            // Meno sensibile (5 sigma)
const double minStdFloor = 0.10;         // 100mA minimo
```

**Perché:**
- Molta variabilità normale → devi essere meno sensibile (K=5.0)
- α alto per seguire le oscillazioni normali
- Evita falsi allarmi dovuti alla variabilità intrinseca

---

### Scenario 3: Rilevamento RAPIDO (pompa può danneggiarsi velocemente)

**Configurazione:**
```cpp
constexpr double EWMA_ALPHA = 0.20;      // Risponde in ~5-15 campioni
constexpr double EWMA_VAR_ALPHA = 0.10;  // Varianza si adatta velocemente
const double baselineK = 3.0;            // Sensibile
const double minStdFloor = 0.05;         // 50mA minimo
```

**Tempo di rilevamento:** ~10-30 secondi

**Compromesso:** Più falsi positivi, ma rileva problemi quasi istantaneamente

---

### Scenario 4: Rilevamento LENTO (pochi falsi allarmi)

**Configurazione:**
```cpp
constexpr double EWMA_ALPHA = 0.02;      // Risponde in ~150 campioni
constexpr double EWMA_VAR_ALPHA = 0.01;  // Varianza molto stabile
const double baselineK = 5.0;            // Poco sensibile
const double minStdFloor = 0.05;         // 50mA minimo
```

**Tempo di rilevamento:** ~5-10 minuti

**Compromesso:** Pochissimi falsi positivi, ma più lento a rilevare

---

### Scenario 5: Configurazione BILANCIATA (Raccomandato per Pompa Pozzo)

**Configurazione:**
```cpp
constexpr double EWMA_ALPHA = 0.05;      // ~60 campioni di memoria (2 min)
constexpr double EWMA_VAR_ALPHA = 0.02;  // Varianza stabile
const double baselineK = 4.0;            // 99.99% soglia statistica
const double minStdFloor = 0.05;         // 50mA minimo
```

**Caratteristiche:**
- ✅ Rileva degrado in ~1-2 minuti
- ✅ Pochi falsi positivi (~0.01%)
- ✅ Ignora rumore <50mA quando pompa spenta
- ✅ Adatto per la maggior parte dei casi d'uso

---

## 🧪 Calibrazione e Testing

### Passo 1: Raccolta Dati Durante Operazione Normale

Abilita il log di monitoraggio (già presente alla riga 332):
```cpp
if (now - lastPrint > 100) {
   LOG_INFO("PumpMonitor: current: %.3f, ewma: %.3f, diff: %.3f threshold: %.3f", 
            current, ewma, diffFromBaseline, baselineK * std::max(ewmaStd, minStdFloor));
   lastPrint = now;
}
```

### Passo 2: Analisi dei Log

Lascia girare per 10-15 minuti durante operazione normale e osserva:

```
PumpMonitor: current: 5.123, ewma: 5.100, diff: 0.023, threshold: 0.400
PumpMonitor: current: 5.098, ewma: 5.099, diff: -0.001, threshold: 0.400
PumpMonitor: current: 5.145, ewma: 5.101, diff: 0.044, threshold: 0.400
```

**Estrai informazioni:**

1. **Variabilità normale della corrente:**
   ```
   diff tipico = ±0.02 a ±0.05A  → Variabilità bassa/normale
   ```

2. **Deviazione standard stimata:**
   ```
   ewmaStd = threshold / baselineK = 0.400 / 4.0 = 0.1A
   ```

3. **Massima variazione normale:**
   ```
   Cerca il max(|diff|) durante operazione normale
   Esempio: max_diff = 0.08A
   ```

### Passo 3: Calcola Parametri Ottimali

**baselineK ottimale:**
```
baselineK = (max_diff_desiderato / ewmaStd) + margine_sicurezza
          = (0.15 / 0.1) + 1.5
          = 3.0
```

Dove:
- `max_diff_desiderato` = massima variazione da considerare normale
- `margine_sicurezza` = 1.0-2.0 (per evitare falsi allarmi)

**EWMA_ALPHA ottimale:**
```
α = 3 / numero_campioni_desiderato
```

Esempi:
- Vuoi rilevamento in 30 campioni (1 min) → α = 3/30 = 0.10
- Vuoi rilevamento in 60 campioni (2 min) → α = 3/60 = 0.05
- Vuoi rilevamento in 100 campioni (3.3 min) → α = 3/100 = 0.03

### Passo 4: Valida con Test

**Test 1: Operazione Normale**
- ❌ Nessun allarme dovrebbe scattare durante operazione normale
- Se scattano allarmi → Aumenta `baselineK` o diminuisci `EWMA_ALPHA`

**Test 2: Simulazione Anomalia Graduale**
- Riduci leggermente il livello dell'acqua per aumentare la corrente
- ✅ Dovrebbe rilevare in tempo ragionevole (1-5 minuti)
- Se non rileva → Diminuisci `baselineK` o aumenta `EWMA_ALPHA`

**Test 3: Simulazione Spike Transiente**
- Crea una perturbazione breve (1-2 secondi)
- ✅ Dovrebbe ignorare spike transienti (grazie a `spikeTransientLimit`)
- ❌ Non dovrebbe causare allarme prolungato

---

## 📊 Esempi Pratici

### Esempio 1: Interpretazione Log Normale

```
PumpMonitor: current: 5.123, ewma: 5.100, diff: 0.023, threshold: 0.400
```

**Analisi:**
- Corrente attuale: 5.123A
- Baseline EWMA: 5.100A
- Deviazione: +0.023A (entro soglia)
- Soglia anomalia: ±0.400A
- **Stato:** ✅ Normale (0.023 << 0.400)

---

### Esempio 2: Anomalia in Corso

```
PumpMonitor: current: 5.612, ewma: 5.100, diff: 0.512, threshold: 0.400
WARN | PumpMonitor: Deviazione prolungata dalla baseline rilevata!
```

**Analisi:**
- Corrente attuale: 5.612A
- Baseline EWMA: 5.100A  
- Deviazione: +0.512A (supera soglia!)
- Soglia anomalia: ±0.400A
- **Stato:** ⚠️ Anomalia (0.512 > 0.400)

**Possibili cause:**
- Pompa sotto sforzo (livello acqua basso)
- Degrado meccanico
- Ostruzione parziale

---

### Esempio 3: Calibrazione Dinamica

**Situazione iniziale:**
```cpp
// Configurazione iniziale
EWMA_ALPHA = 0.05
baselineK = 4.0
```

**Log osservati:**
```
PumpMonitor: current: 5.123, ewma: 5.100, diff: 0.023, threshold: 0.400
PumpMonitor: current: 5.089, ewma: 5.099, diff: -0.010, threshold: 0.400
PumpMonitor: current: 5.234, ewma: 5.105, diff: 0.129, threshold: 0.400
PumpMonitor: current: 5.441, ewma: 5.115, diff: 0.326, threshold: 0.400  ← Vicino alla soglia
WARN | PumpMonitor: Deviazione prolungata dalla baseline rilevata!
```

**Problema:** Falso allarme - la corrente varia normalmente fino a ±0.35A

**Soluzione:**
```cpp
// Aumenta baselineK per tollerare maggiore variabilità
const double baselineK = 5.0;  // Era 4.0

// Ora threshold = 5.0 × 0.1 = 0.500A
// diff = 0.326A < 0.500A → Nessun allarme
```

---

## 🔧 Troubleshooting

### Problema: Troppi Falsi Allarmi

**Sintomo:**
```
WARN | PumpMonitor: Deviazione prolungata dalla baseline rilevata!
```
Ma la pompa funziona normalmente.

**Soluzioni (in ordine di priorità):**

1. **Aumenta baselineK:**
   ```cpp
   const double baselineK = 5.0;  // Da 4.0 a 5.0
   ```

2. **Diminuisci EWMA_ALPHA** (più filtraggio):
   ```cpp
   constexpr double EWMA_ALPHA = 0.03;  // Da 0.05 a 0.03
   ```

3. **Aumenta minStdFloor:**
   ```cpp
   const double minStdFloor = 0.10;  // Da 0.05 a 0.10
   ```

---

### Problema: Non Rileva Anomalie Reali

**Sintomo:**
La pompa ha problemi evidenti ma non viene rilevato.

**Soluzioni (in ordine di priorità):**

1. **Diminuisci baselineK:**
   ```cpp
   const double baselineK = 3.0;  // Da 4.0 a 3.0
   ```

2. **Aumenta EWMA_ALPHA** (risposta più veloce):
   ```cpp
   constexpr double EWMA_ALPHA = 0.10;  // Da 0.05 a 0.10
   ```

3. **Diminuisci minStdFloor:**
   ```cpp
   const double minStdFloor = 0.03;  // Da 0.05 a 0.03
   ```

---

### Problema: EWMA Troppo Lento ad Adattarsi

**Sintomo:**
Dopo un cambio regime (es. pompa accesa/spenta), EWMA impiega troppo tempo.

**Soluzioni:**

1. **Aumenta EWMA_ALPHA:**
   ```cpp
   constexpr double EWMA_ALPHA = 0.10;  // Da 0.05 a 0.10
   ```

2. **Usa reset manuale** quando cambia regime:
   ```cpp
   // Nel codice, dopo accensione pompa
   hasEwma = false;  // Forza reinizializzazione
   ```

---

### Problema: Soglie Troppo Instabili

**Sintomo:**
La threshold varia troppo rapidamente nei log.

**Soluzione:**

**Diminuisci EWMA_VAR_ALPHA:**
```cpp
constexpr double EWMA_VAR_ALPHA = 0.01;  // Da 0.02 a 0.01
```

Questo rallenta l'adattamento di `ewmaStd`, rendendo le soglie più stabili.

---

## 📋 Checklist di Configurazione

### ✅ Prima di Modificare i Parametri

- [ ] Ho raccolto almeno 10-15 minuti di log durante operazione normale
- [ ] Ho identificato la variabilità tipica della corrente (max |diff|)
- [ ] Ho calcolato ewmaStd medio dalla formula: ewmaStd = threshold / baselineK
- [ ] Ho identificato se la pompa è stabile o instabile

### ✅ Dopo Aver Modificato i Parametri

- [ ] Ho testato per almeno 30 minuti in operazione normale
- [ ] Non ci sono falsi allarmi durante operazione normale
- [ ] Ho simulato un'anomalia (es. riduzione livello acqua) e viene rilevata
- [ ] Il tempo di rilevamento è accettabile per il mio caso d'uso
- [ ] Ho documentato le modifiche e il motivo

---

## 📖 Riferimenti

### Formule Chiave

**Memoria efficace EWMA:**
```
N_samples = -ln(0.05) / α ≈ 3 / α
```

**Tempo di adattamento (con campionamento ogni 2 secondi):**
```
T_seconds = (3 / α) × 2
```

**Soglia di anomalia:**
```
threshold = baselineK × max(ewmaStd, minStdFloor)
```

**Probabilità falso positivo (distribuzione normale):**
```
P(falso_positivo) ≈ 2 × [1 - Φ(baselineK)]

Dove Φ è la funzione di distribuzione cumulativa normale standard:
K=2 → P ≈ 4.5%
K=3 → P ≈ 0.3%
K=4 → P ≈ 0.01%
K=5 → P ≈ 0.0001%
```

---

## 🎓 Approfondimenti Teorici

### Perché EWMA invece di Media Mobile Semplice?

**Media Mobile Semplice (SMA):**
```
SMA = (x₁ + x₂ + ... + xₙ) / n
```
- ❌ Tutti i campioni hanno lo stesso peso
- ❌ Richiede buffer di dimensione N
- ❌ Campioni vecchi "cadono" improvvisamente dalla finestra

**EWMA:**
```
EWMA(t) = α × x(t) + (1-α) × EWMA(t-1)
```
- ✅ Campioni recenti hanno più peso
- ✅ Richiede solo 1 variabile di stato
- ✅ Decadimento esponenziale graduale dei campioni vecchi
- ✅ Più efficiente in memoria e computazione

### Relazione tra α e Finestra Equivalente

EWMA con α è approssimativamente equivalente a una SMA con finestra:
```
N_equivalent ≈ 2/α - 1
```

Esempi:
- α=0.10 → N ≈ 19 campioni
- α=0.05 → N ≈ 39 campioni
- α=0.02 → N ≈ 99 campioni

---

## 🔍 Debug e Monitoraggio

### Log Dettagliati per Debug

Per debug intensivo, abilita questi log in PumpMonitor.h:

```cpp
// Riga 332 - Log stato EWMA (già abilitato)
LOG_INFO("PumpMonitor: current: %.3f, ewma: %.3f, diff: %.3f threshold: %.3f", 
         current, ewma, diffFromBaseline, baselineK * std::max(ewmaStd, minStdFloor));

// Riga 282 - Log durante startup (decommentare se serve)
printStats(current, med, approxStd, ewma, ewmaStd);

// Riga 310 - Log spike transienti (decommentare se serve)
LOG_INFO("PumpMonitor: Spike transiente rilevato (sample %d) valore=%.10f", spikeCounter, current);
```

### Metriche da Monitorare

1. **Variabilità normale:** Osserva `diff` durante operazione normale
2. **Deviazione standard:** Calcola `ewmaStd = threshold / baselineK`
3. **Tempo di adattamento:** Misura quanti campioni dopo un cambio regime
4. **Falsi positivi:** Conta allarmi durante operazione normale (target: 0)
5. **Falsi negativi:** Verifica che anomalie reali vengano rilevate

---

## 📞 Supporto

Per ulteriori informazioni o supporto sulla configurazione:

1. Raccogli log di almeno 15 minuti durante operazione normale
2. Documenta il comportamento indesiderato (troppi/pochi allarmi)
3. Specifica il caso d'uso e requisiti temporali di rilevamento
4. Fornisci esempi di log con valori current, ewma, diff, threshold

---

**Versione documento:** 1.0  
**Data:** Novembre 2024  
**Autore:** Sistema di monitoraggio pompa Meshtastic

