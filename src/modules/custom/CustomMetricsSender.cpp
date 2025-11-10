#include "CustomMetricsSender.h"
#include "MeshService.h"
#include "Router.h"
#include "mesh/generated/meshtastic/portnums.pb.h"
#include <Arduino.h>

// Dichiarazioni esterne necessarie
extern Router *router;
extern MeshService *service;

CustomMetricsSender::CustomMetricsSender()
    : messageType("custom_metrics"), messageStarted(false)
{
}

CustomMetricsSender::~CustomMetricsSender()
{
    cleanup();
}

bool CustomMetricsSender::beginMessage()
{
    // Pulisce eventuali messaggi precedenti
    cleanup();

    // Resetta l'array di metriche
    metricsArray.clear();

    messageStarted = true;
    LOG_DEBUG("CustomMetricsSender: Messaggio iniziato");
    return true;
}

bool CustomMetricsSender::addMetric(const char *name, float value, const char *unit)
{
    if (!messageStarted) {
        LOG_ERROR("CustomMetricsSender: Chiamare beginMessage() prima di addMetric()");
        return false;
    }

    if (!name || !unit) {
        LOG_ERROR("CustomMetricsSender: name e unit non possono essere NULL");
        return false;
    }

    // Crea un oggetto metrica
    JSONObject metricObj;
    metricObj["n"] = new JSONValue(name);
    metricObj["v"] = new JSONValue((double)value);
    metricObj["u"] = new JSONValue(unit);

    // Aggiungi la metrica all'array
    metricsArray.push_back(new JSONValue(metricObj));

    LOG_DEBUG("CustomMetricsSender: Metrica aggiunta - %s: %.6f %s", name, value, unit);
    return true;
}

bool CustomMetricsSender::addMetric(const char *name, int value, const char *unit)
{
    if (!messageStarted) {
        LOG_ERROR("CustomMetricsSender: Chiamare beginMessage() prima di addMetric()");
        return false;
    }

    if (!name || !unit) {
        LOG_ERROR("CustomMetricsSender: name e unit non possono essere NULL");
        return false;
    }

    // Crea un oggetto metrica
    JSONObject metricObj;
    metricObj["name"] = new JSONValue(name);
    metricObj["value"] = new JSONValue((int)value);
    metricObj["unit"] = new JSONValue(unit);

    // Aggiungi la metrica all'array
    metricsArray.push_back(new JSONValue(metricObj));

    LOG_DEBUG("CustomMetricsSender: Metrica aggiunta - %s: %d %s", name, value, unit);
    return true;
}

bool CustomMetricsSender::send(bool broadcast)
{
    if (!messageStarted) {
        LOG_ERROR("CustomMetricsSender: Nessun messaggio da inviare");
        return false;
    }

    if (metricsArray.size() == 0) {
        LOG_WARN("CustomMetricsSender: Nessuna metrica da inviare");
        return false;
    }

    // Verifica che router e service siano disponibili
    if (!router || !service) {
        LOG_ERROR("CustomMetricsSender: Router o Service non disponibili");
        return false;
    }

    // Alloca un pacchetto per l'invio
    meshtastic_MeshPacket *p = router->allocForSending();
    if (!p) {
        LOG_ERROR("CustomMetricsSender: Errore allocazione pacchetto");
        return false;
    }

    // Imposta il tipo di porta
    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

        // Crea l'oggetto JSON principale
    JSONObject jsonObj;
    jsonObj["type"] = new JSONValue(messageType.c_str());
    jsonObj["metrics"] = new JSONValue(metricsArray);

    // Converti l'oggetto JSON in una stringa
    JSONValue *jsonValue = new JSONValue(jsonObj);
    std::string jsonData = jsonValue->Stringify();
    
    // IMPORTANTE: delete jsonValue elimina TUTTO l'albero JSON, inclusi i puntatori in metricsArray
    // Quindi dobbiamo eliminarlo e poi svuotare metricsArray senza fare delete
    delete jsonValue;
    // Svuota l'array senza eliminare i puntatori (già eliminati da delete jsonValue)
    metricsArray.clear();
    
    LOG_INFO("CustomMetricsSender: JSON generato (%d byte)", jsonData.length());
    LOG_INFO("%s",jsonData.c_str());
    
    // Verifica che il payload non sia troppo grande
    if (jsonData.length() > sizeof(p->decoded.payload.bytes)) {
        LOG_ERROR("CustomMetricsSender: Payload troppo grande (%d byte, max %d)", 
                    jsonData.length(), sizeof(p->decoded.payload.bytes));
        return false;
    }

    // Copia i dati nel payload
    memcpy(p->decoded.payload.bytes, jsonData.c_str(), jsonData.length());
    p->decoded.payload.size = jsonData.length();
 





    // Imposta il destinatario
    if (broadcast) {
        p->to = NODENUM_BROADCAST;
    }

    p->decoded.want_response = false;
    p->priority = meshtastic_MeshPacket_Priority_RELIABLE;

    // Invia il messaggio
    service->sendToMesh(p, RX_SRC_LOCAL);
    
    LOG_INFO("CustomMetricsSender: Messaggio inviato con %d metriche", metricsArray.size());

    // Resetta il messaggio dopo l'invio
    reset();

    return true;
}

void CustomMetricsSender::reset()
{
    cleanup();
    messageStarted = false;
    LOG_DEBUG("CustomMetricsSender: Messaggio resettato");
}

void CustomMetricsSender::setMessageType(const char *type)
{
    if (type) {
        messageType = type;
        LOG_DEBUG("CustomMetricsSender: Tipo messaggio impostato: %s", type);
    }
}

size_t CustomMetricsSender::getMetricCount() const
{
    return metricsArray.size();
}

std::string CustomMetricsSender::getJsonString() const
{
    if (!messageStarted || metricsArray.size() == 0) {
        return "";
    }

    // Crea l'oggetto JSON principale
    // NOTA: Questa funzione è const quindi non possiamo modificare metricsArray
    // Dobbiamo fare attenzione a non causare memory leak
    JSONObject jsonObj;
    jsonObj["type"] = new JSONValue(messageType.c_str());
    
    // Crea una COPIA dell'array per evitare problemi di ownership
    std::vector<JSONValue *> metricsCopy;
    for (size_t i = 0; i < metricsArray.size(); i++) {
        // Non copiamo, usiamo solo il reference - questo è ok perché è const
        metricsCopy.push_back(metricsArray[i]);
    }
    jsonObj["metrics"] = new JSONValue(metricsCopy);

    // Converti in stringa
    JSONValue *jsonValue = new JSONValue(jsonObj);
    std::string jsonData = jsonValue->Stringify();
    
    // PROBLEMA: delete jsonValue eliminerebbe anche i puntatori in metricsArray!
    // Non possiamo eliminarli perché sono ancora usati dall'oggetto
    // TODO: Questo crea un memory leak - la funzione dovrebbe essere riprogettata
    // Per ora non eliminiamo per evitare il crash
    // delete jsonValue;

    return jsonData;
}

void CustomMetricsSender::cleanup()
{
    // Pulisce tutti i JSONValue nell'array E i JSONValue interni a ciascuna metrica
    for (size_t i = 0; i < metricsArray.size(); i++) {
        if (metricsArray[i]) {
            // Ogni elemento è un JSONValue che contiene un JSONObject con "n", "v", "u"
            // Il delete di JSONValue dovrebbe pulire anche il contenuto interno
            delete metricsArray[i];
        }
    }
    metricsArray.clear();
}

