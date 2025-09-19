#include "ScriptingModule.h"
#include "Router.h"
#include "MeshService.h"
// #include "DHT.h"
#include <Arduino.h>

// Include Duktape
#include "duktape.h"

// Dichiarazioni delle variabili globali necessarie
extern Router *router;
extern MeshService *service;

// Script di test hardcoded
const char* ScriptingModule::SCRIPT_DHT_ADC = R"(
// Script per monitoraggio sensori DHT e ADC
console.log("ScriptingModule: Avvio script DHT+ADC");

const config = {
    dhtPin: 5,
    dhtType: 'DHT11',
    adcPin: 6,
    interval: 30000
};

let dht = null;
let adc = null;

function initSensors() {
    console.log("Inizializzazione sensori...");
    
    if (config.dhtPin !== undefined) {
        dht = new DHT(config.dhtPin, config.dhtType);
        dht.begin();
        console.log("DHT inizializzato su pin", config.dhtPin);
    }
    
    if (config.adcPin !== undefined) {
        adc = new ADC(config.adcPin);
        console.log("ADC inizializzato su pin", config.adcPin);
    }
}

function readSensors() {
    const data = {
        type: "sensor_data",
        timestamp: Date.now()
    };
    
    if (dht) {
        data.temperature = dht.readTemperature();
        data.humidity = dht.readHumidity();
    }
    
    if (adc) {
        data.voltage = adc.readMilliVolts();
    }
    
    return data;
}

function sendData() {
    const sensorData = readSensors();
    console.log("Invio dati:", JSON.stringify(sensorData));
    mesh.broadcast(JSON.stringify(sensorData));
}

// Avvia il sistema
initSensors();
setInterval(sendData, config.interval);
console.log("ScriptingModule: Script DHT+ADC avviato");
)";

const char* ScriptingModule::SCRIPT_GPIO_CONTROL = R"(
// Script per controllo GPIO
console.log("ScriptingModule: Avvio script GPIO Control");

const config = {
    ledPin: 2,
    buttonPin: 0,
    interval: 1000
};

let ledState = false;

function initGPIO() {
    console.log("Inizializzazione GPIO...");
    gpio.pinMode(config.ledPin, gpio.OUTPUT);
    gpio.pinMode(config.buttonPin, gpio.INPUT_PULLUP);
    console.log("GPIO inizializzato");
}

function handleButton() {
    ledState = !ledState;
    gpio.digitalWrite(config.ledPin, ledState);
    
    const data = {
        type: "button_pressed",
        led_state: ledState,
        timestamp: Date.now()
    };
    
    console.log("Button pressed, LED:", ledState);
    mesh.broadcast(JSON.stringify(data));
}

function blinkLED() {
    if (!ledState) {
        const currentState = gpio.digitalRead(config.ledPin);
        gpio.digitalWrite(config.ledPin, !currentState);
    }
}

// Setup
initGPIO();
gpio.onInterrupt(config.buttonPin, gpio.FALLING, handleButton);
setInterval(blinkLED, config.interval);
console.log("ScriptingModule: Script GPIO Control avviato");
)";

const char *ScriptingModule::SCRIPT_IDLE = R"(
// Script idle - test timer
console.log("ScriptingModule: Avvio script IDLE");
console.log("ScriptingModule: Modulo in modalità idle");

)";

const char* ScriptingModule::SCRIPT_LOOP = R"(
    // Script con API Meshtastic
    console.log("ScriptingModule: Avvio script IDLE con API Meshtastic");
    
    var counter = 0;
    
    function setup() {
        console.log("ScriptingModule: Setup IDLE completato");
        console.log("ScriptingModule: Configurando GPIO per LED");
        
        // Configura GPIO per LED built-in (pin 2)
        gpio.pinMode(2, gpio.OUTPUT);
    }
    
    function loop() {
        counter++;
        console.log("ScriptingModule: Loop IDLE attivo", counter);
        
        // Accendi/spegni LED
        gpio.digitalWrite(2, counter % 2);
        
        // Test delay
        delay(100);
    }
    
    // Avvia setup e loop
    setup();
    )";

const char* ScriptingModule::SCRIPT_MESH_TEST = R"(
// Script per test mesh
console.log("ScriptingModule: Avvio script Mesh Test");

