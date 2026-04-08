#ifndef SENSOR_VALIDATION_H
#define SENSOR_VALIDATION_H

#include <Arduino.h>

// ============================================================================
// ENUM ERRORI COMUNI (CATEGORIA 1XXX)
// ============================================================================
enum ErroreComune {
  // 9XXX - Status OK
  STATUS_OK = 9000,
  STATUS_OK_WITH_WARNING = 9001,
  
  // 10XX - Integrità del dato
  ERR_DATA_NULL = 1001,
  ERR_DATA_INVALID_NUMBER = 1002,
  ERR_DATA_NEGATIVE_NOT_ALLOWED = 1003,
  ERR_DATA_FORMAT_INVALID = 1004,
  
  // 11XX - Range
  ERR_MEASURE_OUT_OF_RANGE = 1101,
  ERR_MEASURE_BELOW_MIN = 1102,
  ERR_MEASURE_ABOVE_MAX = 1103,
  
  // 12XX - Sensore/Hardware
  ERR_SENSOR_NOT_READY = 1201,
  ERR_SENSOR_TIMEOUT = 1202,
  ERR_SENSOR_OFFLINE = 1203,
  ERR_SENSOR_DISCONNECTED = 1204,
  
  // 13XX - Qualità segnale
  ERR_MEASURE_UNSTABLE = 1301,
  ERR_MEASURE_NOISE = 1302,
  ERR_MEASURE_OUTLIER = 1303,
  ERR_SENSOR_WARMUP = 1304,
  
  // 14XX - Timestamp
  ERR_TIMESTAMP_INVALID = 1401,
  ERR_TIMESTAMP_FUTURE = 1402,
  ERR_TIMESTAMP_DUPLICATE = 1403,
  
  // 15XX - Connettività
  ERR_NETWORK_OFFLINE = 1501,
  ERR_NETWORK_TIMEOUT = 1502,
  ERR_DB_UNREACHABLE = 1503,
  ERR_UPLOAD_FAILED = 1504,
  ERR_UPLOAD_RETRY_EXHAUSTED = 1505,
  ERR_UPLOAD_DUPLICATE = 1506
};

// ============================================================================
// ENUM ALERT (CATEGORIA 2XXX)
// ============================================================================
enum Alert {
  ALERT_THRESHOLD_LOW = 2001,
  ALERT_THRESHOLD_HIGH = 2002,
  ALERT_THRESHOLD_CRITICAL_LOW = 2003,
  ALERT_THRESHOLD_CRITICAL_HIGH = 2004,
  ALERT_DATA_QUALITY_LOW = 2101,
  ALERT_OUTLIER_DETECTED = 2102,
  ALERT_DRIFT_DETECTED = 2103
};

// ============================================================================
// ENUM ERRORI SPECIFICI PESO (CATEGORIA 3XXX)
// ============================================================================
enum ErrorePeso {
  ERR_PS_NONE = 0,
  ERR_PS_ADC_SATURATION_LOW = 3001,
  ERR_PS_ADC_SATURATION_HIGH = 3002,
  ERR_PS_ADC_SATURATION = 3003,
  ERR_PS_ADC_READ_FAILED = 3004,
  ERR_PS_CALIBRATION_MISSING = 3101,
  ERR_PS_CALIBRATION_INVALID = 3102,
  ERR_PS_CALIBRATION_EXPIRED = 3103,
  ERR_PS_TARE_FAILED = 3104,
  ERR_PS_CONVERSION_FAILED = 3201,
  ERR_PS_CONVERSION_OVERFLOW = 3202,
  ERR_PS_CONVERSION_UNDERFLOW = 3203,
  ERR_PS_LOADCELL_FAULT = 3301,
  ERR_PS_LOADCELL_OVERLOAD = 3302
};

// ============================================================================
// ENUM ERRORI SPECIFICI UMIDITÀ (CATEGORIA 4XXX)
// ============================================================================
enum ErroreUmidita {
  ERR_HUM_NONE = 0,
  ERR_HUM_SENSOR_FAULT = 4001,
  ERR_HUM_I2C_COMM_FAILED = 4002,
  ERR_HUM_CHECKSUM_ERROR = 4003,
  ERR_HUM_NOT_FLOAT = 4101,
  ERR_HUM_PRECISION_LOSS = 4102,
  ALERT_HUM_CONDENSATION_RISK = 4201,
  ALERT_HUM_TOO_DRY = 4202
};

