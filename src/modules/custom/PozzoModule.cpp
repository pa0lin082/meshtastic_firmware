// #if USE_POZZO_MODULE
// #define __PROG__ "jm_LCM2004A_I2C_PrintScreen"
#include "main.h"
#include "PozzoModule.h"
#include "DebugConfiguration.h"
#include "MeshService.h"
#include "OLEDDisplayFonts.h"
#include "Router.h"
#include <Arduino.h>

#include <Adafruit_ADS1X15.h>
#include <jm_LCM2004A_I2C.h>


// Dichiarazioni delle variabili globali necessarie
extern Router *router;
extern MeshService *service;
extern graphics::Screen *screen;

PozzoModule *pozzoModule;

PozzoModule::PozzoModule()
    : concurrency::OSThread("PozzoModule"), initialized(false), lastSentToMesh(0), _gain(1), _dataRate(4)
{
    LOG_INFO("PozzoModule: Costruttore chiamato - l'inizializzazione ADS1115 avverrà in runOnce()");
}

PozzoModule::~PozzoModule()
{
    LOG_INFO("PozzoModule: Modulo ADS1118 distrutto");
}

void PozzoModule::setup()
{
    LOG_INFO("PozzoModule: setup() => Inizializzazione modulo ADS1118");
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
    }


    // per il 3.3v
    ads->setGain(GAIN_ONE);

    // Test di comunicazione per verificare se il sensore risponde
    if (testADS1115Connection()) {
        LOG_INFO("PozzoModule: Test di connessione ADS1118 completato con successo");
        return true;
    } else {
        LOG_ERROR("PozzoModule: Test di connessione ADS1118 fallito");
        return false;
    }
}

bool PozzoModule::initDisplay()
{
    lcd = new jm_LCM2004A_I2C(0x27, Wire1);
    if (!lcd->begin()) {
        LOG_ERROR("PozzoModule: Errore nell'inizializzazione del display");
        return false;
    }

    lcd->clear();
    lcd->setCursor(0, 0);
    lcd->print("PozzoModule");
    lcd->setCursor(0, 1);
    lcd->print("Initialized");
    lcd->display();
    return true;
}


bool PozzoModule::testADS1115Connection()
{
    LOG_INFO("PozzoModule: Esecuzione test di connessione ADS1115...");

    // // Test 1: Verifica temperatura interna
    // LOG_INFO("PozzoModule: Test 1 - Lettura temperatura interna");
    // const double temperature = ads1118->getTemperature();
    // LOG_INFO("PozzoModule: Temperatura interna: %f °C", temperature);

    // if (temperature == 0.0) {
    //     LOG_WARN("PozzoModule: Temperatura interna = 0, possibile problema di connessione");
    // } else if (temperature < -40.0 || temperature > 125.0) {
    //     LOG_WARN("PozzoModule: Temperatura interna fuori range (-40°C to 125°C): %f", temperature);
    // } else {
    //     LOG_INFO("PozzoModule: Temperatura interna nel range normale: %f °C", temperature);
    // }

    // // Test 2: Verifica letture su tutti i canali
    // LOG_INFO("PozzoModule: Test 2 - Lettura tutti i canali");
    // const ads1118_channel_t inputs[] = {ads1118->AIN_0, ads1118->AIN_1, ads1118->AIN_2, ads1118->AIN_3};
    // bool anyChannelActive = false;

    // for (int i = 0; i < 1; i++) {
    //     // ads1118->setInputSelected(inputs[i]);
    //     delay(100); // Aspetta stabilizzazione

    //     const double milliVolts = ads1118->getMilliVolts(inputs[i]);
    //     LOG_INFO("PozzoModule: Canale AIN_%d: %f mV", i, milliVolts);

    //     if (milliVolts != 0.0) {
    //         anyChannelActive = true;
    //         LOG_INFO("PozzoModule: Canale AIN_%d attivo: %f mV", i, milliVolts);
    //     }
    // }

    // // Test 3: Test con tensione di riferimento (se disponibile)
    // LOG_INFO("PozzoModule: Test 3 - Test con tensione di riferimento");

    // // Prova a leggere la tensione di alimentazione (se il sensore supporta questa funzione)
    // // Questo è un test aggiuntivo per verificare se il sensore risponde

    // // Test 4: Verifica configurazione
    // // LOG_INFO("PozzoModule: Test 4 - Verifica configurazione");
    // // LOG_INFO("PozzoModule: Sampling Rate attuale: %d", ads1118->getSamplingRate());
    // // LOG_INFO("PozzoModule: Full Scale Range attuale: %d", ads1118->getFullScaleRange());

    // // Valutazione finale
    // if (temperature != 0.0 || anyChannelActive) {
    //     LOG_INFO("PozzoModule: Test di connessione PASSATO - Sensore
    //     risponde"); return true;
    // } else {
    //     LOG_ERROR("PozzoModule: Test di connessione FALLITO - Nessuna
    //     risposta dal sensore"); LOG_ERROR("PozzoModule: Possibili cause:");
    //     LOG_ERROR("PozzoModule: 1. Connessioni SPI errate");
    //     LOG_ERROR("PozzoModule: 2. Alimentazione non presente");
    //     LOG_ERROR("PozzoModule: 3. Sensore danneggiato");
    //     LOG_ERROR("PozzoModule: 4. Pin CS non collegato correttamente");
    //     return true;
    // }
    return true;
}

