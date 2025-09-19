#include "DHT.h"
#include "ADCModule.h"
#include "configuration.h"
#include "esp32-hal-adc.h"
#include "hal/gpio_types.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <Arduino.h>
#include "Router.h"
#include "MeshService.h"
#include "meshtastic/mesh.pb.h"
#include "meshtastic/telemetry.pb.h"
#include "serialization/JSON.h"
#include "mesh/generated/meshtastic/portnums.pb.h"
#include "configuration.h"

// Dichiarazioni delle variabili globali necessarie
extern Router *router;
extern MeshService *service;

// Pin ADC1_H6 - questo è tipicamente il pin GPIO 7 su ESP32

#define ADC_Ctrl 37
#define ADC_Pin 6
#define ADC_ATTEN ADC_11db
#define ADC_ATTEN_T ADC_ATTEN_DB_12

// Pin DHT - configurabile da file di configurazione esterno
#define DHT_Pin 5
#define DHTTYPE DHT11

//ADC Attenuation
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_12

#define ADC_RESOLUTION 12
#define ADC_EXAMPLE_CALI_SCHEME     ESP_ADC_CAL_VAL_EFUSE_TP_FIT
#define VREF 1100

static esp_adc_cal_characteristics_t adc_chars;
static esp_adc_cal_characteristics_t adc1_chars;
static esp_adc_cal_characteristics_t adc2_chars;
const int SAMPLES = 100;

ADCModule *adcModule;

ADCModule::ADCModule() : concurrency::OSThread("ADCModule"), initialized(false), adcPin(ADC_Pin), dht(nullptr)
{
    LOG_INFO("ADCModule: Inizializzazione modulo ADC per pin %d\n", adcPin);
    
    // Inizializza il DHT
    dht = new DHT(DHT_Pin, DHTTYPE);
    dht->begin();
    
    // Inizializza il pin ADC
    if (initADC()) {
        LOG_INFO("ADCModule: Pin ADC %d inizializzato con successo\n", adcPin);
        initialized = true;
    } else {
        LOG_ERROR("ADCModule: Errore nell'inizializzazione del pin ADC %d\n", adcPin);
        initialized = false;
    }
}

ADCModule::~ADCModule()
{
    if (dht) {
        delete dht;
        dht = nullptr;
    }
    LOG_INFO("ADCModule: Modulo ADC distrutto\n");
}

void ADCModule::setup()
{
    LOG_INFO("ADCModule: setup() => Inizializzazione modulo ADC\n");
}

