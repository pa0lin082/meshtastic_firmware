# ScriptingModule - Modulo di Scripting Dinamico per Meshtastic

## 🎯 Obiettivo

Il **ScriptingModule** risolve il problema di dover ricompilare e riprogrammare tutti i dispositivi Meshtastic ogni volta che si vuole modificare la logica di funzionamento. Permette l'esecuzione dinamica di script JavaScript direttamente sui dispositivi.

## 🏗️ Architettura

### Fase 1: Implementazione Base (Completata)
- ✅ Modulo base con script hardcoded
- ✅ 4 script di test predefiniti
- ✅ Struttura per integrazione Duktape
- ✅ API bindings preparati (da implementare)
- ✅ **Interprete semplificato per JavaScript**
- ✅ **Esecuzione reale di SCRIPT_IDLE**
- ✅ **Sistema di logging JavaScript**

### Fase 2: Integrazione Duktape (Prossima)
- 🔄 Motore JavaScript Duktape integrato
- 🔄 API bindings funzionanti
- 🔄 Esecuzione reale degli script

### Fase 3: Configurazione Dinamica (Futura)
- ⏳ Configurazione via protobuf
- ⏳ Caricamento script remoto
- ⏳ Hot-reload senza riavvio

## 📝 Script di Test Disponibili

### 1. Script DHT+ADC (Index 0)
```javascript
// Monitoraggio sensori DHT e ADC
const config = {
    dhtPin: 5,
    dhtType: 'DHT11', 
    adcPin: 6,
    interval: 30000
};

// Inizializza e legge sensori ogni 30 secondi
// Invia dati via mesh
```

### 2. Script GPIO Control (Index 1)
```javascript
// Controllo GPIO con LED e button
const config = {
    ledPin: 2,
    buttonPin: 0,
    interval: 1000
};

// Gestisce interrupt button
// Blink LED automatico
```

### 3. Script Idle (Index 2)
```javascript
// Script vuoto - modulo in idle
// Non fa nulla, solo log
```

### 4. Script Mesh Test (Index 3)
```javascript
// Test comunicazione mesh
// Invia messaggi test ogni 10 secondi
// Risponde ai messaggi ricevuti
```

## 🔧 API JavaScript Proposte

### GPIO API
```javascript
gpio.pinMode(pin, mode);           // INPUT, OUTPUT, INPUT_PULLUP
gpio.digitalWrite(pin, value);     // HIGH, LOW
gpio.digitalRead(pin);             // boolean
gpio.analogRead(pin);              // int
gpio.onInterrupt(pin, trigger, callback); // RISING, FALLING, CHANGE
```

### DHT API
```javascript
const dht = new DHT(pin, type);    // DHT11, DHT22
dht.begin();
dht.readTemperature();             // float
dht.readHumidity();                // float
```

### ADC API
```javascript
const adc = new ADC(pin);
adc.read();                        // int (0-4095)
adc.readMilliVolts();              // float
```

### Mesh API
```javascript
mesh.broadcast(data);              // Invia a tutti
mesh.sendTo(nodeId, data);         // Invia a nodo specifico
mesh.onMessage(callback);          // Riceve messaggi
mesh.getNodeId();                  // ID del nodo corrente
```

### Utility API
```javascript
console.log(message);              // Log output
setInterval(callback, ms);         // Timer periodico
setTimeout(callback, ms);          // Timer singolo
JSON.stringify(obj);               // Serializzazione JSON
```

## 🚀 Utilizzo

### Compilazione
Il modulo è già integrato nel sistema di build di Meshtastic. Viene automaticamente compilato e incluso nel firmware.

### Esecuzione
1. Il modulo si avvia automaticamente all'avvio del dispositivo
2. Carica lo script IDLE di default (index #2)
3. Esegue lo script IDLE ogni 10 secondi per test
4. Ogni 30 secondi cambia automaticamente script (per test)
5. I log mostrano l'attività del modulo e l'interpretazione JavaScript

### Log di Esempio (SCRIPT_IDLE)
```
ScriptingModule: Inizializzazione modulo scripting
ScriptingModule: setup() => Inizializzazione modulo scripting
ScriptingModule: Inizializzazione Duktape...
ScriptingModule: Duktape simulato - inizializzato
ScriptingModule: Esposizione API GPIO
ScriptingModule: Esposizione API DHT
ScriptingModule: Esposizione API ADC
ScriptingModule: Esposizione API Mesh
ScriptingModule: Esposizione API Utility
ScriptingModule: Caricamento script #2
ScriptingModule: Esecuzione script (89 bytes)
ScriptingModule: Esecuzione script semplificato
ScriptingModule: [JS] Rilevato script IDLE
ScriptingModule: [JS] ScriptingModule: Avvio script IDLE
ScriptingModule: [JS] ScriptingModule: Modulo in modalità idle
ScriptingModule: Script semplificato completato (4 righe processate)
ScriptingModule: Script eseguito con successo
ScriptingModule: Script #2 eseguito con successo
ScriptingModule: Modulo scripting inizializzato con successo
```

## 🔮 Roadmap

### Prossimi Passi
1. **Integrazione Duktape**: Implementare il motore JavaScript reale
2. **API Bindings**: Completare i binding per hardware e mesh
3. **Test Funzionali**: Verificare funzionamento con hardware reale
4. **Configurazione Dinamica**: Aggiungere supporto protobuf
5. **Sicurezza**: Implementare sandbox e limitazioni

### Vantaggi Finali
- ✅ **Zero Ricompilazione**: Modifica script senza rebuild
- ✅ **Configurazione Remota**: Carica script via app/CLI
- ✅ **Flessibilità Totale**: Stesso modulo per usi diversi
- ✅ **Hot-Swap**: Cambia comportamento al volo
- ✅ **Sicurezza**: Sandbox isolato per script

## 📁 File Coinvolti

- `src/modules/ScriptingModule.h` - Header del modulo
- `src/modules/ScriptingModule.cpp` - Implementazione del modulo
- `src/modules/Modules.cpp` - Integrazione nel sistema moduli

## 🐛 Stato Attuale

Il modulo è in **Fase 1** - implementazione base completata. Gli script sono hardcoded e l'esecuzione è simulata. La prossima fase richiederà l'integrazione del motore JavaScript Duktape per l'esecuzione reale degli script.

---

**Autore**: Implementato per risolvere il problema di ricompilazione continua dei dispositivi Meshtastic.
**Data**: Dicembre 2024
**Versione**: 1.0 (Fase 1 - Base)
