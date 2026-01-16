// ============================================================================
// DS18B20 - Sensore Temperatura Interna Arnia (REVISIONATO)
// ============================================================================

#include <OneWire.h>
#include <DallasTemperature.h>
#include "SensorValidation.h"

// ============================================================================
// CONFIGURAZIONE HARDWARE
// ============================================================================
#define ONE_WIRE_BUS 2

static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature sensors(&oneWire);
static DeviceAddress insideThermometer;

// ============================================================================
// VARIABILI INTERNE
// ============================================================================
static float _ds18b20_sogliaMin = 30.0f;
static float _ds18b20_sogliaMax = 37.0f;
static unsigned long _ds18b20_intervallo = 360000;
static bool _ds18b20_abilitato = true;
static bool _ds18b20_inizializzato = false;
static int _ds18b20_contatore = 0;

// Configurazione validazione
static ConfigValidazioneSensore _configValidazioneTemp = {
  .rangeMin = -40.0f,
  .rangeMax = 85.0f,
  .permettiNegativi = true,
  .richiedeTimestamp = true,
  .valoreDefault = 25.0f,
  .nomeSensore = "DS18B20"
};

// ============================================================================
// SETUP
// ============================================================================
void setup_ds18b20() {
  Serial.println(F("-> Inizializzazione sensore DS18B20...")); // F() macro per risparmiare RAM

  sensors.begin();

  // Controllo numero dispositivi
  int deviceCount = sensors.getDeviceCount();
  if (deviceCount == 0) {
    Serial.println(F("  ! ATTENZIONE: Nessun sensore DS18B20 trovato"));
    _ds18b20_inizializzato = false;
    return;
  }

  // Tenta di ottenere l'indirizzo del primo sensore (indice 0)
  if (sensors.getAddress(insideThermometer, 0)) {
    sensors.setResolution(insideThermometer, 12); // 12 bit resolution
    
    // NOTA: setWaitForConversion(true) è default.
    // Blocca il codice per ~750ms durante la lettura.
    // Se serve multitasking, impostare a false e gestire delay manualmente.
    sensors.setWaitForConversion(true); 
    
    _ds18b20_inizializzato = true;
    Serial.println(F("  + Sensore DS18B20 rilevato e configurato"));
  } else {
    Serial.println(F("  ! ERRORE: Trovato dispositivo ma impossibile ottenere indirizzo"));
    _ds18b20_inizializzato = false;
  }

  Serial.println(F("  + Setup DS18B20 completato\n"));
}

// ============================================================================
// INIT - Configurazione
// ============================================================================
void init_ds18b20(SensorConfig* config) {
  if (config == NULL) {
    Serial.println(F("  ! DS18B20: config NULL, uso valori default"));
    return;
  }

  _ds18b20_sogliaMin = config->sogliaMin;
  _ds18b20_sogliaMax = config->sogliaMax;
  _ds18b20_intervallo = config->intervallo;
  _ds18b20_abilitato = config->abilitato;
  _ds18b20_contatore = 0;

  Serial.println(F("  --- Config DS18B20 caricata dal DB ---"));
  Serial.print(F("    Soglia MIN: ")); Serial.println(_ds18b20_sogliaMin);
  Serial.print(F("    Soglia MAX: ")); Serial.println(_ds18b20_sogliaMax);
  Serial.print(F("    Abilitato: ")); Serial.println(_ds18b20_abilitato ? F("SI") : F("NO"));
}

// ============================================================================
// READ
// ============================================================================
RisultatoValidazione read_temperature_ds18b20() {
  RisultatoValidazione risultato;
  // Inizializza struct a zero/default per sicurezza
  risultato.valido = false;
  risultato.valorePulito = _configValidazioneTemp.valoreDefault;
  risultato.timestamp = millis();
  
  // 1. Check Abilitato
  if (!_ds18b20_abilitato) {
    risultato.codiceErrore = ERR_SENSOR_OFFLINE; // Assumendo esista in SensorValidation.h
    strncpy(risultato.messaggioErrore, "[DS18B20] Sensore disabilitato", sizeof(risultato.messaggioErrore) - 1);
    return risultato;
  }

  // 2. Check Inizializzato
  if (!_ds18b20_inizializzato) {
    risultato.codiceErrore = ERR_SENSOR_NOT_READY; 
    strncpy(risultato.messaggioErrore, "[DS18B20] Sensore non init", sizeof(risultato.messaggioErrore) - 1);
    return risultato;
  }

  // 3. Lettura Fisica
  // ATTENZIONE: Questa chiamata blocca per 750ms a 12-bit!
  sensors.requestTemperatures(); 
  float tempC = sensors.getTempC(insideThermometer);

  // 4. Verifica connessione fisica
  bool sensoreReady = (tempC != DEVICE_DISCONNECTED_C); 

  // Se disconnesso, logghiamo subito l'errore specifico
  if (!sensoreReady) {
      // Potresti voler provare a reinizializzare qui se la connessione è persa
      // setup_ds18b20(); // Opzionale: tentativo di recupero
  }

  // 5. Validazione Logica (Range, NaN, ecc.)
  risultato = validaDatoSensore(
    tempC,
    millis(),
    sensoreReady,
    _configValidazioneTemp
  );

  // 6. Verifica Soglie
  if (risultato.valido) {
    verificaSoglie(risultato.valorePulito, _ds18b20_sogliaMin, _ds18b20_sogliaMax, "DS18B20");
    _ds18b20_contatore++;
  }

  return risultato;
}

// ... Getters invariati ...
unsigned long get_intervallo_ds18b20() { return _ds18b20_intervallo; }
bool is_abilitato_ds18b20() { return _ds18b20_abilitato; }
int get_contatore_ds18b20() { return _ds18b20_contatore; }
void reset_contatore_ds18b20() { _ds18b20_contatore = 0; }
