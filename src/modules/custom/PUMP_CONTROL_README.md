# Sistema di Controllo Pompa con Deviatore

## Panoramica

Questo sistema gestisce il controllo di una pompa tramite un relay configurato come **deviatore** (non interruttore), permettendo il controllo sia automatico che manuale esterno.

## Caratteristiche Principali

### 1. **Gestione Deviatore**
- Il relay invia **impulsi brevi** (200ms) per cambiare stato
- Lo stato può essere modificato sia dal software che manualmente
- Il sistema rileva automaticamente i cambiamenti esterni

### 2. **Monitoraggio Corrente**
- Utilizza `PumpMonitor` con sensore SCT-013-030 per rilevare lo stato reale
- Soglia corrente: **0.5A** (configurabile tramite `PUMP_ON_CURRENT_THRESHOLD`)
- Corrente > 0.5A = Pompa ON
- Corrente < 0.5A = Pompa OFF

### 3. **Stati della Pompa**

#### Variabili di Stato:
- `pumpDesiredState`: Stato che il software vuole (true=ON, false=OFF)
- `pumpActualState`: Stato reale rilevato dal monitor corrente
- `pumpExternalControl`: Flag che indica controllo esterno (true quando stato reale ≠ desiderato senza comando)

## Funzionamento

### Flusso di Controllo

```
┌─────────────────────────────────────────────────────┐
│  1. Lettura Livello Acqua                           │
│     waterLevelMilliVolts > 1.5V → Pompa ON          │
│     waterLevelMilliVolts ≤ 1.5V → Pompa OFF         │
└─────────────────┬───────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────┐
│  2. Confronto con Stato Desiderato                  │
│     Se diverso → setPumpState(shouldBeOn)           │
└─────────────────┬───────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────┐
│  3. setPumpState()                                   │
│     • Confronta desiderato vs attuale               │
│     • Se diversi: invia impulso relay (200ms)       │
│     • Se uguali: nessun comando                     │
└─────────────────┬───────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────┐
│  4. syncPumpState() - ogni 1 secondo                │
│     • updatePumpActualState()                       │
│     • Rileva corrente pompa                         │
│     • Aggiorna pumpActualState                      │
│     • Rileva controllo esterno                      │
└─────────────────────────────────────────────────────┘
```

### Rilevamento Controllo Esterno

Quando `updatePumpActualState()` rileva un cambio di stato:

1. **Cambio in linea con comando software**:
   - `pumpExternalControl = false`
   - Log: "Confermato cambio stato pompa"

2. **Cambio NON comandato (esterno)**:
   - `pumpExternalControl = true`
   - Log WARNING: "Rilevato cambio stato pompa ESTERNO"

### Display LCD (20x4)

```
Riga 0: "Pozzo"
Riga 1: "mV: 2.345 mt: 15.234"
Riga 2: "Pompa Watt: 245.5"
Riga 3: "Stato: ON [EXT]"  (mostra [EXT] se controllo esterno)
```

## API Metodi Pubblici

### Controllo Pompa

```cpp
// Imposta stato desiderato (invia impulso se necessario)
void setPumpState(bool turnOn);

// Ottieni stato reale corrente
bool isPumpOn();

// Controlla se pompa è sotto controllo esterno
bool isUnderExternalControl();

// Resetta flag controllo esterno e riprendi controllo automatico
void resetExternalControlFlag();
```

### Esempio Uso

```cpp
// Accendi pompa manualmente
pozzoModule->setPumpState(true);

// Controlla se pompa è accesa
if (pozzoModule->isPumpOn()) {
    LOG_INFO("Pompa attiva");
}

// Controlla se qualcuno ha cambiato lo stato dall'esterno
if (pozzoModule->isUnderExternalControl()) {
    LOG_WARN("Pompa sotto controllo esterno!");
    // Opzionale: riprendi controllo
    pozzoModule->resetExternalControlFlag();
}
```

## Configurazione

### Costanti (PozzoModule.cpp)

```cpp
#define PUMP_STATE_CHECK_INTERVAL_MS 1000   // Intervallo controllo stato (ms)
#define PUMP_RELAY_IMPULSE_MS 200           // Durata impulso relay (ms)
#define PUMP_ON_CURRENT_THRESHOLD 0.5f      // Soglia corrente ON (Ampere)
```

### Personalizzazione

1. **Soglia Corrente**: Modifica `PUMP_ON_CURRENT_THRESHOLD` in base alla pompa
2. **Durata Impulso**: Modifica `PUMP_RELAY_IMPULSE_MS` per relay più lenti/veloci
3. **Frequenza Controllo**: Modifica `PUMP_STATE_CHECK_INTERVAL_MS`

## Logging

### Messaggi Importanti

- `INFO`: "Confermato cambio stato pompa"
- `WARN`: "Rilevato cambio stato pompa ESTERNO"
- `WARN`: "Pompa sotto controllo esterno"
- `INFO`: "Livello acqua richiede pompa ON/OFF"
- `INFO`: "Impulso relay inviato"

## Sicurezza

### Protezioni Implementate

1. **Anti-rimbalzo**: Impulsi brevi evitano oscillazioni
2. **Rilevamento stato**: Monitor corrente conferma cambio stato
3. **Controllo esterno**: Sistema consapevole di modifiche manuali
4. **Threshold corrente**: Evita false rilevazioni con 0.5A

### Gestione Errori

- Se `pumpMonitor == NULL`: `updatePumpActualState()` esce silenziosamente
- Se stato desiderato ≠ reale per più cicli: Log debug (possibile transizione)
- Se controllo esterno attivo: Log warning continuo ogni secondo

## Limitazioni

1. **Delay nell'impulso**: Il comando `delay(200)` blocca il thread per 200ms
2. **Tempo di risposta**: Stato reale aggiornato ogni 1 secondo
3. **Soglia fissa**: La soglia 0.5A potrebbe non essere adatta a tutte le pompe
4. **Solo deviatore**: Non compatibile con relay bistabili o altri tipi

## Miglioramenti Futuri

1. ⚡ Usare timer non-bloccanti al posto di `delay()`
2. 📊 Aggiungere telemetria Meshtastic per stato pompa
3. 🔧 Rendere soglia corrente configurabile a runtime
4. ⏱️ Aggiungere timeout per rilevare malfunzionamenti relay
5. 🔔 Notifica push quando rileva controllo esterno

## Note Tecniche

### Hardware
- **Pin Relay**: GPIO 45 (`PIN_RELAY_PUMP`)
- **Sensore Corrente**: SCT-013-030 su canale 2-3 differenziale ADS1115
- **Display**: LCD I2C 20x4 (0x27)

### Timing
- Lettura livello acqua: ogni 5 secondi
- Lettura corrente pompa: continua quando non legge livello
- Controllo stato pompa: ogni 1 secondo
- Aggiornamento display: ogni 1 secondo

---

**Autore**: Sistema di Controllo Pozzo Meshtastic  
**Versione**: 1.0  
**Data**: Novembre 2025