// ============================================================================
// ENUM ERRORI SPECIFICI TEMPERATURA (CATEGORIA 5XXX)
// ============================================================================
enum ErroreTemperatura {
  ERR_TEMP_NONE = 0,
  ERR_TEMP_SENSOR_DISCONNECTED = 5001,
  ERR_TEMP_CRC_ERROR = 5002,
  ERR_TEMP_ONEWIRE_FAILED = 5003,
  ERR_TEMP_SAMPLING_INTERVAL = 5101,
  ERR_TEMP_CONVERSION_TIMEOUT = 5102,
  ERR_TEMP_RESOLUTION_ERROR = 5103,
  ERR_TEMP_STORAGE_FULL = 5201,
  ERR_TEMP_SYNC_FAILED = 5202,
  ALERT_TEMP_FREEZE_RISK = 5301,
  ALERT_TEMP_OVERHEAT = 5302
};

// ============================================================================
// ENUM LEGACY (per compatibilità con esp. ino)
// ============================================================================
enum ErrorLegacy {
  ERROR_NONE = 0,
  ERROR_SENSOR_NOT_FOUND = 1204,      // Mappa a ERR_SENSOR_DISCONNECTED
  ERROR_READ_FAILED = 1202,           // Mappa a ERR_SENSOR_TIMEOUT
  ERROR_OUT_OF_RANGE = 1101,          // Mappa a ERR_MEASURE_OUT_OF_RANGE
  ERROR_SPIKE_DETECTED = 1303         // Mappa a ERR_MEASURE_OUTLIER
};

// ============================================================================
// STRUTTURE
// ============================================================================

struct ConfigValidazioneSensore {
  float rangeMin;
  float rangeMax;
  bool permettiNegativi;
  bool richiedeTimestamp;
  float valoreDefault;
  const char* nomeSensore;
};

struct SensorConfig {
  float sogliaMin;          // sea_min
  float sogliaMax;          // sea_max
  unsigned long intervallo; // sea_intervallo_ms
  bool abilitato;           // sea_stato
  char sensorId[32];        // _id del sensore su MongoDB (sea)
  float deltaMinimo;        // sea_delta: variazione minima per invio anticipato (0 = disabilitato)
  // Campi specifici cella di carico HX711 (ignorati dagli altri sensori)
  float calFactor;          // sea_cal_factor: fattore di calibrazione HX711_ADC
  long  tareOffset;         // sea_tare_offset: offset ADC grezzo per zero persistente
};

struct RisultatoValidazione {
  bool valido;
  int codiceErrore;
  float valorePulito;
  char messaggioErrore[80];
  unsigned long timestamp;  // AGGIUNTO:  mancava nel tuo codice originale
};

struct ServerConfig {
  char baseUrl[128];
  char apiKey[64];
  int timeout;
};

// Valore sensore da salvare
struct SensorData {
  char macAddress[18];
  char tipoSensore[32];
  char idSensore[32];
  float valore;
  char unita[8];
  unsigned long timestamp;
  int codiceStato;
  bool alert;
  char alertTipo[16];
};

// Notifica/Log
struct NotificationData {
  char macAddress[18];
  char tipoSensore[32];
  float valoreRiferimento;
  unsigned long timestamp;
  char messaggio[128];
  int livello;  // 0=INFO, 1=WARNING, 2=ERROR, 3=CRITICAL
};

// Risposta configurazione
struct ConfigData {
  bool success;
  SensorConfig ds18b20;
  SensorConfig sht21_humidity;
  SensorConfig sht21_temperature;
  SensorConfig hx711;
  float calibrationFactor;
  long calibrationOffset;
};

// ============================================================================
// FUNZIONI HELPER
// ============================================================================

