// ============================================================================
// HX711 - Sensore Peso Arnia (HX711_ADC library)
// ============================================================================
// Libreria: HX711_ADC by Olav Kallhovd
// https://github.com/olkal/HX711_ADC
// ============================================================================

#include <HX711_ADC.h>
#include "SensorValidation.h"

// ============================================================================
// CONFIGURAZIONE HARDWARE
// ============================================================================
const int HX711_DOUT_PIN = 14;
const int HX711_SCK_PIN = 15;

// ============================================================================
// OGGETTO HX711_ADC
// ============================================================================
HX711_ADC LoadCell(HX711_DOUT_PIN, HX711_SCK_PIN);

// ============================================================================
// VARIABILI INTERNE
// ============================================================================
static float _hx711_sogliaMin = 5.0f;          // kg
static float _hx711_sogliaMax = 80.0f;         // kg
static unsigned long _hx711_intervallo = 60000; // 1 minuto default
static bool _hx711_abilitato = true;
static bool _hx711_inizializzato = false;
static bool _hx711_tarato = false;
static unsigned long _last_read_time = 0;

// Parametri calibrazione
static float _hx711_calibration_factor = 696.0f;  // Valore di calibrazione

// ============================================================================
// CONFIGURAZIONE VALIDAZIONE PESO
// ============================================================================
static ConfigValidazioneSensore _configValidazionePeso = {
  .rangeMin = 0.0f,
  .rangeMax = 150.0f,
  .permettiNegativi = false,
  .richiedeTimestamp = true,
  .valoreDefault = 0.0f,
  .nomeSensore = "HX711"
};

// ============================================================================
// SETUP - Inizializzazione hardware con tara automatica
// ============================================================================
void setup_hx711() {
  Serial.println("-> Inizializzazione HX711 (HX711_ADC)...");

  LoadCell.begin();

  // Tempo di stabilizzazione e tara automatica
  unsigned long stabilizingTime = 2000;
  boolean doTare = true;

  Serial.println("  Stabilizzazione e tara in corso...");
  LoadCell.start(stabilizingTime, doTare);

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("  ! TIMEOUT: Controlla cablaggio HX711");
    Serial.println("    DOUT -> GPIO 14");
    Serial.println("    SCK  -> GPIO 15");
    Serial.println("    VCC  -> 5V");
    Serial.println("    GND  -> GND");
    _hx711_inizializzato = false;
    return;
  }

  // Imposta fattore di calibrazione
  LoadCell.setCalFactor(_hx711_calibration_factor);

  _hx711_inizializzato = true;
  _hx711_tarato = true;

  Serial.println("  + HX711 inizializzato e tarato");
  Serial.print("    Calibration factor: ");
  Serial.println(_hx711_calibration_factor);
  Serial.println();
}

// ============================================================================
// INIT - Configurazione parametri da server
// ============================================================================
void init_hx711(SensorConfig* config) {
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

  Serial.println("  --- HX711 configurato ---");
  Serial.print("    Soglia MIN: "); Serial.print(_hx711_sogliaMin); Serial.println(" kg");
  Serial.print("    Soglia MAX: "); Serial.print(_hx711_sogliaMax); Serial.println(" kg");
  Serial.print("    Intervallo: "); Serial.print(_hx711_intervallo / 1000); Serial.println(" sec");
  Serial.print("    Abilitato: "); Serial.println(_hx711_abilitato ? "SI" : "NO");
  Serial.println();
}

// ============================================================================
// CALIBRAZIONE DA SERVER
// ============================================================================
void calibrate_hx711(float calibration_factor, long offset) {
  if (!_hx711_inizializzato) {
    Serial.println("  ! HX711 non inizializzato, calibrazione ignorata");
    return;
  }

  if (calibration_factor != 0) {
    _hx711_calibration_factor = calibration_factor;
    LoadCell.setCalFactor(_hx711_calibration_factor);
  }

  // Con HX711_ADC, l'offset viene gestito internamente dalla tara
  // Se necessario, si puo' fare una nuova tara:
  // LoadCell.tareNoDelay();

  Serial.println("  --- HX711 Calibrazione applicata ---");
  Serial.print("    Calibration factor: "); Serial.println(_hx711_calibration_factor);
}

