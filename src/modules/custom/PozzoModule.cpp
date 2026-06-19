// #if USE_POZZO_MODULE
// #define __PROG__ "jm_LCM2004A_I2C_PrintScreen"
#include "main.h"
#include "PozzoModule.h"
#include "DebugConfiguration.h"
#include "CustomMetricsSender.h"

#include "MeshService.h"
#include "OLEDDisplayFonts.h"
#include "Router.h"
#include <Arduino.h>

#include <Adafruit_ADS1X15.h>
#include <jm_LCM2004A_I2C.h>
#include <Throttle.h>
#include "memGet.h"


#define PIN_RELAY_PUMP 45
#define DISPLAY_UPDATE_INTERVAL_MS 1000
#define TELEMETRY_UPDATE_INTERVAL_MS 10*60*1000 // 10 minuti
#define PUMP_STATE_CHECK_INTERVAL_MS 250   // Intervallo controllo stato pompa
#define PUMP_ON_CURRENT_THRESHOLD 0.05f      // Soglia corrente per considerare pompa accesa (Ampere)


#define USE_FFTPUMPMONITOR 0
#define USE_PUMPMONITOR 1

// Costanti per il display LCD 20x4
static const uint8_t LCD_COLS = 20;
static const uint8_t LCD_ROWS = 4;

// Buffer del display per ottimizzare gli aggiornamenti
static char displayBuffer[LCD_ROWS][LCD_COLS + 1];

static const uint8_t WATER_LEVEL_SENSOR_CHANNEL = 0;
static const uint8_t SCT013_CURRENT_SENSOR_CHANNEL = 1;  // Canale 1 per SCT-013-030

static const float WATER_LEVEL_SCALE_FACTOR = 15000.0f / 3.3f;

// Configurazione SCT-013-030
// Il SCT-013-030 è un trasformatore di corrente (Current Transformer - CT)
// Specifiche:
// - Misura corrente AC fino a 30A
// - Output: 1V per 30A (quindi 33.33 mV/A)
// - Non ha offset DC (centrato a 0V, oscilla tra positivo e negativo)
// - Richiede bias a Vcc/2 tramite circuito esterno per lettura su ADC unipolare
static const float SCT013_SENSITIVITY = 0.03333f;  // 33.33 mV/A (1V/30A)

// Configurazione campionamento
// NOTA: Il data rate dell'ADS1115 è GLOBALE per tutti i canali
// Scegliere un compromesso in base al sensore più critico:
// - Per DC (livello acqua, corrente DC): 8-16 SPS è ottimale
// - Per AC 50Hz: serve minimo 128-250 SPS
static const int WATER_LEVEL_READ_SAMPLES = 3;
static const int PUMP_CURRENT_READ_SAMPLES = 1;

static const bool BYPASS_PUMP_MONITOR_FOR_RELAY_CONTROL = true;


// Dichiarazioni delle variabili globali necessarie
extern Router *router;
extern MeshService *service;
extern graphics::Screen *screen;

PozzoModule *pozzoModule;

PozzoModule::PozzoModule()
    : SinglePortModule("PozzoModule", meshtastic_PortNum_TEXT_MESSAGE_APP),
      concurrency::OSThread("PozzoModule")
{
    LOG_INFO("PozzoModule: Costruttore chiamato - l'inizializzazione ADS1115 avverrà in runOnce()");
    // printMemoryInfo("COSTRUTTORE INIZIO");
    initDisplayBuffer();
    pinMode(PIN_RELAY_PUMP, OUTPUT);
    digitalWrite(PIN_RELAY_PUMP, LOW);
#if USE_FFTPUMPMONITOR
    fftPumpMonitor = new FFTPumpMonitor(&pumpCurrentAmps);
#endif
#if USE_PUMPMONITOR
    pumpMonitor = new PumpMonitor(&pumpCurrentAmps);
#endif
    // printMemoryInfo("COSTRUTTORE FINE");
}

PozzoModule::~PozzoModule()
{
    LOG_INFO("PozzoModule: Modulo ADS1118 distrutto");
}

void PozzoModule::setup()
{
    LOG_INFO("PozzoModule: setup() => Inizializzazione modulo ADS1118");
}

/**
 * Determina se il modulo vuole ricevere questo pacchetto
 */
bool PozzoModule::wantPacket(const meshtastic_MeshPacket *p)
{
    // Vogliamo ricevere tutti i messaggi testuali
    return MeshService::isTextPayload(p);
}