inline const char* getErroreDescrizione(int codiceErrore) {
  switch(codiceErrore) {
    case 9000: return "OK";
    case 9001: return "OK con warning";
    case 1001: return "Dato nullo/mancante";
    case 1002: return "Dato NaN o Infinito";
    case 1003: return "Dato negativo non ammesso";
    case 1004: return "Formato dato errato";
    case 1101: return "Misura fuori range";
    case 1102: return "Misura sotto minimo";
    case 1103: return "Misura sopra massimo";
    case 1201: return "Sensore non pronto";
    case 1202: return "Timeout sensore";
    case 1203: return "Sensore offline";
    case 1204: return "Sensore disconnesso";
    case 1301: return "Misura instabile";
    case 1302: return "Rumore eccessivo";
    case 1303: return "Outlier rilevato";
    case 1304: return "Sensore in warm-up";
    case 1401: return "Timestamp non valido";
    case 1402: return "Timestamp futuro";
    case 1403: return "Timestamp duplicato";
    case 1501: return "Rete offline";
    case 1502: return "Timeout rete";
    case 1503: return "DB non raggiungibile";
    case 1504: return "Upload fallito";
    case 1505: return "Retry esauriti";
    case 1506: return "Misura duplicata";
    case 2001: return "Alert soglia bassa";
    case 2002: return "Alert soglia alta";
    case 2003: return "Alert critico basso";
    case 2004: return "Alert critico alto";
    case 2101: return "Qualità dato bassa";
    case 2102: return "Outlier rilevato";
    case 2103: return "Deriva sensore";
    case 3001: return "ADC saturo basso";
    case 3002: return "ADC saturo alto";
    case 3003: return "ADC saturo";
    case 3004: return "Lettura ADC fallita";
    case 3101: return "Calibrazione mancante";
    case 3102: return "Calibrazione non valida";
    case 3103: return "Calibrazione scaduta";
    case 3104: return "Tara fallita";
    case 3201: return "Conversione kg fallita";
    case 3202: return "Overflow conversione";
    case 3203: return "Underflow conversione";
    case 3301: return "Guasto load cell";
    case 3302: return "Sovraccarico load cell";
    case 4001: return "Errore hardware sensore";
    case 4002: return "I2C fallito";
    case 4003: return "Errore checksum";
    case 4101: return "Dato non float";
    case 4102: return "Perdita precisione";
    case 4201: return "Rischio condensa";
    case 4202: return "Aria troppo secca";
    case 5001: return "DS18B20 disconnesso";
    case 5002: return "Errore CRC";
    case 5003: return "OneWire fallito";
    case 5101: return "Intervallo non rispettato";
    case 5102: return "Timeout conversione";
    case 5103: return "Errore risoluzione";
    case 5201: return "Storage pieno";
    case 5202: return "Sync fallita";
    case 5301: return "Rischio congelamento";
    case 5302: return "Surriscaldamento";
    default: return "Errore sconosciuto";
  }
}

inline const char* getErroreCategoria(int codiceErrore) {
  int categoria = codiceErrore / 1000;
  switch(categoria) {
    case 1: return "ERRORE_COMUNE";
    case 2: return "ALERT";
    case 3: return "ERRORE_PESO";
    case 4: return "ERRORE_UMIDITA";
    case 5: return "ERRORE_TEMPERATURA";
    case 9: return "STATUS";
    default: return "SCONOSCIUTO";
  }
}

// ============================================================================
// FUNZIONE VALIDAZIONE CENTRALE
// ============================================================================

