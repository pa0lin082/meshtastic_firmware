# CustomMetricsSender

Classe helper per l'invio di metriche custom tramite JSON senza utilizzare i protobuf di Meshtastic.

## Descrizione

`CustomMetricsSender` è una classe C++ che semplifica l'invio di metriche personalizzate attraverso la rete mesh di Meshtastic. Le metriche vengono inviate come messaggi JSON tramite `TEXT_MESSAGE_APP`, permettendo di trasmettere dati senza dover modificare i file protobuf.

## Caratteristiche

- ✅ **Facile da usare**: API semplice e intuitiva
- ✅ **Flessibile**: Supporta valori float e int
- ✅ **Sicuro**: Gestione automatica della memoria
- ✅ **Debug-friendly**: Metodi per ispezionare il JSON generato
- ✅ **Riutilizzabile**: La stessa istanza può essere usata per messaggi multipli

## Struttura del JSON

I messaggi generati hanno la seguente struttura:

```json
{
  "type": "custom_metrics",
  "metrics": [
    {
      "name": "temperature",
      "value": 25.5,
      "unit": "celsius"
    },
    {
      "name": "humidity",
      "value": 65.0,
      "unit": "percent"
    }
  ]
}
```

## Utilizzo Base

```cpp
#include "CustomMetricsSender.h"

void inviaMetriche() {
    CustomMetricsSender sender;
    
    // Inizia un nuovo messaggio
    sender.beginMessage();
    
    // Aggiungi le metriche
    sender.addMetric("temperature", 25.5f, "celsius");
    sender.addMetric("humidity", 65.0f, "percent");
    sender.addMetric("pressure", 1013, "hPa");
    
    // Invia in broadcast
    sender.send(true);
}
```

## API Reference

### Costruttore

```cpp
CustomMetricsSender()
```
Crea una nuova istanza del sender.

### beginMessage()

```cpp
bool beginMessage()
```
Inizia un nuovo messaggio. Deve essere chiamato prima di aggiungere metriche.

**Ritorna**: `true` se l'inizializzazione è riuscita, `false` altrimenti.

### addMetric() - Float

```cpp
bool addMetric(const char *name, float value, const char *unit)
```
Aggiunge una metrica con valore float.

**Parametri**:
- `name`: Nome della metrica (es. "temperature")
- `value`: Valore della metrica (float)
- `unit`: Unità di misura (es. "celsius")

**Ritorna**: `true` se la metrica è stata aggiunta, `false` altrimenti.

### addMetric() - Int

```cpp
bool addMetric(const char *name, int value, const char *unit)
```
Aggiunge una metrica con valore intero.

**Parametri**:
- `name`: Nome della metrica
- `value`: Valore della metrica (int)
- `unit`: Unità di misura

**Ritorna**: `true` se la metrica è stata aggiunta, `false` altrimenti.

### send()

```cpp
bool send(bool broadcast = true)
```
Invia il messaggio con tutte le metriche. Dopo l'invio, il messaggio viene automaticamente resettato.

**Parametri**:
- `broadcast`: Se `true` (default), invia in broadcast

**Ritorna**: `true` se l'invio è riuscito, `false` altrimenti.

### reset()

```cpp
void reset()
```
Resetta il messaggio corrente senza inviarlo. Utile per annullare un messaggio in preparazione.

### setMessageType()

```cpp
void setMessageType(const char *type)
```
Imposta un tipo custom per il messaggio (default: "custom_metrics").

**Parametri**:
- `type`: Tipo del messaggio

### getMetricCount()

```cpp
size_t getMetricCount() const
```
Restituisce il numero di metriche attualmente nel messaggio.

**Ritorna**: Numero di metriche.

### getJsonString()

```cpp
std::string getJsonString() const
```
Ottiene la stringa JSON che verrà inviata (utile per debug).

**Ritorna**: Stringa JSON o stringa vuota se non ci sono metriche.

## Esempi Avanzati

### Esempio 1: Verifica prima dell'invio