let messageCount = 0;

function sendTestMessage() {
    messageCount++;
    const data = {
        type: "test_message",
        count: messageCount,
        timestamp: Date.now(),
        node_id: mesh.getNodeId()
    };
    
    console.log("Invio messaggio test #", messageCount);
    mesh.broadcast(JSON.stringify(data));
}

function handleIncomingMessage(msg) {
    console.log("Messaggio ricevuto:", msg);
    
    if (msg.type === "test_message") {
        const response = {
            type: "test_response",
            original_count: msg.count,
            timestamp: Date.now()
        };
        mesh.broadcast(JSON.stringify(response));
    }
}

// Setup
mesh.onMessage(handleIncomingMessage);
setInterval(sendTestMessage, 10000);
console.log("ScriptingModule: Script Mesh Test avviato");
)";

ScriptingModule *scriptingModule;

ScriptingModule::ScriptingModule() 
    : MeshModule("ScriptingModule")
    , concurrency::OSThread("ScriptingModule")
    , jsContext(nullptr)
    , initialized(false)
    , scriptRunning(false)
    , executionInterval(5000)
    , maxMemoryKB(64)
    , executionTimeout(1000)
    , scriptHasLoops(false)
    , scriptLastRun(0)
    , currentScript()  // Inizializza come undefined
{
    LOG_INFO("ScriptingModule: Inizializzazione modulo scripting");
}

ScriptingModule::~ScriptingModule()
{
    cleanupDuktape();
    LOG_INFO("ScriptingModule: Modulo scripting distrutto");
}

void ScriptingModule::setup()
{
    LOG_INFO("ScriptingModule: setup() => Inizializzazione modulo scripting");
    
    if (initDuktape()) {
        initialized = true;
        loadScript();
        LOG_INFO("ScriptingModule: Modulo scripting inizializzato con successo");
    } else {
        LOG_ERROR("ScriptingModule: Errore nell'inizializzazione del modulo scripting");
    }
}


bool ScriptingModule::initDuktape()
{
    LOG_INFO("ScriptingModule: Inizializzazione Duktape standard...");
    
    // Crea il context Duktape
    jsContext = duk_create_heap_default();
    if (!jsContext) {
        LOG_ERROR("ScriptingModule: Errore nella creazione del context Duktape");
        return false;
    }
    
    LOG_INFO("ScriptingModule: Context Duktape creato con successo");
    
    // Espone le API JavaScript
    exposeGPIOAPI();
    exposeDHTAPI();
    exposeADCAPI();
    exposeMeshAPI();
    exposeUtilityAPI();
    
    return true;
}

void ScriptingModule::cleanupDuktape()
{
    if (jsContext) {
        LOG_INFO("ScriptingModule: Cleanup Duktape context");
        duk_destroy_heap(jsContext);
        jsContext = nullptr;
    }
}

void ScriptingModule::exposeGPIOAPI()
{
    LOG_INFO("ScriptingModule: Esposizione API GPIO");
    
    // Crea oggetto gpio
    duk_push_object(jsContext);
    
    // Costanti GPIO
    duk_push_int(jsContext, OUTPUT);
    duk_put_prop_string(jsContext, -2, "OUTPUT");
    duk_push_int(jsContext, INPUT);
    duk_put_prop_string(jsContext, -2, "INPUT");
    duk_push_int(jsContext, INPUT_PULLUP);
    duk_put_prop_string(jsContext, -2, "INPUT_PULLUP");
    
    // Metodo pinMode
    duk_push_c_function(jsContext, [](duk_context *ctx) -> duk_ret_t {
        int pin = duk_get_int(ctx, 0);
        int mode = duk_get_int(ctx, 1);
        pinMode(pin, mode);
        return 0;
    }, 2);
    duk_put_prop_string(jsContext, -2, "pinMode");
    
    // Metodo digitalWrite
    duk_push_c_function(jsContext, [](duk_context *ctx) -> duk_ret_t {
        int pin = duk_get_int(ctx, 0);
        int value = duk_get_int(ctx, 1);
        digitalWrite(pin, value);
        return 0;
    }, 2);
    duk_put_prop_string(jsContext, -2, "digitalWrite");
    
    // Metodo digitalRead
    duk_push_c_function(jsContext, [](duk_context *ctx) -> duk_ret_t {
        int pin = duk_get_int(ctx, 0);
        int value = digitalRead(pin);
        duk_push_int(ctx, value);
        return 1;
    }, 1);
    duk_put_prop_string(jsContext, -2, "digitalRead");
    
    // Metodo analogRead
    duk_push_c_function(jsContext, [](duk_context *ctx) -> duk_ret_t {
        int pin = duk_get_int(ctx, 0);
        int value = analogRead(pin);
        duk_push_int(ctx, value);
        return 1;
    }, 1);
    duk_put_prop_string(jsContext, -2, "analogRead");
    
    // Metodo analogWrite
    duk_push_c_function(jsContext, [](duk_context *ctx) -> duk_ret_t {
        int pin = duk_get_int(ctx, 0);
        int value = duk_get_int(ctx, 1);
        analogWrite(pin, value);
        return 0;
    }, 2);
    duk_put_prop_string(jsContext, -2, "analogWrite");
    
    // Espone l'oggetto gpio globalmente
    duk_put_global_string(jsContext, "gpio");
    
    LOG_INFO("ScriptingModule: API GPIO esposte con successo");
}