/**
 * Gestisce la ricezione di messaggi testuali
 */
ProcessMessage PozzoModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    // Estrae il testo dal payload
    const auto &p = mp.decoded;
    
    LOG_INFO("========================================");
    LOG_INFO("PozzoModule: Ricevuto messaggio testuale (%d byte) da 0x%0x", 
             p.payload.size, mp.from);
    
    // Stampa il messaggio ricevuto
    LOG_INFO("PozzoModule: Contenuto: %.*s", p.payload.size, p.payload.bytes);
    LOG_INFO("========================================");

    // Ora proviamo il parsing JSON se il messaggio inizia con '{'
    if (p.payload.size > 0 && p.payload.bytes[0] == '{') {
        LOG_INFO("PozzoModule: Il messaggio sembra essere JSON, provo il parsing...");
        
        // Crea una stringa null-terminated
        char jsonStr[p.payload.size + 1];
        memcpy(jsonStr, p.payload.bytes, p.payload.size);
        jsonStr[p.payload.size] = '\0';
        
        // Tenta il parsing
        JSONValue *jsonValue = JSON::Parse(jsonStr);
        
        if (jsonValue == NULL) {
            LOG_ERROR("PozzoModule: Errore nel parsing JSON");
        } else {
            LOG_INFO("PozzoModule: ✓ Parsing JSON riuscito!");
            
            // Verifica che sia un oggetto
            if (jsonValue->IsObject()) {
                JSONObject root = jsonValue->AsObject();
                
                // Stampa il tipo se presente
                if (root.find("type") != root.end() && root["type"]->IsString()) {
                    LOG_INFO("PozzoModule: - type: %s", root["type"]->AsString().c_str());
                }
                
                // Stampa le metriche se presenti
                if (root.find("metrics") != root.end() && root["metrics"]->IsArray()) {
                    JSONArray metrics = root["metrics"]->AsArray();
                    LOG_INFO("PozzoModule: - metrics: %d elementi", metrics.size());
                    
                    // Stampa ogni metrica
                    for (size_t i = 0; i < metrics.size(); i++) {
                        if (metrics[i]->IsObject()) {
                            JSONObject metric = metrics[i]->AsObject();
                            
                            // Estrai i campi
                            const char *name = "";
                            double value = 0.0;
                            const char *unit = "";
                            
                            if (metric.find("n") != metric.end() && metric["n"]->IsString()) {
                                name = metric["n"]->AsString().c_str();
                            }
                            if (metric.find("v") != metric.end() && metric["v"]->IsNumber()) {
                                value = metric["v"]->AsNumber();
                            }
                            if (metric.find("u") != metric.end() && metric["u"]->IsString()) {
                                unit = metric["u"]->AsString().c_str();
                            }
                            
                            LOG_INFO("PozzoModule:   [%d] %s = %.6f %s", i, name, value, unit);
                        }
                    }
                }
            } else {
                LOG_WARN("PozzoModule: JSON non è un oggetto");
            }
            
            // Pulisce il JSON
            delete jsonValue;
        }
        LOG_INFO("========================================");
    }

    // Controlla se il payload è "pump_on" o "pump_off"
    if (p.payload.size == 7 && memcmp(p.payload.bytes, "pump_on", 7) == 0) {
        LOG_INFO("PozzoModule: Comando ricevuto - ACCENDI POMPA");
        setPumpState(true);
        sendTelemetry();
    } else if (p.payload.size == 8 && memcmp(p.payload.bytes, "pump_off", 8) == 0) {
        LOG_INFO("PozzoModule: Comando ricevuto - SPEGNI POMPA");
        setPumpState(false);
        sendTelemetry();
    }

    // Lascia che altri moduli possano processare il messaggio
    return ProcessMessage::CONTINUE;
}


bool PozzoModule::initADS1115()
{
    ads = new Adafruit_ADS1115();
    LOG_INFO("PozzoModule: Inizializzazione ADS1115...");
    // ads1118 = new ADS1118(ADS1118_CS, ADS1118_SCLK, ADS1118_MISO, ADS1118_MOSI);
    if (!ads->begin(ADS1X15_ADDRESS, &Wire1)) {
      LOG_ERROR("PozzoModule: Errore nell'inizializzazione dell'ADS1115");
      return false;
    }else{
      LOG_INFO("PozzoModule: ADS1115.begin() completato con successo");
      return true;
    }
}