void PozzoModule::sendADS1118Telemetry()
{
    // if (!initialized) {
    //     LOG_ERROR("PozzoModule: Modulo non inizializzato");
    //     return;
    // }

    // // Legge tutti e 4 i canali dell'ADS1118
    // float voltages[4];
    // int16_t rawValues[4];

    // const ads1118_channel_t inputs[] = {ads1118->AIN_0, ads1118->AIN_1, ads1118->AIN_2, ads1118->AIN_3};
    // for (int channel = 0; channel < 4; channel++) {
    //     voltages[channel] = ads1118->getMilliVolts(inputs[channel]);
    //     // rawValues[channel] = readChannelRaw(channel);
    //     LOG_INFO("PozzoModule: Channel %d: %.6f V (raw: %d)", channel, voltages[channel], rawValues[channel]);
    // }

    // // Crea il pacchetto di telemetria
    // meshtastic_MeshPacket *p = router->allocForSending();
    // p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

    // // Creazione di un oggetto JSON strutturato
    // JSONObject jsonObj;
    // jsonObj["type"] = new JSONValue("ads1118_metrics");

    // // Creazione dell'array di metriche
    // JSONArray metricsArray;

    // // Aggiungi le metriche per ogni canale
    // for (int channel = 0; channel < 4; channel++) {
    //     JSONObject voltageMetric;
    //     voltageMetric["name"] = new JSONValue("ads1118_ch" + String(channel) + "_voltage");
    //     voltageMetric["value"] = new JSONValue(voltages[channel]);
    //     voltageMetric["unit"] = new JSONValue("voltage");
    //     voltageMetric["channel"] = new JSONValue(channel);

    //     metricsArray.push_back(new JSONValue(voltageMetric));

    //     JSONObject rawMetric;
    //     rawMetric["name"] = new JSONValue("ads1118_ch" + String(channel) + "_raw");
    //     rawMetric["value"] = new JSONValue(rawValues[channel]);
    //     rawMetric["unit"] = new JSONValue("raw");
    //     rawMetric["channel"] = new JSONValue(channel);

    //     metricsArray.push_back(new JSONValue(rawMetric));
    // }

    // // Aggiungi l'array delle metriche all'oggetto principale
    // jsonObj["metrics"] = new JSONValue(metricsArray);

    // // Converti l'oggetto JSON in una stringa
    // JSONValue *jsonValue = new JSONValue(jsonObj);
    // std::string jsonData = jsonValue->Stringify();
    // LOG_INFO("PozzoModule: JSON generato: %s", jsonData.c_str());
    // delete jsonValue;

    // memcpy(p->decoded.payload.bytes, jsonData.c_str(), jsonData.length());
    // p->decoded.payload.size = jsonData.length();
    // p->to = NODENUM_BROADCAST;
    // p->decoded.want_response = false;
    // p->priority = meshtastic_MeshPacket_Priority_RELIABLE;
    // service->sendToMesh(p, RX_SRC_LOCAL);
}

#if HAS_SCREEN
void PozzoModule::writeToDisplay() {
  return;
    // Verifica se il display è disponibile
    if (!screen || !screen->getDisplayDevice()) {
        LOG_WARN("TestModule: Display non disponibile");
        return;
    }
    // char *bannerMsg = "%d";
    // snprintf(bannerMsg, sizeof(bannerMsg), " c:%d", counter);
    // screen->showSimpleBanner(bannerMsg, 1000);
    // screen->showOverlayBanner(bannerMsg, 1000);

    OLEDDisplay *display = screen->getDisplayDevice();

    // Pulisce il display
    display->clear();

    // Imposta il colore del testo
    display->setColor(OLEDDISPLAY_COLOR::WHITE);
    display->setTextAlignment(TEXT_ALIGN_CENTER);

    // // Scrive il titolo
    // display->setFont(ArialMT_Plain_16);
    // display->drawString(display->width() / 2, 10, "Test Counter");

    // // Scrive il numero incrementale
    display->setFont(ArialMT_Plain_16);
    // char counterStr[20];
    // snprintf(counterStr, sizeof(counterStr), "%d", remainingTime);
    display->drawString(display->width() / 2, 40, message);

    // // Aggiunge informazioni aggiuntive
    // display->setFont(ArialMT_Plain_10);
    // char infoStr[50];
    // snprintf(infoStr, sizeof(infoStr), "Uptime: %d sec", millis() / 1000);
    // display->drawString(display->width() / 2, 70, infoStr);

    // Aggiorna il display
    display->display();

    LOG_INFO("TestModule: Scritto contatore %s sul display", message.c_str());
}
#endif // HAS_SCREEN