void ScriptingModule::exposeDHTAPI()
{
    LOG_INFO("ScriptingModule: Esposizione API DHT");
    // TODO: Implementare binding DHT per JavaScript
}

void ScriptingModule::exposeADCAPI()
{
    LOG_INFO("ScriptingModule: Esposizione API ADC");
    // TODO: Implementare binding ADC per JavaScript
}

void ScriptingModule::exposeMeshAPI()
{
    LOG_INFO("ScriptingModule: Esposizione API Mesh");
    // TODO: Implementare binding Mesh per JavaScript
}

void ScriptingModule::exposeUtilityAPI()
{
    LOG_INFO("ScriptingModule: Esposizione API Utility");
    
    // Espone console.log
    duk_push_c_function(jsContext, [](duk_context *ctx) -> duk_ret_t {
        const char *msg = duk_safe_to_string(ctx, 0);
        LOG_INFO("ScriptingModule: [JS] %s", msg);
        return 0;
    }, 1);
    duk_put_global_string(jsContext, "console_log");
    
    // Crea oggetto console con metodo log
    duk_push_object(jsContext);
    duk_push_c_function(jsContext, [](duk_context *ctx) -> duk_ret_t {
        const char *msg = duk_safe_to_string(ctx, 0);
        LOG_INFO("ScriptingModule: [JS] %s", msg);
        return 0;
    }, 1);
    duk_put_prop_string(jsContext, -2, "log");
    duk_put_global_string(jsContext, "console");
    
    // Espone delay() per JavaScript
    duk_push_c_function(jsContext, [](duk_context *ctx) -> duk_ret_t {
        duk_int_t ms = duk_to_int(ctx, 0);
        delay(ms);
        return 0;
    }, 1);
    duk_put_global_string(jsContext, "delay");
    
    LOG_INFO("ScriptingModule: API console.log e delay esposte");
}

void ScriptingModule::loadScript() {

    currentScript = SCRIPT_LOOP;
    LOG_INFO("ScriptingModule: Caricamento script %s (todo: load from config)", currentScript.c_str());
    if (!initialized) {
        LOG_ERROR("ScriptingModule: Modulo non inizializzato");
        return;
    }
    
    scriptHasLoops = false; // Reset flag loop

    LOG_INFO("ScriptingModule: Caricamento script %s", currentScript.c_str());

    const std::string scriptHash = "IDLE";
    
    if (validateScript(currentScript)) {
        if (executeScript(currentScript)) {
            scriptRunning = true;
            LOG_INFO("ScriptingModule: Script #%s eseguito con successo", scriptHash);
        } else {
            LOG_ERROR("ScriptingModule: Errore nell'esecuzione dello script #%s", scriptHash);
        }
    } else {
        LOG_ERROR("ScriptingModule: Script #%s non valido", scriptHash);
    }
}