bool PozzoModule::initDisplay()
{
    lcd = new jm_LCM2004A_I2C(0x27, Wire1);
    if (!lcd->begin()) {
        LOG_ERROR("PozzoModule: Errore nell'inizializzazione del display");
        delete lcd;
        lcd = NULL;
        return false;
    }

    // Pulisce il display fisico una sola volta all'inizializzazione
    lcd->clear();
    
    // Usa il metodo ottimizzato per scrivere i messaggi iniziali
    _writeToDisplay(0, 0, "PozzoModule", true);
    _writeToDisplay(0, 1, "Initialized", true);
    
    return true;
}

void PozzoModule::initDisplayBuffer()
{
    // Inizializza il buffer con spazi
    for (uint8_t row = 0; row < LCD_ROWS; row++) {
        for (uint8_t col = 0; col < LCD_COLS; col++) {
            displayBuffer[row][col] = ' ';
        }
        displayBuffer[row][LCD_COLS] = '\0'; // Null terminator
    }
    LOG_DEBUG("PozzoModule: Buffer display inizializzato");
}

/**
 * Metodo ottimizzato per scrivere sul display LCD 20x4
 * 
 * Funzionamento:
 * - Mantiene un buffer interno (displayBuffer) che replica il contenuto del display
 * - Confronta il nuovo testo con il buffer e crea una maschera delle differenze
 * - Identifica i blocchi contigui di caratteri cambiati (es: "ABCDEFGHIL" vs "ABCXXFGXXL" -> "0001100110")
 * - Invia al display via I2C SOLO i blocchi di caratteri che sono effettivamente cambiati
 * 
 * Vantaggi:
 * - Riduce drasticamente il traffico I2C (lento)
 * - Minimizza le chiamate a setCursor() raggruppando caratteri contigui
 * - Elimina la necessità di chiamare lcd->clear() (operazione molto lenta)
 * - Evita il flickering del display
 * - Migliora la reattività generale del sistema
 * 
 * Esempio:
 *   Buffer: "Temp: 23.5C"
 *   Nuovo:  "Temp: 24.7C"
 *   Maschera: "00000011110" -> aggiorna solo "24.7" in un'unica operazione
 * 
 * @param col Colonna di partenza (0-19)
 * @param row Riga (0-3)
 * @param text Testo da scrivere (verrà troncato se troppo lungo)
 * @param forceUpdate Se true, aggiorna anche se il buffer è uguale (utile all'inizializzazione)
 */
void PozzoModule::_writeToDisplay(uint8_t col, uint8_t row, const char *text, bool forceUpdate)
{
    if (lcd == NULL) {
        LOG_ERROR("PozzoModule: Display non inizializzato");
        return;
    }

    if (row >= LCD_ROWS || col >= LCD_COLS) {
        LOG_ERROR("PozzoModule: Posizione fuori dai limiti (col:%d, row:%d)", col, row);
        return;
    }

    if (text == NULL) {
        return;
    }

    // Calcola la lunghezza effettiva da scrivere (limitata alla larghezza del display)
    uint8_t maxLen = LCD_COLS - col;
    uint8_t textLen = strlen(text);
    if (textLen > maxLen) {
        textLen = maxLen;
    }

    // Crea una maschera delle differenze (1 = cambiato, 0 = uguale)
    bool changeMask[LCD_COLS] = {false};
    bool hasChanges = false;
    
    for (uint8_t i = 0; i < textLen; i++) {
        uint8_t currentCol = col + i;
        if (forceUpdate || displayBuffer[row][currentCol] != text[i]) {
            changeMask[i] = true;
            hasChanges = true;
            displayBuffer[row][currentCol] = text[i];
        }
    }

    // Se ci sono cambiamenti, identifica i blocchi contigui e aggiornali
    if (hasChanges) {
        uint8_t i = 0;
        while (i < textLen) {
            // Cerca l'inizio del prossimo blocco di cambiamenti
            while (i < textLen && !changeMask[i]) {
                i++;
            }
            
            if (i < textLen) {
                // Trovato l'inizio di un blocco
                uint8_t blockStart = i;
                
                // Trova la fine del blocco
                while (i < textLen && changeMask[i]) {
                    i++;
                }
                uint8_t blockEnd = i - 1;
                
                // Aggiorna il blocco sul display
                lcd->setCursor(col + blockStart, row);
                for (uint8_t j = blockStart; j <= blockEnd; j++) {
                    lcd->print(displayBuffer[row][col + j]);
                }
                
                // LOG_DEBUG("PozzoModule: Aggiornato blocco da col %d a %d, row %d",  col + blockStart, col + blockEnd, row);
            }
        }
    }

    // Se il testo è più corto della riga precedente, riempi con spazi
    uint8_t firstSpace = maxLen;
    uint8_t lastSpace = 0;
    bool needsSpaces = false;
    
    for (uint8_t i = textLen; i < maxLen; i++) {
        uint8_t currentCol = col + i;
        if (displayBuffer[row][currentCol] != ' ') {
            if (!needsSpaces) {
                firstSpace = i;
                needsSpaces = true;
            }
            lastSpace = i;
            displayBuffer[row][currentCol] = ' ';
        }
    }
    
    // Se ci sono spazi da aggiungere, aggiornali come un blocco
    if (needsSpaces) {
        lcd->setCursor(col + firstSpace, row);
        for (uint8_t i = firstSpace; i <= lastSpace; i++) {
            lcd->print(' ');
        }
    }
}




