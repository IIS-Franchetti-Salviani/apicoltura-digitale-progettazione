// ============================================================================
// SHT21 (HTU21D) - Sensore Temperatura e Umidita Ambientale
// ============================================================================

#include <Wire.h>
#include <esp_task_wdt.h>
#include "Adafruit_HTU21DF.h" //Adafruit v1.1.2 + Adafruit.BusIO.Register
#include "SensorValidation.h"

// ============================================================================
// CONFIGURAZIONE HARDWARE
// ============================================================================
#define I2C_SDA 15
#define I2C_SCL 14

static Adafruit_HTU21DF sht21 = Adafruit_HTU21DF();

// ============================================================================
// VARIABILI INTERNE - UMIDITA
// ============================================================================
static float _sht21_umidita_sogliaMin = 40.0f;
static float _sht21_umidita_sogliaMax = 70.0f;
static unsigned long _sht21_umidita_intervallo = 360000;
static bool _sht21_umidita_abilitato = true;
static int _sht21_umidita_contatore = 0;
static unsigned long _last_read_time_hum = 0;

// ============================================================================
// VARIABILI INTERNE - TEMPERATURA
// ============================================================================
static float _sht21_temp_sogliaMin = 20.0f;
static float _sht21_temp_sogliaMax = 40.0f;
static unsigned long _sht21_temp_intervallo = 360000;
static bool _sht21_temp_abilitato = true;
static int _sht21_temp_contatore = 0;
static unsigned long _last_read_time_temp = 0;

// ============================================================================
// STATO SENSORE
// ============================================================================
static bool _sht21_inizializzato = false;

// ============================================================================
// CONFIG VALIDAZIONE
// ============================================================================
static ConfigValidazioneSensore _configValidazioneUmidita = {
  .rangeMin = 0.0f,
  .rangeMax = 100.0f,
  .permettiNegativi = false,
  .richiedeTimestamp = true,
  .valoreDefault = 50.0f,
  .nomeSensore = "SHT21_HUM"
};

// NOTE: _configValidazioneTemp is defined in ds18b20.ino, don't redefine it here

// ============================================================================
// DB READ / WRITE (STUB) - RISERVATO AL GRUPPO DATABASE
// ============================================================================

static void letturaConfigDalDB_sht21_humidity() {
  // STUB: lettura configurazione umidità dal DB
}

static void letturaConfigDalDB_sht21_temperature() {
  // STUB: lettura configurazione temperatura dal DB
}

static void scritturaDatoNelDB_sht21_humidity(const RisultatoValidazione* risultato) {
  // STUB: scrittura umidità nel DB
  (void)risultato;
}

static void scritturaDatoNelDB_sht21_temperature(const RisultatoValidazione* risultato) {
  // STUB: scrittura temperatura nel DB
  (void)risultato;
}

// ============================================================================
// SETUP
// ============================================================================
void setup_sht21() {
  Serial.println("-> Inizializzazione sensore SHT21...");
  esp_task_wdt_reset();

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setPins(15, 14);
  esp_task_wdt_reset();

  if (!sht21.begin()) {
    Serial.println("  ! ERRORE: Sensore SHT21 non trovato!");
    _sht21_inizializzato = false;
  } else {
    _sht21_inizializzato = true;
    Serial.println("  + Sensore SHT21 inizializzato");
  }
  esp_task_wdt_reset();

  Serial.println("  + Setup SHT21 completato\n");
}

// ============================================================================
// INIT UMIDITA
// ============================================================================
void init_humidity_sht21(SensorConfig* config) {
  if (config == NULL) return;

  _sht21_umidita_sogliaMin = config->sogliaMin;
  _sht21_umidita_sogliaMax = config->sogliaMax;
  _sht21_umidita_intervallo = config->intervallo;
  _sht21_umidita_abilitato = config->abilitato;
  _sht21_umidita_contatore = 0;

  Serial.println("  --- Config SHT21 Umidita caricata dal DB ---");
  Serial.print("    Soglia MIN: "); Serial.print(_sht21_umidita_sogliaMin); Serial.println(" %");
  Serial.print("    Soglia MAX: "); Serial.print(_sht21_umidita_sogliaMax); Serial.println(" %");
  Serial.print("    Intervallo: "); Serial.print(_sht21_umidita_intervallo / 1000.0); Serial.println(" sec");
  Serial.print("    Abilitato: "); Serial.println(_sht21_umidita_abilitato ?  "SI" : "NO");
}

