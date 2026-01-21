// ============================================================================
// HX711 - Sensore Peso Arnia (4 celle di carico)
// ============================================================================

#include <Arduino.h>
#include "HX711.h" //HX711 Arduino Library v0.7.5 di Bogdan Necula 
#include "SensorValidation.h"

// ============================================================================
// CONFIGURAZIONE HARDWARE
// ============================================================================
#define HX711_DOUT_PIN 15
#define HX711_SCK_PIN  14

// ============================================================================
// OGGETTO HX711
// ============================================================================
static HX711 scale;

// ============================================================================
// VARIABILI INTERNE
// ============================================================================
static float _hx711_sogliaMin = 5.0f;          // kg
static float _hx711_sogliaMax = 80.0f;         // kg
static unsigned long _hx711_intervallo = 60000; // 3 ore
static bool _hx711_abilitato = true;
static bool _hx711_inizializzato = false;
static bool _hx711_tarato = false;

// Parametri calibrazione
static float _hx711_calibration_factor = 2280.0f;  // DEFAULT
static long  _hx711_offset = 0;

// ============================================================================
// CONFIGURAZIONE VALIDAZIONE PESO
// ============================================================================
static ConfigValidazioneSensore _configValidazionePeso = {
  .rangeMin = 0.0f,
  .rangeMax = 100.0f,
  .permettiNegativi = false,
  .richiedeTimestamp = true,
  .valoreDefault = 0.0f,
  .nomeSensore = "HX711"
};

// ============================================================================
// SETUP - Inizializzazione hardware
// ============================================================================
void setup_hx711() {
  Serial.println("-> Inizializzazione HX711...");
  esp_task_wdt_reset();

  pinMode(HX711_DOUT_PIN, INPUT);
  pinMode(HX711_SCK_PIN, OUTPUT);
  digitalWrite(HX711_SCK_PIN, LOW);

  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  esp_task_wdt_reset();

  if (!scale.is_ready()) {
    Serial.println("  ! HX711 NON pronto (controlla collegamenti)");
    _hx711_inizializzato = false;
    return;
  }

  _hx711_inizializzato = true;
  Serial.println("  + HX711 inizializzato correttamente\n");
}

// ============================================================================
// INIT - Configurazione + calibrazione
// ============================================================================
void init_hx711(SensorConfig* config) {
  esp_task_wdt_reset();

  if (!_hx711_inizializzato) {
    Serial.println("  ! HX711 non inizializzato");
    return;
  }

  if (config != NULL) {
    _hx711_sogliaMin = config->sogliaMin;
    _hx711_sogliaMax = config->sogliaMax;
    _hx711_intervallo = config->intervallo;
    _hx711_abilitato = config->abilitato;
  }

  scale.set_scale(_hx711_calibration_factor);
  esp_task_wdt_reset();
  scale.tare();
  esp_task_wdt_reset();

  _hx711_offset = scale.get_offset();
  _hx711_tarato = true;

  Serial.println("  --- HX711 configurato ---");
  Serial.print("    Soglia MIN: "); Serial.print(_hx711_sogliaMin); Serial.println(" kg");
  Serial.print("    Soglia MAX: "); Serial.print(_hx711_sogliaMax); Serial.println(" kg");
  Serial.print("    Calibration factor: "); Serial.println(_hx711_calibration_factor);
  Serial.print("    Offset: "); Serial.println(_hx711_offset);
  Serial.print("    Abilitato: "); Serial.println(_hx711_abilitato ? "SI" : "NO");
  Serial.println();
}

// ============================================================================
// TARA MANUALE
// ============================================================================
bool tare_hx711() {
  if (!_hx711_inizializzato) return false;

  scale.tare();
  _hx711_offset = scale.get_offset();
  _hx711_tarato = true;

  Serial.println("  + HX711: tara completata");
  return true;
}

// ============================================================================
// VERIFICA TARATURA
// ============================================================================
static bool verificaTaratura(int* erroreSpecifico) {
  if (!_hx711_tarato || _hx711_calibration_factor == 0) {
    *erroreSpecifico = ERR_PS_CALIBRATION_MISSING;
    return false;
  }
  return true;
}

// ============================================================================
// LETTURA PESO
// ============================================================================
RisultatoValidazione read_weight_hx711() {
  RisultatoValidazione risultato;

  if (!_hx711_abilitato) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_SENSOR_OFFLINE;
    risultato.valorePulito = 0.0f;
    strcpy(risultato.messaggioErrore, "[HX711] Sensore disabilitato");
    return risultato;
  }

  if (!_hx711_inizializzato) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_SENSOR_NOT_READY;
    risultato.valorePulito = 0.0f;
    strcpy(risultato.messaggioErrore, "[HX711] Sensore non inizializzato");
    return risultato;
  }

  int erroreSpecifico = 0;
  if (!verificaTaratura(&erroreSpecifico)) {
    risultato.valido = false;
    risultato.codiceErrore = erroreSpecifico;
    risultato.valorePulito = 0.0f;
    strcpy(risultato.messaggioErrore, "[HX711] Calibrazione mancante");
    return risultato;
  }

  long raw = scale.read_average(5);
  float peso_kg = scale.get_units(5);

  if (raw == 0 || isnan(peso_kg) || isinf(peso_kg)) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_PS_CONVERSION_FAILED;
    risultato.valorePulito = 0.0f;
    strcpy(risultato.messaggioErrore, "[HX711] Lettura non valida");
    return risultato;
  }

  unsigned long timestamp = millis();
  bool sensoreReady = scale.is_ready();

  risultato = validaDatoSensore(
    peso_kg,
    timestamp,
    sensoreReady,
    _configValidazionePeso
  );

  if (risultato.valido) {
    verificaSoglie(peso_kg, _hx711_sogliaMin, _hx711_sogliaMax, "HX711");
  }

  scale.power_down();
  delay(50);
  scale.power_up();

  return risultato;
}

// ============================================================================
// CALIBRAZIONE DA SERVER
// ============================================================================
void calibrate_hx711(float calibration_factor, long offset) {
  esp_task_wdt_reset();

  if (!_hx711_inizializzato) {
    Serial.println("  ! HX711 non inizializzato, calibrazione ignorata");
    return;
  }

  if (calibration_factor != 0) {
    _hx711_calibration_factor = calibration_factor;
  }
  _hx711_offset = offset;

  scale.set_scale(_hx711_calibration_factor);
  scale.set_offset(_hx711_offset);
  _hx711_tarato = true;
  esp_task_wdt_reset();

  Serial.println("  --- HX711 Calibrazione applicata ---");
  Serial.print("    Calibration factor: "); Serial.println(_hx711_calibration_factor);
  Serial.print("    Offset: "); Serial.println(_hx711_offset);
}

// ============================================================================
// GETTERS
// ============================================================================
unsigned long get_intervallo_hx711() { return _hx711_intervallo; }
bool is_abilitato_hx711() { return _hx711_abilitato; }
bool is_tarato_hx711() { return _hx711_tarato; }
float get_calibration_factor_hx711() { return _hx711_calibration_factor; }