void PozzoModule::sendTelemetry() {

  CustomMetricsSender customMetricsSender;
  customMetricsSender.beginMessage();
  customMetricsSender.addMetric("wat_level", waterLevelMillimeters, "mm");
  customMetricsSender.addMetric("pump_current", pumpCurrentAmps, "A");
  customMetricsSender.addMetric("pump_power", pumpCurrentPower, "W");
  customMetricsSender.addMetric("pump_state", pumpActualState , "bool");
  customMetricsSender.send();
  
}

#if HAS_SCREEN
void PozzoModule::writeToDisplay(bool firstUpdate) {
  if (lcd == NULL) {
    LOG_ERROR("PozzoModule: Display non inizializzato");
    return;
  }

  _writeToDisplay(0, 0, "Pozzo", firstUpdate);
        
  // Prepara le stringhe con i valori
  char mvStr[20];
  snprintf(mvStr, sizeof(mvStr), "mV: %.3f mt: %.3f", waterLevelMilliVolts,  waterLevelMillimeters/1000.0f);
  _writeToDisplay(0, 1, mvStr, firstUpdate);
  

  char pumpCurrentStr[20];
  snprintf(pumpCurrentStr, sizeof(pumpCurrentStr), "Pompa Watt: %.3f", pumpCurrentPower);
  _writeToDisplay(0, 2, pumpCurrentStr, firstUpdate);

  // Mostra stato pompa
  char pumpStateStr[20];
  if (pumpExternalControl) {
      snprintf(pumpStateStr, sizeof(pumpStateStr), "St: %s [ext] (%s) ", pumpActualState ? "ON " : "OFF", pumpDesiredState ? "ON" : "OFF");
  } else {
      snprintf(pumpStateStr, sizeof(pumpStateStr), "St: %s (%s)", pumpActualState ? "ON " : "OFF", pumpDesiredState ? "ON" : "OFF");
  }
  _writeToDisplay(0, 3, pumpStateStr, firstUpdate);

  return;
    // // Verifica se il display è disponibile
    // if (!screen || !screen->getDisplayDevice()) {
    //     LOG_WARN("TestModule: Display non disponibile");
    //     return;
    // }
    // // char *bannerMsg = "%d";
    // // snprintf(bannerMsg, sizeof(bannerMsg), " c:%d", counter);
    // // screen->showSimpleBanner(bannerMsg, 1000);
    // // screen->showOverlayBanner(bannerMsg, 1000);

    // OLEDDisplay *display = screen->getDisplayDevice();

    // // Pulisce il display
    // display->clear();

    // // Imposta il colore del testo
    // display->setColor(OLEDDISPLAY_COLOR::WHITE);
    // display->setTextAlignment(TEXT_ALIGN_CENTER);

    // // // Scrive il titolo
    // // display->setFont(ArialMT_Plain_16);
    // // display->drawString(display->width() / 2, 10, "Test Counter");

    // // // Scrive il numero incrementale
    // display->setFont(ArialMT_Plain_16);
    // // char counterStr[20];
    // // snprintf(counterStr, sizeof(counterStr), "%d", remainingTime);
    // display->drawString(display->width() / 2, 40, message);

    // // // Aggiunge informazioni aggiuntive
    // // display->setFont(ArialMT_Plain_10);
    // // char infoStr[50];
    // // snprintf(infoStr, sizeof(infoStr), "Uptime: %d sec", millis() / 1000);
    // // display->drawString(display->width() / 2, 70, infoStr);

    // // Aggiorna il display
    // display->display();

    // LOG_INFO("TestModule: Scritto contatore %s sul display", message.c_str());
}
#endif // HAS_SCREEN

