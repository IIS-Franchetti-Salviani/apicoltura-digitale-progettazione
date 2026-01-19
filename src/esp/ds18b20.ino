// ============================================================================
// DS18B20 - MODULO INTEGRATO (LETTURA + VALIDAZIONE + SOGLIE)
// ============================================================================

#include <OneWire.h> //Paul Stoffregen 2.3.8 https://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h> //Miles Burton 4.0.5
#include <esp_task_wdt.h>
#include "SensorValidation.h"

// --- CONFIGURAZIONE HARDWARE ---
#define ONE_WIRE_BUS 2

static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature sensors(&oneWire);
static DeviceAddress insideThermometer;

// --- STATO E PARAMETRI (CONFIGURABILI) ---
static float _ds18b20_sogliaMin = 30.0f;
static float _ds18b20_sogliaMax = 37.0f;
static unsigned long _ds18b20_intervallo = 360000;
static bool _ds18b20_abilitato = true;
static bool _ds18b20_inizializzato = false;
static int _ds18b20_contatore = 0;

// Configurazione per SensorValidation.h
static ConfigValidazioneSensore _configValidazioneTemp = {
  .rangeMin = -40.0f,
  .rangeMax = 85.0f,
  .permettiNegativi = true,
  .richiedeTimestamp = true,
  .valoreDefault = 25.0f,
  .nomeSensore = "DS18B20"
};

// ============================================================================
// UTILITY: STAMPA INDIRIZZO (FORWARD DECLARATION)
// ============================================================================
void printAddress(uint8_t* deviceAddress);

// ============================================================================
// SETUP - Inizializzazione Fisica
// ============================================================================
void setup_ds18b20() {
  Serial.println(F("-> Avvio scansione bus OneWire..."));

  esp_task_wdt_reset();
  sensors.begin();
  esp_task_wdt_reset();

  int deviceCount = sensors.getDeviceCount();
  esp_task_wdt_reset();

  if (deviceCount == 0) {
    Serial.println(F("  ! ERRORE: Nessun sensore DS18B20 trovato sul pin 2"));
    _ds18b20_inizializzato = false;
    return;
  }

  Serial.print(F("  + Sensori trovati: "));
  Serial.println(deviceCount);

  if (!sensors.getAddress(insideThermometer, 0)) {
    Serial.println(F("  ! ERRORE: Impossibile recuperare indirizzo fisico"));
    _ds18b20_inizializzato = false;
  } else {
    Serial.print(F("  + Indirizzo Sensore: "));
    printAddress(insideThermometer);
    Serial.println();

    sensors.setResolution(insideThermometer, 12);
    sensors.setWaitForConversion(true);
    esp_task_wdt_reset();

    Serial.print(F("  + Modalità alimentazione: "));
    Serial.println(sensors.isParasitePowerMode() ? F("PARASITE") : F("ESTERNA"));

    _ds18b20_inizializzato = true;
    Serial.println(F("  + DS18B20 pronto."));
  }
}

// ============================================================================
// INIT - Configurazione Parametri
// ============================================================================
void init_ds18b20(SensorConfig* config) {
  if (config == NULL) {
    Serial.println("  ! DS18B20: config NULL, uso valori default");
    return;
  }

  _ds18b20_sogliaMin = config->sogliaMin;
  _ds18b20_sogliaMax = config->sogliaMax;
  _ds18b20_intervallo = config->intervallo;
  _ds18b20_abilitato = config->abilitato;
  _ds18b20_contatore = 0;

  Serial.println(F("  --- Config DS18B20 caricata ---"));
  Serial.print("    Soglia MIN: "); Serial.print(_ds18b20_sogliaMin); Serial.println(" °C");
  Serial.print("    Soglia MAX: "); Serial.print(_ds18b20_sogliaMax); Serial.println(" °C");
  Serial.print("    Intervallo: "); Serial.print(_ds18b20_intervallo / 1000); Serial.println(" sec");
  Serial.print("    Abilitato: "); Serial.println(_ds18b20_abilitato ? "SI" : "NO");
}

// ============================================================================
// READ - Lettura, Validazione e Controllo Soglie
// ============================================================================
RisultatoValidazione read_temperature_ds18b20() {
  RisultatoValidazione risultato;
  risultato.valido = false;
  risultato.valorePulito = _configValidazioneTemp.valoreDefault;
  risultato.timestamp = millis();

  // 1. Controllo se disabilitato
  if (!_ds18b20_abilitato) {
    risultato.codiceErrore = ERR_SENSOR_OFFLINE;
    strncpy(risultato.messaggioErrore, "[DS18B20] Sensore disabilitato", sizeof(risultato.messaggioErrore));
    return risultato;
  }

  // 2. Controllo se inizializzato
  if (!_ds18b20_inizializzato) {
    risultato.codiceErrore = ERR_SENSOR_NOT_READY;
    strncpy(risultato.messaggioErrore, "[DS18B20] HW non inizializzato", sizeof(risultato.messaggioErrore));
    return risultato;
  }

  // 3. Lettura fisica
  sensors.requestTemperatures(); 
  float tempC = sensors.getTempC(insideThermometer);

  // 4. Controllo connessione fisica
  bool isConnected = (tempC != DEVICE_DISCONNECTED_C);

  // 5. Validazione con SensorValidation.h
  risultato = validaDatoSensore(
    tempC, 
    risultato.timestamp, 
    isConnected, 
    _configValidazioneTemp
  );

  // 6. Controllo soglie se valido
  if (risultato.valido) {
    int alertCode = verificaSoglie(risultato.valorePulito, _ds18b20_sogliaMin, _ds18b20_sogliaMax, "DS18B20");
    
    // Se c'è un alert, aggiorna il codice errore ma mantieni valido=true
    if (alertCode != STATUS_OK) {
      risultato.codiceErrore = alertCode;
    }
    
    _ds18b20_contatore++;
    
    Serial.print(F("[DS18B20] Temperatura: "));
    Serial.print(risultato.valorePulito);
    Serial.println(F(" °C"));
  } else {
    Serial.print(F("! Errore Lettura DS18B20: "));
    Serial.println(risultato.messaggioErrore);
  }

  return risultato;
}

// ============================================================================
// UTILITY: STAMPA INDIRIZZO (IMPLEMENTAZIONE)
// ============================================================================
void printAddress(uint8_t* deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// ============================================================================
// GETTERS
// ============================================================================
unsigned long get_intervallo_ds18b20() { 
  return _ds18b20_intervallo; 
}

bool is_abilitato_ds18b20() { 
  return _ds18b20_abilitato; 
}