inline RisultatoValidazione validaDatoSensore(
  float valoreGrezzo,
  unsigned long timestamp,
  bool sensoreReady,
  ConfigValidazioneSensore config
) {
  RisultatoValidazione risultato;
  risultato.valido = true;
  risultato.codiceErrore = STATUS_OK;
  risultato.valorePulito = valoreGrezzo;
  risultato.timestamp = timestamp;
  strcpy(risultato.messaggioErrore, "OK");
  
  if (!sensoreReady) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_SENSOR_NOT_READY;
    risultato.valorePulito = config.valoreDefault;
    snprintf(risultato.messaggioErrore, 80, "[%s][%d] %s", 
             config.nomeSensore, ERR_SENSOR_NOT_READY,
             getErroreDescrizione(ERR_SENSOR_NOT_READY));
    return risultato;
  }
  
  if (isnan(valoreGrezzo)) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_DATA_INVALID_NUMBER;
    risultato.valorePulito = config.valoreDefault;
    snprintf(risultato.messaggioErrore, 80, "[%s][%d] %s (NaN)", 
             config.nomeSensore, ERR_DATA_INVALID_NUMBER,
             getErroreDescrizione(ERR_DATA_INVALID_NUMBER));
    return risultato;
  }
  
  if (isinf(valoreGrezzo)) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_DATA_INVALID_NUMBER;
    risultato.valorePulito = config.valoreDefault;
    snprintf(risultato.messaggioErrore, 80, "[%s][%d] %s (Inf)", 
             config.nomeSensore, ERR_DATA_INVALID_NUMBER,
             getErroreDescrizione(ERR_DATA_INVALID_NUMBER));
    return risultato;
  }
  
  if (! config.permettiNegativi && valoreGrezzo < 0.0f) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_DATA_NEGATIVE_NOT_ALLOWED;
    risultato.valorePulito = config.valoreDefault;
    snprintf(risultato.messaggioErrore, 80, "[%s][%d] %s:  %. 2f", 
             config.nomeSensore, ERR_DATA_NEGATIVE_NOT_ALLOWED,
             getErroreDescrizione(ERR_DATA_NEGATIVE_NOT_ALLOWED), valoreGrezzo);
    return risultato;
  }
  
  if (valoreGrezzo < config.rangeMin || valoreGrezzo > config.rangeMax) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_MEASURE_OUT_OF_RANGE;
    risultato.valorePulito = config.valoreDefault;
    snprintf(risultato.messaggioErrore, 80, "[%s][%d] %s: %.2f [%.2f-%.2f]", 
             config.nomeSensore, ERR_MEASURE_OUT_OF_RANGE,
             getErroreDescrizione(ERR_MEASURE_OUT_OF_RANGE),
             valoreGrezzo, config.rangeMin, config.rangeMax);
    return risultato;
  }
  
  if (config.richiedeTimestamp && (timestamp == 0 || timestamp == ULONG_MAX)) {
    risultato.valido = false;
    risultato.codiceErrore = ERR_TIMESTAMP_INVALID;
    risultato.valorePulito = valoreGrezzo;
    snprintf(risultato.messaggioErrore, 80, "[%s][%d] %s", 
             config.nomeSensore, ERR_TIMESTAMP_INVALID,
             getErroreDescrizione(ERR_TIMESTAMP_INVALID));
    return risultato;
  }
  
  risultato.valido = true;
  risultato.codiceErrore = STATUS_OK;
  risultato.valorePulito = valoreGrezzo;
  snprintf(risultato.messaggioErrore, 80, "[%s][%d] Valore valido:  %.2f", 
           config.nomeSensore, STATUS_OK, valoreGrezzo);
  
  return risultato;
}

inline void gestisciRisultatoValidazione(RisultatoValidazione risultato) {
  if (risultato.codiceErrore != STATUS_OK) {
    Serial.print("❌ ");
    Serial.print(getErroreCategoria(risultato.codiceErrore));
    Serial.print(" ");
  } else {
    Serial.print("✓ ");
  }
  Serial.println(risultato.messaggioErrore);
}

inline int verificaSoglie(float valore, float sogliaMin, float sogliaMax, const char* nomeSensore) {
  if (valore > sogliaMax) {
    Serial.print("[ALERT][");
    Serial.print(ALERT_THRESHOLD_HIGH);
    Serial.print("] ");
    Serial.print(nomeSensore);
    Serial.print(": ");
    Serial.print(valore);
    Serial.print(" > ");
    Serial.println(sogliaMax);
    return ALERT_THRESHOLD_HIGH;
  }
  
  if (valore < sogliaMin) {
    Serial.print("[ALERT][");
    Serial.print(ALERT_THRESHOLD_LOW);
    Serial.print("] ");
    Serial.print(nomeSensore);
    Serial.print(": ");
    Serial.print(valore);
    Serial.print(" < ");
    Serial.println(sogliaMin);
    return ALERT_THRESHOLD_LOW;
  }
  
  return STATUS_OK;
}

#endif // SENSOR_VALIDATION_H