void PozzoModule::readPumpCurrent() {
  if (ads == NULL) {
    LOG_ERROR("PozzoModule: ADS1115 non inizializzato");
    return;
  }

  if (readingMode != PUMP_CURRENT) {
    // LOG_WARN("PozzoModule: readPumpCurrent change ADS data rate to 475SPS");
    ads->setDataRate(RATE_ADS1115_860SPS);
    // ads->setDataRate(RATE_ADS1115_475SPS);
    // ads->setDataRate(RATE_ADS1115_8SPS);

     // Configura il gain per il range 0-1V
    ads->setGain(GAIN_FOUR); //< +/-1.024V range = Gain 4
                             
    // LOG_WARN("PozzoModule: readPumpCurrent cstarty continuos reading");
    ads->startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_2_3, /*continuous=*/true);
    readingMode = PUMP_CURRENT;

    // Aspetta che la prima conversione sul nuovo canale sia completa
    // while (!ads->conversionComplete()) {
    //     delay(1);  // Piccolo delay per non saturare il bus I2C
    // }

    delay(1);
    ads->getLastConversionResults();
    delay(1);

  }

  // Calcolo del valore RMS (Root Mean Square) dell'ADC per corrente AC
  // Formula: ADC_RMS = sqrt(media(ADC^2))
  // Il SCT-013-030 fornisce un segnale AC che oscilla attorno a un punto di bias
  
  long sumOfSquares = 0;
  int sampleCount = 0;
  long startTime = millis();
  
  // Campiona per 1000ms (con 8 SPS otterremo circa 8-10 campioni)
//   while (millis() - startTime < 200) {
  for (int i = 0; i < PUMP_CURRENT_READ_SAMPLES; i++) {
    // Legge il valore differenziale ADC dal canale 2-3
    int16_t adcValue = ads->getLastConversionResults();
    
    // Accumula il quadrato del valore ADC
    sumOfSquares += (long)adcValue * (long)adcValue;
    sampleCount++;
  }

  long sampleTime = millis() - startTime;

  // Calcola il valore ADC RMS: radice quadrata della media dei quadrati
  if (sampleCount > 0) {
    pumpCurrentAdcValue = (int16_t)sqrt(sumOfSquares / sampleCount);
  } else {
    pumpCurrentAdcValue = 0;
  }


  pumpCurrentMilliVolts = ads->computeVolts(pumpCurrentAdcValue);
  pumpCurrentAmps = pumpCurrentMilliVolts / SCT013_SENSITIVITY;
//   pumpCurrentPower = 220 * pumpCurrentAmps;

#if USE_FFTPUMPMONITOR
  fftPumpMonitor->pick();
#endif
#if USE_PUMPMONITOR
  pumpMonitor->pick();
  pumpCurrentPower = 220 * pumpMonitor->getAverage();
#endif

//   LOG_INFO("PozzoModule: RMS - Campioni: %d, ADC RMS: %d , MilliVolts: %.6f, Amp: %.6f, Watt: %.3f. sampleTime: %ld ms",    
//            sampleCount, pumpCurrentAdcValue, pumpCurrentMilliVolts, pumpCurrentAmps, pumpCurrentPower, sampleTime);
}


