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
    metricObj["name"] = new JSONValue(name);
    metricObj["value"] = new JSONValue((double)value);
    metricObj["unit"] = new JSONValue(unit);

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
    metricObj["value"] = new JSONValue((double)value);
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
    
    LOG_INFO("CustomMetricsSender: JSON generato (%d byte): %s", jsonData.length(), jsonData.c_str());
    
    // Verifica che il payload non sia troppo grande
    if (jsonData.length() > sizeof(p->decoded.payload.bytes)) {
        LOG_ERROR("CustomMetricsSender: Payload troppo grande (%d byte, max %d)", 
                  jsonData.length(), sizeof(p->decoded.payload.bytes));
        delete jsonValue;
        return false;
    }

    // Copia i dati nel payload
    memcpy(p->decoded.payload.bytes, jsonData.c_str(), jsonData.length());
    p->decoded.payload.size = jsonData.length();

    // Imposta il destinatario
    if (broadcast) {
        p->to = NODENUM_BROADCAST;
    }

    // Pulisce il JSON value (il jsonObj contiene le metriche che saranno pulite dopo)
    delete jsonValue;

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
    JSONObject jsonObj;
    jsonObj["type"] = new JSONValue(messageType.c_str());
    jsonObj["metrics"] = new JSONValue(metricsArray);

    // Converti in stringa
    JSONValue *jsonValue = new JSONValue(jsonObj);
    std::string jsonData = jsonValue->Stringify();
    delete jsonValue;

    return jsonData;
}

void CustomMetricsSender::cleanup()
{
    // Pulisce tutti i JSONValue nell'array
    for (size_t i = 0; i < metricsArray.size(); i++) {
        delete metricsArray[i];
    }
    metricsArray.clear();
}