bool ADCModule::initADC()
{
    // // Configure ESP32 ADC attenuation (voltage range)
    // analogSetAttenuation(static_cast<adc_attenuation_t>(ADC_ATTEN)); 
    // analogReadResolution(ADC_RESOLUTION);
    // analogSetPinAttenuation(adcPin, ADC_ATTEN);


    // esp_err_t ret;
    // bool cali_enable = false;

    // ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);

    // if (ret == ESP_ERR_NOT_SUPPORTED) {
    //     LOG_INFO( "ADCModule: Calibration scheme not supported, skip software calibration");
    // } else if (ret == ESP_ERR_INVALID_VERSION) {
    //     LOG_INFO( "ADCModule: eFuse not burnt, skip software calibration");
    // } else if (ret == ESP_OK) {
    //     LOG_INFO( "ADCModule: eFuse burnt, software calibration");
    //     // cali_enable = true;
    //     // esp_adc_cal_characterize(ADC_UNIT_1, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_12, 0, &adc1_chars);
    //     // esp_adc_cal_characterize(ADC_UNIT_2, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_12, 0, &adc2_chars);
    // } else {
    //     LOG_INFO( "Invalid arg");
    // }


    // // Characterize and check calibration scheme
    // esp_adc_cal_value_t efuse_config = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_T, ADC_WIDTH_BIT_12, VREF, &adc_chars);
    // switch (efuse_config) {

    //     case ESP_ADC_CAL_VAL_EFUSE_VREF:
    //     LOG_INFO("ADCModule: Characterized using eFuse Vref");
    //     break;

    //     case ESP_ADC_CAL_VAL_EFUSE_TP:
    //     LOG_INFO("ADCModule: Characterized using Two Point Value stored in eFuse");
    //     break;

    //     case ESP_ADC_CAL_VAL_DEFAULT_VREF:
    //     LOG_INFO("ADCModule: Characterized using Default Vref (no eFuse)");
    //     break;

    //     case ESP_ADC_CAL_VAL_EFUSE_TP_FIT:
    //     LOG_INFO("ADCModule: Characterization based on Two Point values and fitting curve coefficients stored in eFuse");
    //     break;
        
    //     default:
    //     LOG_INFO("ADCModule: ALERT, ADC calibration failed");
    // }

    // LOG_INFO("ADCModule: Gradient of ADC-Voltage curve: %d\n", adc_chars.coeff_a);
    // LOG_INFO("ADCModule: Offset of ADC-Voltage curve: %d\n", adc_chars.coeff_b);
    // LOG_INFO("ADCModule: Vref used by lookup table %d mV\n", adc_chars.vref);
    // LOG_INFO("ADCModule: adc_num of ADC-Voltage curve %d mV\n", adc_chars.adc_num);
    // LOG_INFO("ADCModule: atten of ADC-Voltage curve %d mV\n", adc_chars.atten);
    // LOG_INFO("ADCModule: bit_width of ADC-Voltage curve %d mV\n", adc_chars.bit_width);
    // LOG_INFO("ADCModule: low_curve of ADC-Voltage curve %d mV\n", adc_chars.low_curve);
    // LOG_INFO("ADCModule: high_curve of ADC-Voltage curve %d mV\n", adc_chars.high_curve);
    // LOG_INFO("ADCModule: version of ADC-Voltage curve %d mV\n", adc_chars.version);







    // Configura il pin come input analogico
    pinMode(adcPin, INPUT);
    // Disabilita eventuali pull-up/pull-down interni
    // Su ESP32, questo è gestito automaticamente per i pin analogici
    
    LOG_INFO("ADCModule: Pin %d configurato come input analogico\n", adcPin);
    
    return true;
}

int ADCModule::readADCValue()
{
    if (!initialized) {
        LOG_ERROR("ADCModule: Modulo non inizializzato\n");
        return -1;
    }
    
    // Legge il valore analogico dal pin
    int value = analogReadMilliVolts(adcPin);
    
    return value;
}

float ADCModule::readDHTValue()
{
    if (!dht) {
        LOG_ERROR("ADCModule: DHT non inizializzato\n");
        return -1.0f;
    }
    
    float h = dht->readHumidity();
    float t = dht->readTemperature();
    LOG_INFO("ADCModule: Humidity: %.3f\n", h);
    LOG_INFO("ADCModule: Temperature: %.3f\n", t);

    meshtastic_Telemetry m = meshtastic_Telemetry_init_zero;
    m.which_variant = meshtastic_Telemetry_environment_metrics_tag;
    m.variant.environment_metrics.has_relative_humidity = true;
    m.variant.environment_metrics.relative_humidity = h;
    m.variant.environment_metrics.has_temperature = true;
    m.variant.environment_metrics.temperature = t;
  

    meshtastic_MeshPacket *p = router->allocForSending();
    p->decoded.portnum = meshtastic_PortNum_TELEMETRY_APP;
    p->decoded.payload.size = pb_encode_to_bytes(p->decoded.payload.bytes, sizeof(p->decoded.payload.bytes), &meshtastic_Telemetry_msg, &m);
    p->to = NODENUM_BROADCAST;
    p->decoded.want_response = false;
    p->priority = meshtastic_MeshPacket_Priority_RELIABLE;
    service->sendToMesh(p, RX_SRC_LOCAL); 


    return h;
}