void PozzoModule::readWaterLevel() {
  if (ads == NULL) {
    LOG_ERROR("PozzoModule: ADS1115 non inizializzato");
    return;
  }
  if (readingMode != WATER_LEVEL) {
    // LOG_WARN("PozzoModule: readWaterLevel change ADS data rate to 8SPS");
    ads->setDataRate(RATE_ADS1115_8SPS);
    // Configura il gain per il range 3.3V
    ads->setGain(GAIN_ONE); // ±4.096V
    
    readingMode = WATER_LEVEL ;
  }



  int32_t waterLevelAdcValueSum = 0;  // Usa int32_t per evitare overflow con somme multiple
  for (int i = 0; i < WATER_LEVEL_READ_SAMPLES; i++) {
    waterLevelAdcValueSum += ads->readADC_SingleEnded(WATER_LEVEL_SENSOR_CHANNEL);
  }
  waterLevelAdcValue = waterLevelAdcValueSum / WATER_LEVEL_READ_SAMPLES;
  waterLevelMilliVolts = ads->computeVolts(waterLevelAdcValue);
  waterLevelMillimeters = WATER_LEVEL_SCALE_FACTOR * waterLevelMilliVolts;

// LOG_INFO("PozzoModule: waterLevelAdcValue: %d, waterLevelMilliVolts: %.2f, waterLevelMillimeters: %.0f ", waterLevelAdcValue, (double)waterLevelMilliVolts, (double)waterLevelMillimeters);

}

/**
 * Aggiorna lo stato reale della pompa leggendo la corrente dal pumpMonitor
 * Considera la pompa accesa se la corrente è > 0.5A
 */
void PozzoModule::updatePumpActualState() {
#if USE_PUMPMONITOR
    if (pumpMonitor == NULL) {
        return;
    }

    // Leggi la corrente media dal monitor
    float avgCurrent = pumpMonitor->getAverage();
    
    // Determina lo stato reale
    bool newActualState = (avgCurrent > PUMP_ON_CURRENT_THRESHOLD);
    
    // Se lo stato è cambiato rispetto a quello che conosciamo
    if (newActualState != pumpActualState) {
        // Controlla se il cambio è stato fatto dall'esterno
        // (cioè se lo stato reale è diverso da quello desiderato)
        if (newActualState != pumpDesiredState) {
            pumpExternalControl = true;
            LOG_WARN("PozzoModule: Rilevato cambio stato pompa ESTERNO - Stato reale: %s, Stato desiderato: %s",
                     newActualState ? "ON" : "OFF",
                     pumpDesiredState ? "ON" : "OFF");
        } else {
            // Il cambio è in linea con il nostro comando
            LOG_INFO("PozzoModule: Confermato cambio stato pompa - Nuovo stato: %s", 
                     newActualState ? "ON" : "OFF");
            pumpExternalControl = false;
        }

        pumpActualState = newActualState;
        sendTelemetry();
    }
#endif
}

/**
 * Imposta lo stato desiderato della pompa
 * Con un deviatore, il comando è un impulso che inverte lo stato corrente
 */
void PozzoModule::setPumpState(bool turnOn) {
    LOG_INFO("PozzoModule: Richiesta cambio stato pompa a: %s (stato attuale: %s)", 
             turnOn ? "ON" : "OFF",
             pumpActualState ? "ON" : "OFF");
    
    // Imposta lo stato desiderato
    pumpDesiredState = turnOn;
    
    // Se lo stato desiderato è diverso dallo stato attuale, invia impulso al relay
    // Con un deviatore, il relay deve solo invertire lo stato corrente
    if (pumpDesiredState != pumpActualState || (BYPASS_PUMP_MONITOR_FOR_RELAY_CONTROL && pumpDesiredState != pumpRelayState)) {
      LOG_INFO("PozzoModule: Invio impulso al relay per cambiare stato pompa");

      pumpRelayState = !pumpRelayState;
        
        digitalWrite(PIN_RELAY_PUMP, pumpRelayState ? HIGH : LOW);
        
        LOG_INFO("PozzoModule: Impulso relay inviato, stato desiderato: %s", pumpDesiredState ? "ON" : "OFF");
    } else {
        LOG_INFO("PozzoModule: Pompa già nello stato desiderato, nessun comando inviato");
    }
}

/**
 * Ritorna lo stato reale della pompa
 */
bool PozzoModule::isPumpOn() {
    return pumpActualState;
}

/**
 * Sincronizza lo stato reale con quello desiderato
 * Chiamata periodicamente per verificare che la pompa sia nello stato corretto
 */
void PozzoModule::syncPumpState() {
    // Aggiorna lo stato reale leggendo il monitor
    updatePumpActualState();
    
    // Se c'è una discrepanza tra desiderato e reale, registra un warning
    if (pumpDesiredState != pumpActualState) {
        if (pumpExternalControl) {
            LOG_WARN("PozzoModule: Pompa sotto controllo esterno - Desiderato: %s, Reale: %s",
                     pumpDesiredState ? "ON" : "OFF",
                     pumpActualState ? "ON" : "OFF");
        } else {
            // Potrebbe essere in transizione, aspetta qualche ciclo prima di segnalare errore
            LOG_DEBUG("PozzoModule: Discrepanza stato pompa - Desiderato: %s, Reale: %s",
                      pumpDesiredState ? "ON" : "OFF",
                      pumpActualState ? "ON" : "OFF");
        }
    }
}