// ============================================================================
// TARA MANUALE
// ============================================================================
bool tare_hx711() {
  if (!_hx711_inizializzato) return false;

  Serial.println("  Esecuzione tara...");
  LoadCell.tareNoDelay();

  // Attendi completamento tara
  unsigned long timeout = millis() + 3000;
  while (!LoadCell.getTareStatus() && millis() < timeout) {
    LoadCell.update();
    delay(10);
  }

  if (LoadCell.getTareStatus()) {
    _hx711_tarato = true;
    Serial.println("  + Tara completata");
    return true;
  } else {
    Serial.println("  ! Tara fallita (timeout)");
    return false;
  }
}

// ============================================================================
// LETTURA PESO
// ============================================================================
RisultatoValidazione read_weight_hx711() {
  RisultatoValidazione risultato;
  risultato.valido = false;
  risultato.valorePulito = 0.0f;
  risultato.timestamp = millis();

  // 1. CHECK ABILITATO
  if (!_hx711_abilitato) {
    risultato.codiceErrore = ERR_SENSOR_OFFLINE;
    strcpy(risultato.messaggioErrore, "[HX711] Sensore disabilitato");
    return risultato;
  }

  // 2. CHECK INIZIALIZZAZIONE
  if (!_hx711_inizializzato) {
    risultato.codiceErrore = ERR_SENSOR_NOT_READY;
    strcpy(risultato.messaggioErrore, "[HX711] Sensore non inizializzato");
    return risultato;
  }

  // 3. CHECK TARATURA
  if (!_hx711_tarato) {
    risultato.codiceErrore = ERR_PS_CALIBRATION_MISSING;
    strcpy(risultato.messaggioErrore, "[HX711] Tara non eseguita");
    return risultato;
  }

  // 4. AGGIORNA E LEGGI
  // Aggiorna il convertitore (necessario per HX711_ADC)
  unsigned long startTime = millis();
  boolean newData = false;

  while (millis() - startTime < 500) {  // Timeout 500ms
    if (LoadCell.update()) {
      newData = true;
      break;
    }
    delay(10);
  }

  if (!newData) {
    risultato.codiceErrore = ERR_SENSOR_TIMEOUT;
    strcpy(risultato.messaggioErrore, "[HX711] Timeout lettura");
    return risultato;
  }

  float peso_kg = LoadCell.getData();

  // 5. CHECK VALIDITA' LETTURA
  if (isnan(peso_kg) || isinf(peso_kg)) {
    risultato.codiceErrore = ERR_PS_CONVERSION_FAILED;
    strcpy(risultato.messaggioErrore, "[HX711] Lettura non valida");
    return risultato;
  }

  // 6. VALIDAZIONE CON SENSORVALIDATION
  risultato = validaDatoSensore(
    peso_kg,
    risultato.timestamp,
    true,  // sensoreReady
    _configValidazionePeso
  );

  // 7. CONTROLLO SOGLIE
  if (risultato.valido) {
    verificaSoglie(peso_kg, _hx711_sogliaMin, _hx711_sogliaMax, "HX711");
  }

  _last_read_time = millis();
  return risultato;
}

// ============================================================================
// GETTERS
// ============================================================================
unsigned long get_intervallo_hx711() { return _hx711_intervallo; }
bool is_abilitato_hx711() { return _hx711_abilitato; }
bool is_tarato_hx711() { return _hx711_tarato; }
bool is_inizializzato_hx711() { return _hx711_inizializzato; }
float get_calibration_factor_hx711() { return _hx711_calibration_factor; }
float get_last_weight_hx711() { return LoadCell.getData(); }
