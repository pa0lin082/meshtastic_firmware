#pragma once

#include "MeshModule.h"
#include "concurrency/OSThread.h"
#include <Arduino.h>

// Forward declarations per Duktape
struct duk_hthread;

/**
 * ScriptingModule - Modulo per esecuzione dinamica di script JavaScript
 * 
 * Questo modulo permette di eseguire script JavaScript hardcoded per test,
 * con accesso alle API hardware e mesh di Meshtastic.
 * 
 * Caratteristiche:
 * - Motore JavaScript Duktape integrato
 * - API bindings per GPIO, DHT, ADC, Mesh
 * - Script hardcoded per test (fase 1)
 * - Esecuzione in thread separato
 * - Sandbox di sicurezza
 */
class ScriptingModule : public MeshModule, private concurrency::OSThread
{
private:
    // Duktape JavaScript context
    duk_hthread* jsContext;
    
    // Stato del modulo
    bool initialized;
    bool scriptRunning;
    String currentScript;
    
    // Configurazione script
    uint32_t executionInterval;
    uint32_t maxMemoryKB;
    uint32_t executionTimeout;
    
    // Gestione esecuzione script
    bool scriptHasLoops;
    uint32_t scriptLastRun;
    
    // Script di test hardcoded
    static const char* SCRIPT_DHT_ADC;
    static const char* SCRIPT_GPIO_CONTROL;
    static const char* SCRIPT_IDLE;
    static const char* SCRIPT_LOOP;
    static const char* SCRIPT_MESH_TEST;
    
    // Indice script corrente (per test)
    uint8_t currentScriptIndex;
    
public:
    ScriptingModule();
    ~ScriptingModule();
    
    // Override da MeshModule
    virtual void setup() override;
    virtual bool wantPacket(const meshtastic_MeshPacket *p) override;
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    
    // Override da OSThread
    virtual int32_t runOnce() override;
    
    // Metodi pubblici
    void loadScript(uint8_t scriptIndex);
    void stopScript();
    void restartScript();
    bool isScriptRunning() const { return scriptRunning; }
    
private:
    // Inizializzazione Duktape
    bool initDuktape();
    void cleanupDuktape();
    
    // API bindings per JavaScript
    void exposeGPIOAPI();
    void exposeDHTAPI();
    void exposeADCAPI();
    void exposeMeshAPI();
    void exposeUtilityAPI();
    
    // Esecuzione script
    bool executeScript(const String& script);
    bool validateScript(const String& script);
    
    // Gestione errori
    void handleJSError();
    void logJSError(const char* error);
    
    // Utility
    String getScriptByIndex(uint8_t index);
    void switchToNextScript();
};

extern ScriptingModule *scriptingModule;