/**
 * Resetta il flag di controllo esterno
 * Utile quando si vuole riprendere il controllo automatico della pompa
 */
void PozzoModule::resetExternalControlFlag() {
    if (pumpExternalControl) {
        LOG_INFO("PozzoModule: Reset flag controllo esterno - ripresa controllo automatico");
        pumpExternalControl = false;
        // Sincronizza lo stato desiderato con quello reale
        pumpDesiredState = pumpActualState;
    }
}

/**
 * Ritorna true se la pompa è sotto controllo esterno
 */
bool PozzoModule::isUnderExternalControl() {
    return pumpExternalControl;
}

int32_t PozzoModule::runOnce()
{
    // Inizializza l'ADS1115 al primo ciclo (dopo che Wire1 è stato configurato)
    if (!initialized && ads == NULL) {
      LOG_INFO("PozzoModule: Primo ciclo - inizializzazione ADS1115...");

      
        if (initADS1115() & initDisplay()) {
            LOG_INFO("PozzoModule: ADS1115 inizializzato con successo");
            initialized = true;
            initializationTime = millis();
            // printMemoryInfo("DOPO INIZIALIZZAZIONE");
            return 2000;
        } else {
            LOG_ERROR("PozzoModule: Errore nell'inizializzazione dell'ADS1115, riprovo tra 5 secondi");
            return 5000; // Riprova dopo 5 secondi
        }
    }

    if (!initialized) {
        LOG_INFO("PozzoModule: Non inizializzato, riprova dopo 5 secondi");
        return 5000;
    }


    Throttle::execute(
        &lastWaterLevelReadTime, 5000,
        []() {
          pozzoModule->readWaterLevel();
        //   LOG_WARN("PozzoModule: Livello acqua letto");
        },
        []() {
            pozzoModule->readPumpCurrent();
         }
    );

    // Sincronizza lo stato della pompa periodicamente
    if (!Throttle::isWithinTimespanMs(lastPumpStateCheck, PUMP_STATE_CHECK_INTERVAL_MS)) {
        syncPumpState();
        lastPumpStateCheck = millis();
    }

    // // Logica di controllo automatico della pompa basata sul livello acqua
    // // Determina se la pompa dovrebbe essere accesa o spenta
    // bool shouldPumpBeOn = (waterLevelMilliVolts > 1.5f);
    
    // // Se lo stato desiderato è diverso da quello che vorremmo, comanda il cambio
    // if (shouldPumpBeOn != pumpDesiredState) {
    //     LOG_INFO("PozzoModule: Livello acqua richiede pompa %s (livello: %.3fV)", 
    //              shouldPumpBeOn ? "ON" : "OFF", waterLevelMilliVolts);
    //     setPumpState(shouldPumpBeOn);
    // }





    if (!Throttle::isWithinTimespanMs(lastPumpSamplingCheck,2000)) {

#if USE_FFTPUMPMONITOR
            if (fftPumpMonitor->hasSamplingWarning()) {
                LOG_WARN("Campionamento lento: %.1f Hz", fftPumpMonitor->getSamplingFrequency());
            } else {
                LOG_WARN("Campionamento OK: %.1f Hz", fftPumpMonitor->getSamplingFrequency());
            }
            // Controlla anomalie pompa
            if (fftPumpMonitor->hasAnomaly()) {
                LOG_ERROR("⚠️ Pompa: THD elevato %.2f%%", fftPumpMonitor->getTHD() * 100.0f);
                // Invia allarme via Meshtastic...
            }

            // Info diagnostica
            if (fftPumpMonitor->getSampleCount() % 50 == 0) {
                LOG_INFO("Campioni: %d/256 - Fs: %.1f Hz - Tempo: %.1fs", 
                        fftPumpMonitor->getSampleCount(), 
                        fftPumpMonitor->getSamplingFrequency(),
                        fftPumpMonitor->getEstimatedTimeToAnalysis());
            }
#endif

#if USE_PUMPMONITOR

            LOG_WARN("PumpMonitor: avg: %.3fA, stddev: %.3fA, min: %.3fA, max: %.3fA, status: %s on baseline: %.3fA", pumpMonitor->getAverage(), pumpMonitor->getStdDev(), pumpMonitor->getMin(), pumpMonitor->getMax(), pumpMonitor->getStatusString(), pumpMonitor->getBaseline());

            // printMemoryInfo("DOPO PUMPMONITOR");


            // if (pumpMonitor->hasSamplingWarning()) {
            //     LOG_WARN("Campionamento lento: %.1f Hz", pumpMonitor->getSamplingFrequency());
            // } else {
            //     LOG_WARN("Campionamento OK: %.1f Hz", pumpMonitor->getSamplingFrequency());
            // }
            // // Controlla anomalie pompa
            // if (pumpMonitor->hasAnomaly()) {
            //     LOG_ERROR("⚠️ Pompa: THD elevato %.2f%%", pumpMonitor->getTHD() * 100.0f);
            //     // Invia allarme via Meshtastic...
            // }
#endif
            lastPumpSamplingCheck = millis();
    }
    // Throttle::execute(
    //     &lastPumpSamplingCheck, 2000,
    //     []() {
    //       // Controlla stato campionamento
    //         if (pozzoModule->fftPumpMonitor->hasSamplingWarning()) {
    //             LOG_WARN("Campionamento lento: %.1f Hz", fftPumpMonitor->getSamplingFrequency());
    //         }
    //     }
    //     //   ,[]() { LOG_DEBUG("Skip send telemetry due to time throttling"); }
    // );
    
    

    



    // Invia telemetria ogni TELEMETRY_UPDATE_INTERVAL_MS
    // if (lastSentToMesh == 0 || (millis() - lastSentToMesh) >= TELEMETRY_UPDATE_INTERVAL_MS) {
    //     lastSentToMesh = millis();
    //     // sendADS1118Telemetry();
    //     LOG_INFO("PozzoModule: Telemetria ADS1118 inviata");
    // }

    if (millis() - initializationTime >= 10000) {
      Throttle::execute(&lastTelemetrySentTime, TELEMETRY_UPDATE_INTERVAL_MS,
                        []() {
                          pozzoModule->sendTelemetry();
                          LOG_INFO("PozzoModule: Telemetria inviata");
                        }
                        //   ,[]() { LOG_DEBUG("Skip send telemetry due to time
                        //   throttling"); }
      );
    }


#if HAS_SCREEN
        // Aggiorna il display ogni DISPLAY_UPDATE_INTERVAL_MS
    Throttle::execute(&lastDisplayUpdateTime, DISPLAY_UPDATE_INTERVAL_MS,
                      []() {
                        pozzoModule->writeToDisplay();
                        // LOG_INFO("PozzoModule: Display aggiornato");
                      }
        //               ,[]() {
        //   LOG_DEBUG("PozzoModule: Skip display update due to time throttling");
        // }
        );


    // writeToDisplay();
#endif // HAS_SCREEN

    return 0; 
}