bool ScriptingModule::validateScript(const String& script)
{
    // Validazione base dello script
    if (script.length() == 0) {
        LOG_ERROR("ScriptingModule: Script vuoto");
        return false;
    }
    
    if (script.length() > 8192) { // Limite di 8KB per ora
        LOG_ERROR("ScriptingModule: Script troppo grande (%d bytes)", script.length());
        return false;
    }
    
    // TODO: Aggiungere altre validazioni (sintassi, sicurezza, etc.)
    return true;
}

bool ScriptingModule::executeScript(const String& script)
{
    LOG_INFO("ScriptingModule: Esecuzione script (%d bytes)", script.length());

    if (!jsContext) {
        LOG_ERROR("ScriptingModule: Context Duktape non inizializzato");
        return false;
    }

    // Esegui lo script JavaScript
    duk_int_t rc = duk_peval_string(jsContext, script.c_str());
    
    if (rc != 0) {
        // Errore nell'esecuzione
        const char *error = duk_safe_to_string(jsContext, -1);
        LOG_ERROR("ScriptingModule: Errore JavaScript: %s", error);
        duk_pop(jsContext); // Rimuovi l'errore dallo stack
        return false;
    }
    
    // Rimuovi il risultato dallo stack se presente
    duk_pop(jsContext);
    
    LOG_INFO("ScriptingModule: Script eseguito con successo");
    return true;
}

// bool ScriptingModule::executeScriptTick()
// {
//     if (!initialized || jsContext == nullptr) {
//         LOG_ERROR("ScriptingModule: Contesto JavaScript non inizializzato");
//         return false;
//     }
    
//     // Cerca la funzione loop() nello stack globale
//     if (duk_get_global_string(jsContext, "loop") != 1) {
//         LOG_DEBUG("ScriptingModule: Funzione loop() non trovata");
//         duk_pop(jsContext); // Pulisci lo stack
//         return false;
//     }
    
//     // Chiama la funzione loop()
//     if (duk_pcall(jsContext, 0) != 0) {
//         handleJSError();
//         return false;
//     }
    
//     // Pulisci lo stack
//     duk_pop(jsContext);
    
//     return true;
// }

void ScriptingModule::stopScript()
{
    if (scriptRunning) {
        LOG_INFO("ScriptingModule: Arresto script corrente");
        scriptRunning = false;
        // TODO: Implementare arresto script
    }
}

void ScriptingModule::restartScript()
{
    LOG_INFO("ScriptingModule: Riavvio script");
    stopScript();
    loadScript();
}


int32_t ScriptingModule::runOnce()
{
    LOG_DEBUG("ScriptingModule: runOnce() chiamato");
    
    // Chiama setup() la prima volta se non inizializzato
    if (!initialized) {
        LOG_INFO("ScriptingModule: Prima esecuzione - chiamando setup()");
        setup();
        
        if (!initialized) {
            LOG_ERROR("ScriptingModule: Errore nell'inizializzazione, riprovo tra 5 secondi");
            return 5000; // Riprova dopo 5 secondi
        }
    }
    
    if (!scriptRunning) {
        LOG_DEBUG("ScriptingModule: Script non in esecuzione, controllo tra 10 secondi");
        return 10000; // Se non c'è script in esecuzione, controlla ogni 10 secondi
    }
    
    // Simula l'esecuzione periodica dello script
    static uint32_t lastExecution = 0;
    uint32_t now = millis();
    
    if (now - lastExecution >= executionInterval) {
        LOG_INFO("ScriptingModule: Esecuzione periodica script");
        lastExecution = now;
        
        executeScript(currentScript);
    }
    
    return 1000; // Controlla ogni secondo
}

bool ScriptingModule::wantPacket(const meshtastic_MeshPacket *p)
{
    // Per ora non gestiamo pacchetti mesh specifici
    // TODO: Implementare gestione messaggi JavaScript
    return false;
}

ProcessMessage ScriptingModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    // Per ora non gestiamo pacchetti ricevuti
    // TODO: Implementare gestione messaggi JavaScript
    return ProcessMessage::CONTINUE;
}

void ScriptingModule::handleJSError()
{
    LOG_ERROR("ScriptingModule: Errore JavaScript");
    // TODO: Implementare gestione errori JavaScript
}

void ScriptingModule::logJSError(const char* error)
{
    LOG_ERROR("ScriptingModule: JS Error: %s", error);
}