int32_t ADCModule::runOnce()
{
    if (!initialized) {
        // Se non siamo inizializzati, riprova dopo 2 secondi
        return 2000;
    }
    
    // Legge il valore dal pin ADC


    
    // analogSetAttenuation(ADC_ATTENDB_MAX);
    // analogSetAttenuation(ADC_ATTEN_DB_11);
    digitalWrite(ADC_Ctrl,LOW);
    delay(100);

    int adcValue = 0;
    int adcRawValue = 0;
    float adcRawMilliVoltsValue = 0;

    for(int i = 0; i < SAMPLES; i++) {
        adcValue += analogRead(adcPin);
        adcRawValue += analogReadRaw(adcPin);
        adcRawMilliVoltsValue += analogReadMilliVolts(adcPin);  // per ora migliori letture
        delayMicroseconds(500);   // ~50Hz sampling
    }
    adcValue = adcValue / SAMPLES;
    adcRawValue = adcRawValue / SAMPLES;
    adcRawMilliVoltsValue = adcRawMilliVoltsValue / SAMPLES/ 1000.0f;




    float voltage = (adcValue * 3.3) / 4095.0;
    float voltageRaw = (adcRawValue * 3.3) / 4095.0;
    float voltageRawMilliVolts = adcRawMilliVoltsValue;
    // delay(100);
    // //Characterize ADC at particular atten
    // esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    // // esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    // // //Check type of calibration value used to characterize ADC
    // // if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    // //     printf("eFuse Vref");
    // // } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    // //     printf("Two Point");
    // // } else {
    // //     printf("Default");
    // // }
    // uint32_t reading2 =  adc1_get_raw(ADC1_CHANNEL_5);
    // uint32_t voltage2 = esp_adc_cal_raw_to_voltage(reading2, adc_chars);

    digitalWrite(ADC_Ctrl,HIGH);



    if (adcValue >= 0) {
        meshtastic_MeshPacket *p = router->allocForSending();
        p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

        // Creazione di un oggetto JSON strutturato
        JSONObject jsonObj;
        jsonObj["type"] = new JSONValue("custom_metrics");
        
        // Creazione dell'array di metriche
        JSONArray metricsArray;
        
        // Creazione dell'oggetto metrica per voltage
        JSONObject voltageMetric;
        voltageMetric["name"] = new JSONValue("voltage_raw_milli_volts");
        voltageMetric["value"] = new JSONValue(voltageRawMilliVolts);
        voltageMetric["unit"] = new JSONValue("voltage");
        
        // Log per verificare la precisione
        LOG_INFO("ADCModule: Valore float originale: %.6f", voltageRawMilliVolts);
        
        // Aggiungi la metrica all'array
        metricsArray.push_back(new JSONValue(voltageMetric));
        
        // Aggiungi l'array delle metriche all'oggetto principale
        jsonObj["metrics"] = new JSONValue(metricsArray);

        // Converti l'oggetto JSON in una stringa
        JSONValue *jsonValue = new JSONValue(jsonObj);
        std::string jsonData = jsonValue->Stringify();
        LOG_INFO("ADCModule: JSON generato: %s", jsonData.c_str());
        delete jsonValue;

        memcpy(p->decoded.payload.bytes, jsonData.c_str(), jsonData.length());
        p->decoded.payload.size = jsonData.length();
        service->sendToMesh(p, RX_SRC_LOCAL);
      
        // Stampa il valore letto
        LOG_INFO("--------------------------------");
        LOG_INFO("ADCModule: ADC       dal pin %d: %d   Voltage: %.3f V\n", adcPin, adcValue, voltage);
        LOG_INFO("ADCModule: ADC (raw) dal pin %d: %d   Voltage: %.3f V\n", adcPin, adcRawValue, voltageRaw);
        LOG_INFO("ADCModule: ADC (mv)  dal pin %d: %.3f   Voltage: %.3f V\n", adcPin, adcRawMilliVoltsValue, voltageRawMilliVolts);
        // LOG_INFO("ADCModule: reading: %d  voltage: %.3f V\n", reading2, voltage2);
        
        // Opzionalmente, converte in voltaggio (per ESP32 con 3.3V di riferimento)
        // float voltage = (adcValue * 3.3) / 4095.0; // 12-bit ADC
        // LOG_INFO("ADCModule: Voltaggio stimato: %.3f V\n", voltage);
    } else {
        LOG_ERROR("ADCModule: Errore nella lettura del valore ADC\n");
    }

    float h = readDHTValue();
    
    // Ritorna ogni 1 secondo (1000ms)
    return 30000;
}