/**
 * Stampa informazioni dettagliate sulla memoria (Heap e PSRAM)
 */
void PozzoModule::printMemoryInfo(const char *prefix)
{
    uint32_t freeHeap = memGet.getFreeHeap();
    uint32_t totalHeap = memGet.getHeapSize();
    uint32_t usedHeap = totalHeap - freeHeap;
    
    uint32_t freePsram = memGet.getFreePsram();
    uint32_t totalPsram = memGet.getPsramSize();
    uint32_t usedPsram = totalPsram - freePsram;
    
    LOG_INFO("========== MEMORIA %s ==========", prefix);
    LOG_INFO("HEAP:  Usata: %6u bytes (%.1f%%) | Libera: %6u bytes | Totale: %u bytes", 
             usedHeap, 
             (totalHeap > 0) ? (usedHeap * 100.0f / totalHeap) : 0.0f,
             freeHeap, 
             totalHeap);
    
    if (totalPsram > 0) {
        LOG_INFO("PSRAM: Usata: %6u bytes (%.1f%%) | Libera: %6u bytes | Totale: %u bytes", 
                 usedPsram,
                 (usedPsram * 100.0f / totalPsram),
                 freePsram, 
                 totalPsram);
    } else {
        LOG_INFO("PSRAM: Non disponibile");
    }
    LOG_INFO("==========================================");
}

// #endif // USE_ADS118_MODULE