```cpp
CustomMetricsSender sender;
sender.beginMessage();

// Aggiungi metriche condizionalmente
if (sensorDisponibile) {
    sender.addMetric("temperature", leggiTemperatura(), "celsius");
}

// Verifica prima di inviare
if (sender.getMetricCount() > 0) {
    LOG_INFO("Invio di %d metriche", sender.getMetricCount());
    sender.send(true);
}
```

### Esempio 2: Tipo messaggio personalizzato

```cpp
CustomMetricsSender sender;
sender.setMessageType("sensor_data");  // Invece di "custom_metrics"

sender.beginMessage();
sender.addMetric("soil_moisture", 45.2f, "percent");
sender.send(true);
```

### Esempio 3: Debug del JSON

```cpp
CustomMetricsSender sender;
sender.beginMessage();
sender.addMetric("debug_value", 123.45f, "test");

// Visualizza il JSON prima dell'invio
std::string json = sender.getJsonString();
LOG_DEBUG("JSON: %s", json.c_str());

sender.send(true);
```

### Esempio 4: Riutilizzo della stessa istanza

```cpp
CustomMetricsSender sender;

// Primo invio
sender.beginMessage();
sender.addMetric("reading1", 10.5f, "unit");
sender.send(true);

// Secondo invio (viene resettato automaticamente)
sender.beginMessage();
sender.addMetric("reading2", 20.3f, "unit");
sender.send(true);
```

### Esempio 5: Uso in un modulo Meshtastic

```cpp
class MioModuloCustom : public concurrency::OSThread
{
  private:
    CustomMetricsSender sender;
    unsigned long lastSendTime = 0;
    const unsigned long SEND_INTERVAL = 60000; // 60 secondi

  public:
    MioModuloCustom() : OSThread("MioModulo") {}

    int32_t runOnce() override
    {
        unsigned long now = millis();
        
        if (now - lastSendTime >= SEND_INTERVAL) {
            inviaMetriche();
            lastSendTime = now;
        }
        
        return 1000; // Esegui ogni secondo
    }

  private:
    void inviaMetriche()
    {
        float temp = leggiTemperatura();
        float hum = leggiUmidita();

        sender.beginMessage();
        sender.addMetric("temperatura", temp, "celsius");
        sender.addMetric("umidita", hum, "percent");
        
        if (sender.send(true)) {
            LOG_INFO("Metriche inviate con successo");
        } else {
            LOG_ERROR("Errore nell'invio delle metriche");
        }
    }

    float leggiTemperatura() { /* ... */ return 25.0f; }
    float leggiUmidita() { /* ... */ return 60.0f; }
};
```

## Note Importanti

1. **Limite dimensione payload**: Il payload JSON non può superare la dimensione massima del pacchetto mesh (tipicamente 237 byte). La classe verifica automaticamente questo limite.

2. **Gestione memoria**: La classe gestisce automaticamente la pulizia della memoria. Non è necessario deallocare manualmente le risorse.

3. **Thread-safety**: La classe NON è thread-safe. Se usata in contesti multi-thread, è necessario implementare meccanismi di sincronizzazione esterni.

4. **Reset automatico**: Il metodo `send()` resetta automaticamente il messaggio dopo l'invio. Non è necessario chiamare `reset()` manualmente.

## Files

- `CustomMetricsSender.h` - Header della classe
- `CustomMetricsSender.cpp` - Implementazione
- `CustomMetricsSender_Example.cpp` - Esempi di utilizzo (non compilato)
- `CustomMetricsSender_README.md` - Questa documentazione

## Dipendenze

- `serialization/JSON.h` - Libreria JSON di Meshtastic
- `Router.h` e `MeshService.h` - Per l'invio dei pacchetti
- `meshtastic/portnums.pb.h` - Definizioni dei port numbers

## Autore

Creato per il progetto Meshtastic Firmware.

## Licenza

Segue la licenza del progetto Meshtastic (GPL-3.0).


