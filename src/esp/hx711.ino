// ============================================================================
// HX711 - Sensore Peso Arnia (HX711_ADC library)
// ============================================================================
// Libreria: HX711_ADC by Olav Kallhovd v1.2.12
// https://github.com/olkal/HX711_ADC
// ============================================================================

#include <HX711_ADC.h>
#include <esp_task_wdt.h>
#include "SensorValidation.h"

// ============================================================================
// CONFIGURAZIONE HARDWARE
// ============================================================================
// Profilo ESP32-CAM senza microSD:
// - DOUT su GPIO2: in esecuzione normale il pin e' ignorato dal boot da flash,
//   ma in modalita' download puo' richiedere di scollegare HX711 durante il flash.
// - SCK su GPIO4: pin valido ma condiviso con il LED flash onboard.
const int HX711_DOUT_PIN = 2;
const int HX711_SCK_PIN = 4;

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

// Campionamento adattivo
static float _hx711_delta = 0.05f;          // kg — soglia variazione per invio anticipato
static float _hx711_lastSentValue = NAN;
static unsigned long _hx711_lastSentTime = 0;

// Parametri calibrazione persistente (caricati dal server in init_hx711)
// - _hx711_calibration_factor: pendenza della cella (setCalFactor)
// - _hx711_tare_offset:        zero ADC grezzo (setTareOffset)
// Valori di fallback usati solo finché la config non arriva dal server.
static float _hx711_calibration_factor = 696.0f;
static long  _hx711_tare_offset        = 0L;
static bool  _hx711_calibrazione_valida = false;

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

  esp_task_wdt_reset();
  LoadCell.begin();
  esp_task_wdt_reset();

  // Avvio SENZA tara automatica: il nostro zero e' persistente, viene
  // caricato dal server in init_hx711(). Questo garantisce letture
  // assolute e confrontabili fra riavvii, OTA e reset watchdog.
  // LoadCell.start() blocca per stabilizingTime ms internamente; e'
  // essenziale resettare il WDT prima e dopo per evitare che scatti.
  unsigned long stabilizingTime = 2000;
  boolean doTare = false;  // <-- IMPORTANTE: tara disabilitata

  Serial.println("  Stabilizzazione HX711 (2s, senza tara)...");
  esp_task_wdt_reset();   // reset prima del blocco da 2s
  LoadCell.start(stabilizingTime, doTare);
  esp_task_wdt_reset();   // reset subito dopo il blocco

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("  ! TIMEOUT: Controlla cablaggio HX711");
    Serial.println("    DOUT -> GPIO 2");
    Serial.println("    SCK  -> GPIO 4");
    Serial.println("    VCC  -> 3.3V (consigliato con ESP32-CAM)");
    Serial.println("    GND  -> GND");
    _hx711_inizializzato = false;
    return;
  }

  // Applica il fattore di calibrazione di fallback finche' init_hx711
  // non sovrascrivera' con il valore dal server.
  LoadCell.setCalFactor(_hx711_calibration_factor);

  _hx711_inizializzato = true;
  _hx711_tarato = false;          // Non siamo ancora tarati: attendiamo init_hx711
  _hx711_calibrazione_valida = false;

  Serial.println("  + HX711 inizializzato (in attesa di calibrazione dal server)");
  Serial.print("    Fallback cal factor: ");
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
    _hx711_sogliaMin  = config->sogliaMin;
    _hx711_sogliaMax  = config->sogliaMax;
    _hx711_intervallo = config->intervallo;
    _hx711_abilitato  = config->abilitato;
    _hx711_delta      = config->deltaMinimo;

    // Applica calibrazione persistente dal server.
    if (config->calFactor > 0.0f && !isnan(config->calFactor) && !isinf(config->calFactor)) {
      _hx711_calibration_factor = config->calFactor;
      LoadCell.setCalFactor(_hx711_calibration_factor);
    }
    // Il tare offset e' valido solo se diverso da 0 (0 = "tara mai eseguita").
    // L'apicoltore deve eseguire la tara iniziale dalla dashboard con
    // arnia vuota sulla bilancia.
    if (config->tareOffset != 0L) {
      _hx711_tare_offset = config->tareOffset;
      LoadCell.setTareOffset(_hx711_tare_offset);
      _hx711_tarato = true;
      _hx711_calibrazione_valida = true;
    } else {
      _hx711_tarato = false;
      _hx711_calibrazione_valida = false;
      Serial.println("  ! ATTENZIONE: tare_offset=0 -> tara non ancora eseguita");
      Serial.println("    Letture peso non affidabili finche' non esegui la tara");
      Serial.println("    dalla dashboard (http://<ip_esp>/hx711/tare) con");
      Serial.println("    arnia VUOTA sulla bilancia.");
    }
  }
  _hx711_lastSentTime  = 0;   // forza primo campione
  _hx711_lastSentValue = NAN;

  Serial.println("  --- HX711 configurato ---");
  Serial.print("    Soglia MIN: "); Serial.print(_hx711_sogliaMin);  Serial.println(" kg");
  Serial.print("    Soglia MAX: "); Serial.print(_hx711_sogliaMax);  Serial.println(" kg");
  Serial.print("    Intervallo: "); Serial.print(_hx711_intervallo / 1000); Serial.println(" sec");
  Serial.print("    Delta min:  "); Serial.print(_hx711_delta);       Serial.println(" kg");
  Serial.print("    Cal factor: "); Serial.println(_hx711_calibration_factor);
  Serial.print("    Tare offset:"); Serial.println(_hx711_tare_offset);
  Serial.print("    Calibrato:  "); Serial.println(_hx711_calibrazione_valida ? "SI" : "NO");
  Serial.print("    Abilitato:  "); Serial.println(_hx711_abilitato ? "SI" : "NO");
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
// Esegue una nuova tara e aggiorna _hx711_tare_offset con il valore ADC
// grezzo rilevato. Dopo averla eseguita, il chiamante deve persistere il
// nuovo offset sul server (save_hx711_calibration) altrimenti al prossimo
// boot verra' ricaricato il vecchio valore.
//
// Implementazione robusta:
//   1. Pre-warming: raccolta di sample "a vuoto" per 1s per assicurarsi
//      che il convertitore HX711 stia producendo dati stabili.
//   2. Timeout interno della libreria esteso a 8s (default 4s non basta
//      quando il sample rate e' 10 SPS e servono ~16 sample).
//   3. tareNoDelay() + polling manuale con WDT reset ogni iterazione,
//      fino al completamento o timeout esteso.
//   4. Diagnostica dettagliata su Serial per debug.
// ============================================================================
bool tare_hx711() {
  if (!_hx711_inizializzato) {
    Serial.println("  ! tare_hx711: sensore non inizializzato");
    return false;
  }

  Serial.println("  [TARA] Avvio procedura tara manuale");

  // --- STEP 1: pre-warming, aspetta che arrivino sample validi ---
  Serial.println("  [TARA] Pre-warming convertitore (max 2s)...");
  esp_task_wdt_reset();
  int sampleCount = 0;
  unsigned long warmupEnd = millis() + 2000;
  while (millis() < warmupEnd && sampleCount < 10) {
    if (LoadCell.update()) {
      sampleCount++;
    }
    esp_task_wdt_reset();
    delay(5);
  }
  Serial.print("  [TARA] Sample raccolti in warmup: ");
  Serial.println(sampleCount);
  if (sampleCount == 0) {
    Serial.println("  ! TARA FALLITA: nessun sample dal convertitore.");
    Serial.println("    Possibili cause:");
    Serial.println("      - HX711 non cablato (DOUT/SCK/VCC/GND)");
    Serial.println("      - Alimentazione insufficiente (usa 3.3V stabile)");
    Serial.println("      - Cella di carico non collegata al modulo HX711");
    return false;
  }

  // --- STEP 2: disabilita il timeout interno della libreria ---
  // (La libreria chiamerebbe comunque tareTimeoutFlag dentro a tare())
  LoadCell.disableTareTimeout();

  // --- STEP 3: tara BLOCCANTE ---
  // tare() internamente chiama update() in loop + yield(), accumula
  // SAMPLES+IGN_HIGH+IGN_LOW letture grezze e calcola la media. E' la
  // stessa identica sequenza usata da LoadCell.start(_, true) in setup
  // (che storicamente ha sempre funzionato).
  // A 10 SPS servono tipicamente 1.5-2.5 secondi. Resettiamo il WDT
  // prima e dopo, e yield() interno alla libreria dovrebbe bastare a
  // mantenere vivi gli altri task.
  Serial.println("  [TARA] Esecuzione tare() bloccante (max ~3s)...");
  esp_task_wdt_reset();
  unsigned long tareStart = millis();
  LoadCell.tare();
  unsigned long tareDuration = millis() - tareStart;
  esp_task_wdt_reset();

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.print("  ! TARA FALLITA: tareTimeoutFlag set dopo ");
    Serial.print(tareDuration);
    Serial.println(" ms");
    Serial.println("    La libreria non ha ricevuto abbastanza sample.");
    Serial.println("    Controlla cablaggio HX711 (DOUT/SCK) e alimentazione.");
    return false;
  }

  _hx711_tare_offset = LoadCell.getTareOffset();
  _hx711_tarato = true;
  _hx711_calibrazione_valida = true;
  Serial.print("  + TARA COMPLETATA in ");
  Serial.print(tareDuration);
  Serial.println(" ms");
  Serial.print("    Nuovo tare_offset (ADC grezzo): ");
  Serial.println(_hx711_tare_offset);
  return true;
}

// Getter usati da esp.ino per persistere la calibrazione sul server
long  get_tare_offset_hx711()  { return _hx711_tare_offset; }
float get_cal_factor_hx711()   { return _hx711_calibration_factor; }
bool  is_calibrato_hx711()     { return _hx711_calibrazione_valida; }

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
// CAMPIONAMENTO ADATTIVO - should_send / mark_sent
// ============================================================================
bool should_send_hx711(float nuovoValore) {
  if (_hx711_lastSentTime == 0) return true;
  if (millis() - _hx711_lastSentTime >= _hx711_intervallo) return true;
  if (_hx711_delta > 0.0f && !isnan(_hx711_lastSentValue)) {
    if (fabs(nuovoValore - _hx711_lastSentValue) >= _hx711_delta) return true;
  }
  return false;
}

void mark_sent_hx711(float valoreInviato) {
  _hx711_lastSentValue = valoreInviato;
  _hx711_lastSentTime  = millis();
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
