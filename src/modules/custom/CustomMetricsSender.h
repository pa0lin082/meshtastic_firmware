#pragma once

#include "serialization/JSON.h"

/**
 * Classe helper per l'invio di metriche custom tramite JSON
 * senza utilizzare i protobuf.
 * 
 * Esempio di utilizzo:
 * 
 *   CustomMetricsSender sender;
 *   sender.beginMessage();
 *   sender.addMetric("temperature", 25.5f, "celsius");
 *   sender.addMetric("humidity", 65.0f, "percent");
 *   sender.addMetric("voltage", 3.7f, "volts");
 *   sender.send();
 */
class CustomMetricsSender
{
  public:
    /**
     * Costruttore della classe
     */
    CustomMetricsSender();

    /**
     * Distruttore - gestisce la pulizia della memoria
     */
    ~CustomMetricsSender();

    /**
     * Inizia un nuovo messaggio di metriche custom.
     * Deve essere chiamato prima di aggiungere metriche.
     * 
     * @return true se l'inizializzazione è riuscita, false altrimenti
     */
    bool beginMessage();

    /**
     * Aggiunge una metrica al messaggio corrente.
     * 
     * @param name Nome della metrica (es. "temperature")
     * @param value Valore della metrica (float)
     * @param unit Unità di misura (es. "celsius", "volts", "percent")
     * @return true se la metrica è stata aggiunta, false altrimenti
     */
    bool addMetric(const char *name, float value, const char *unit);

    /**
     * Aggiunge una metrica al messaggio corrente (versione con valore intero).
     * 
     * @param name Nome della metrica
     * @param value Valore della metrica (int)
     * @param unit Unità di misura
     * @return true se la metrica è stata aggiunta, false altrimenti
     */
    bool addMetric(const char *name, int value, const char *unit);

    /**
     * Invia il messaggio con tutte le metriche aggiunte alla mesh.
     * Dopo l'invio, il messaggio viene resettato.
     * 
     * @param broadcast Se true, invia in broadcast. Se false, usa il destinatario predefinito
     * @return true se l'invio è riuscito, false altrimenti
     */
    bool send(bool broadcast = true);

    /**
     * Resetta il messaggio corrente senza inviarlo.
     * Utile per annullare un messaggio in preparazione.
     */
    void reset();

    /**
     * Imposta un tipo custom per il messaggio (default: "custom_metrics")
     * 
     * @param type Tipo del messaggio
     */
    void setMessageType(const char *type);

    /**
     * Restituisce il numero di metriche attualmente nel messaggio
     * 
     * @return Numero di metriche
     */
    size_t getMetricCount() const;

    /**
     * Ottiene la stringa JSON generata (utile per debug)
     * Nota: questa funzione genera il JSON al momento della chiamata
     * 
     * @return Stringa JSON o stringa vuota se non ci sono metriche
     */
    std::string getJsonString() const;

  private:
    JSONArray metricsArray;
    std::string messageType;
    bool messageStarted;

    /**
     * Pulisce tutte le risorse allocate
     */
    void cleanup();
};