int32_t PozzoModule::runOnce()
{
    // Inizializza l'ADS1115 al primo ciclo (dopo che Wire1 è stato configurato)
    if (!initialized && ads == NULL) {
      LOG_INFO("PozzoModule: Primo ciclo - inizializzazione ADS1115...");

      
        if (initADS1115() & initDisplay()) {
            LOG_INFO("PozzoModule: ADS1115 inizializzato con successo");
            initialized = true;
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



    if (ads != NULL) {

    //   const int16_t inputs[] = {adc0, adc1, adc2, adc3};

      for (int channel = 0; channel < 1; channel++) {
        const int16_t adcValue = ads->readADC_SingleEnded(channel);
        const float milliVolts = ads->computeVolts(adcValue);
        LOG_INFO("PozzoModule: canale %d, adcValue: %d, milliVolts:%f ", channel, adcValue, milliVolts);

        const float millimeters = 15000.0f / 3.3f * milliVolts;
        
        lcd->clear();
        lcd->setCursor(0, 0);
        lcd->print("Pozzo");

        lcd->setCursor(0, 1);
        lcd->print("mV:");
        lcd->setCursor(5, 1);
        lcd->print(String(milliVolts));

        lcd->setCursor(0, 2);
        lcd->print("mm:");
        lcd->setCursor(6, 2);
        lcd->print(String(millimeters));

        lcd->setCursor(0, 3);
        lcd->print("mt:");
        lcd->setCursor(6, 3);
        lcd->print(String(millimeters/1000.0f));

        // lcd->display();
        // inputs[channel] = milliVolts;
      }

        // const ads1118_rate_t rates[] = {ads1118->RATE_8SPS,   ads1118->RATE_16SPS,  ads1118->RATE_32SPS,  ads1118->RATE_64SPS,
        //                                 ads1118->RATE_128SPS, ads1118->RATE_250SPS, ads1118->RATE_475SPS,
        //                                 ads1118->RATE_860SPS};
        // for (int rate = 0; rate < 1; rate++) {
        //     ads1118->setSamplingRate(rates[rate]);
        //     LOG_INFO("PozzoModule: Sampling Rate: %d", rates[rate]);
        //     const ads1118_channel_t inputs[] = {ads1118->AIN_0, ads1118->AIN_1, ads1118->AIN_2,
        //                                         ads1118->AIN_3}; // AIN_0, AIN_1, AIN_2, AIN_3
        //     const double temperature = ads1118->getTemperature();
        //     LOG_INFO("PozzoModule: Temperature: %f", temperature);

        //     for (int i = 0; i < 4; i++) {
        //         // ads1118->setInputSelected(inputs[i]);
        //         // delay(100);                                                  // Aspetta che la configurazione sia applicata
        //         const double milliVolts = ads1118->getMilliVolts(inputs[i]); // Usa sempre il canale esplicito
        //         LOG_INFO("PozzoModule: Input AIN_%d, MilliVolts: %f", i, milliVolts);

        //         message = String(milliVolts);
        //         // double milliVoltsNoWait;
        //         // const bool success = ads1118->getMilliVoltsNoWait(inputs[i], milliVoltsNoWait);
        //         // if (success) {
        //         //     LOG_INFO("PozzoModule: Input AIN_%d, MilliVoltsNoWait: %f", i, milliVoltsNoWait);
        //         // } else {
        //         //     LOG_ERROR("PozzoModule: Input AIN_%d, MilliVoltsNoWait: %f", i, milliVoltsNoWait);
        //         // }
        //     }
        // }
    } else {
        LOG_ERROR("Ads1118Module: ADS1118 non inizializzato");
    }

    // Invia telemetria ogni 30 secondi
    if (lastSentToMesh == 0 || (millis() - lastSentToMesh) >= 30000) {
        lastSentToMesh = millis();
        // sendADS1118Telemetry();
        LOG_INFO("PozzoModule: Telemetria ADS1118 inviata");
    }

#if HAS_SCREEN
    writeToDisplay();
#endif // HAS_SCREEN

    return 100; // Controlla ogni 5 secondi
}

// #endif // USE_ADS118_MODULE