// ============================================================================
// INIT TEMPERATURA
// ============================================================================
void init_temperature_sht21(SensorConfig* config) {
  if (config == NULL) return;

  _sht21_temp_sogliaMin = config->sogliaMin;
  _sht21_temp_sogliaMax = config->sogliaMax;
  _sht21_temp_intervallo = config->intervallo;
  _sht21_temp_abilitato = config->abilitato;
  _sht21_temp_contatore = 0;

  Serial.println("  --- Config SHT21 Temperatura caricata dal DB ---");
  Serial.print("    Soglia MIN:  "); Serial.print(_sht21_temp_sogliaMin); Serial.println(" C");
  Serial.print("    Soglia MAX: "); Serial.print(_sht21_temp_sogliaMax); Serial.println(" C");
  Serial.print("    Intervallo: "); Serial.print(_sht21_temp_intervallo / 1000.0); Serial.println(" sec");
  Serial.print("    Abilitato: "); Serial.println(_sht21_temp_abilitato ? "SI" : "NO");
}

// ============================================================================
// READ HUMIDITY
// ============================================================================
RisultatoValidazione read_humidity_sht21() {
  RisultatoValidazione risultato;

  // 1. CHECK ABILITATO
  if (!_sht21_umidita_abilitato) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_SENSOR_OFFLINE;
    risultato.valorePulito = _configValidazioneUmidita.valoreDefault;
    strcpy(risultato.messaggioErrore, "[SHT21_HUM] Sensore disabilitato");
    return risultato;
  }

  // 2. CHECK INIZIALIZZAZIONE
  if (!_sht21_inizializzato) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_SENSOR_NOT_READY;
    risultato.valorePulito = _configValidazioneUmidita.valoreDefault;
    strcpy(risultato.messaggioErrore, "[SHT21_HUM] Sensore non inizializzato");
    return risultato;
  }

  // 3. CHECK INTERVALLO
  if (millis() - _last_read_time_hum < _sht21_umidita_intervallo) {
    risultato.valido = false;
    risultato.codiceErrore = 0;
    risultato.valorePulito = _configValidazioneUmidita.valoreDefault;
    strcpy(risultato.messaggioErrore, "Intervallo non trascorso");
    return risultato;
  }

  // Aggiorniamo il tempo dell'ultima lettura
  _last_read_time_hum = millis();

  // 4. LETTURA E VALIDAZIONE
  float umidita = sht21.readHumidity();
  bool sensoreReady = ! isnan(umidita);
  unsigned long timestamp = millis();

  risultato = validaDatoSensore(
    umidita,
    timestamp,
    sensoreReady,
    _configValidazioneUmidita
  );

  if (risultato.valido) {
    verificaSoglie(risultato. valorePulito,
                   _sht21_umidita_sogliaMin,
                   _sht21_umidita_sogliaMax,
                   "SHT21_HUM");
    _sht21_umidita_contatore++;
    scritturaDatoNelDB_sht21_humidity(&risultato);
  }

  return risultato;
}

// ============================================================================
// READ TEMPERATURE
// ============================================================================
RisultatoValidazione read_temperature_sht21() {
  RisultatoValidazione risultato;

  // 1. CHECK ABILITATO
  if (!_sht21_temp_abilitato) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_SENSOR_OFFLINE;
    risultato.valorePulito = 25.0f; // Default per temperatura
    strcpy(risultato.messaggioErrore, "[SHT21_TEMP] Sensore disabilitato");
    return risultato;
  }

  // 2. CHECK INIZIALIZZAZIONE
  if (!_sht21_inizializzato) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_SENSOR_NOT_READY;
    risultato.valorePulito = 25.0f;
    strcpy(risultato.messaggioErrore, "[SHT21_TEMP] Sensore non inizializzato");
    return risultato;
  }

  // 3. CHECK INTERVALLO
  if (millis() - _last_read_time_temp < _sht21_temp_intervallo) {
    risultato.valido = false;
    risultato.codiceErrore = 0;
    risultato.valorePulito = 25.0f;
    strcpy(risultato.messaggioErrore, "Intervallo non trascorso");
    return risultato;
  }

  _last_read_time_temp = millis();

  // 4. LETTURA E VALIDAZIONE
  float temperatura = sht21.readTemperature();
  bool sensoreReady = !isnan(temperatura);
  unsigned long timestamp = millis();

  risultato = validaDatoSensore(
    temperatura, timestamp, sensoreReady, 
    {-40.0f, 125.0f, true, true, 25.0f, "SHT21_TEMP"}
  );

  if (risultato.valido) {
    verificaSoglie(risultato.valorePulito,
                   _sht21_temp_sogliaMin,
                   _sht21_temp_sogliaMax,
                   "SHT21_TEMP");
    _sht21_temp_contatore++;
    scritturaDatoNelDB_sht21_temperature(&risultato);
  }

  return risultato;
}

// ============================================================================
// GETTERS - Accesso ai parametri di configurazione
// ============================================================================
unsigned long get_intervallo_humidity_sht21() {
  return _sht21_umidita_intervallo;
}

unsigned long get_intervallo_temperature_sht21() {
  return _sht21_temp_intervallo;
}

bool is_abilitato_humidity_sht21() {
  return _sht21_umidita_abilitato;
}

bool is_abilitato_temperature_sht21() {
  return _sht21_temp_abilitato;
}

bool is_inizializzato_sht21() {
  return _sht21_inizializzato;
}
