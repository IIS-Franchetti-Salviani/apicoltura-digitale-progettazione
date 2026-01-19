/*******************************************************
 *  ALL-IN-ONE SENSORS FILE (ESP32-CAM)
 *  - DS18B20 (Temp Interna)
 *  - SHT21 / HTU21D (Temp + Umidita)
 *  - HX711 (Peso) - opzionale
 *  - SensorValidation integrato
 *  
 *  VERSIONE CON OUTPUT DETTAGLIATO SU SERIAL MONITOR
 *******************************************************/

#include <Arduino.h>

// ===== DS18B20 =====
#include <OneWire.h>
#include <DallasTemperature.h>

// ===== SHT21 =====
#include <Wire.h>
#include "Adafruit_HTU21DF.h"

// ===== HX711 (opzionale) =====
// #include "HX711.h"  // Decommenta se usi il sensore peso

// =====================================================
// CONFIGURAZIONE INTERVALLI (modificabili)
// =====================================================
#define INTERVALLO_TEST_RAPIDO    10000   // 10 secondi - per test iniziale
#define INTERVALLO_NORMALE        360000  // 6 minuti - per uso normale

// Cambia qui per test rapidi o uso normale
#define MODALITA_TEST  true  // true = lettura ogni 10 secondi, false = ogni 6 minuti

// =====================================================
// SENSOR VALIDATION (integrato)
// =====================================================

// Codici minimi (per rendere il file autosufficiente)
static const int STATUS_OK                     = 9000;
static const int ERR_SENSOR_OFFLINE            = 1200;
static const int ERR_SENSOR_NOT_READY          = 1201;
static const int ERR_DATA_INVALID_NUMBER       = 1002;
static const int ERR_DATA_NEGATIVE_NOT_ALLOWED = 1003;
static const int ERR_MEASURE_OUT_OF_RANGE      = 1101;
static const int ERR_TIMESTAMP_INVALID         = 1401;

static const int ALERT_THRESHOLD_LOW           = 2001;
static const int ALERT_THRESHOLD_HIGH          = 2002;

static const int ERR_PS_CALIBRATION_MISSING    = 3001;
static const int ERR_PS_CALIBRATION_INVALID    = 3002;
static const int ERR_PS_CONVERSION_FAILED      = 3003;

struct ConfigValidazioneSensore {
  float rangeMin;
  float rangeMax;
  bool permettiNegativi;
  bool richiedeTimestamp;
  float valoreDefault;
  const char* nomeSensore;
};

struct SensorConfig {
  float sogliaMin;
  float sogliaMax;
  unsigned long intervallo;
  bool abilitato;
};

struct RisultatoValidazione {
  bool valido;
  int codiceErrore;
  float valorePulito;
  char messaggioErrore[80];
};

static const char* getErroreDescrizione(int codiceErrore) {
  switch (codiceErrore) {
    case STATUS_OK: return "OK";
    case ERR_SENSOR_OFFLINE: return "Sensore disabilitato";
    case ERR_SENSOR_NOT_READY: return "Sensore non pronto/non inizializzato";
    case ERR_DATA_INVALID_NUMBER: return "Dato NaN o infinito";
    case ERR_DATA_NEGATIVE_NOT_ALLOWED: return "Dato negativo non ammesso";
    case ERR_MEASURE_OUT_OF_RANGE: return "Misura fuori range";
    case ERR_TIMESTAMP_INVALID: return "Timestamp non valido";
    case ERR_PS_CALIBRATION_MISSING: return "Calibrazione mancante";
    case ERR_PS_CALIBRATION_INVALID: return "Calibrazione non valida";
    case ERR_PS_CONVERSION_FAILED: return "Conversione fallita";
    default: return "Errore sconosciuto";
  }
}

static const char* getErroreCategoria(int codiceErrore) {
  int categoria = codiceErrore / 1000;
  switch (categoria) {
    case 1: return "ERRORE_COMUNE";
    case 2: return "ALERT";
    case 3: return "ERRORE_PESO";
    case 9: return "STATUS";
    default: return "SCONOSCIUTO";
  }
}

static RisultatoValidazione validaDatoSensore(
  float valoreGrezzo,
  unsigned long timestamp,
  bool sensoreReady,
  ConfigValidazioneSensore config
) {
  RisultatoValidazione r;
  r.valido = true;
  r.codiceErrore = STATUS_OK;
  r.valorePulito = valoreGrezzo;
  strcpy(r.messaggioErrore, "OK");

  if (!sensoreReady) {
    r.valido = false;
    r.codiceErrore = ERR_SENSOR_NOT_READY;
    r.valorePulito = config.valoreDefault;
    snprintf(r.messaggioErrore, sizeof(r.messaggioErrore),
             "[%s][%d] %s",
             config.nomeSensore, r.codiceErrore, getErroreDescrizione(r.codiceErrore));
    return r;
  }

  if (isnan(valoreGrezzo) || isinf(valoreGrezzo)) {
    r.valido = false;
    r.codiceErrore = ERR_DATA_INVALID_NUMBER;
    r.valorePulito = config.valoreDefault;
    snprintf(r.messaggioErrore, sizeof(r.messaggioErrore),
             "[%s][%d] %s",
             config.nomeSensore, r.codiceErrore, getErroreDescrizione(r.codiceErrore));
    return r;
  }

  if (!config.permettiNegativi && valoreGrezzo < 0.0f) {
    r.valido = false;
    r.codiceErrore = ERR_DATA_NEGATIVE_NOT_ALLOWED;
    r.valorePulito = config.valoreDefault;
    snprintf(r.messaggioErrore, sizeof(r.messaggioErrore),
             "[%s][%d] %s: %.2f",
             config.nomeSensore, r.codiceErrore, getErroreDescrizione(r.codiceErrore), valoreGrezzo);
    return r;
  }

  if (valoreGrezzo < config.rangeMin || valoreGrezzo > config.rangeMax) {
    r.valido = false;
    r.codiceErrore = ERR_MEASURE_OUT_OF_RANGE;
    r.valorePulito = config.valoreDefault;
    snprintf(r.messaggioErrore, sizeof(r.messaggioErrore),
             "[%s][%d] %s: %.2f [%.2f-%.2f]",
             config.nomeSensore, r.codiceErrore, getErroreDescrizione(r.codiceErrore),
             valoreGrezzo, config.rangeMin, config.rangeMax);
    return r;
  }

  if (config.richiedeTimestamp && (timestamp == 0 || timestamp == ULONG_MAX)) {
    r.valido = false;
    r.codiceErrore = ERR_TIMESTAMP_INVALID;
    r.valorePulito = valoreGrezzo;
    snprintf(r.messaggioErrore, sizeof(r.messaggioErrore),
             "[%s][%d] %s",
             config.nomeSensore, r.codiceErrore, getErroreDescrizione(r.codiceErrore));
    return r;
  }

  r.valido = true;
  r.codiceErrore = STATUS_OK;
  r.valorePulito = valoreGrezzo;
  snprintf(r.messaggioErrore, sizeof(r.messaggioErrore),
           "[%s] Lettura OK",
           config.nomeSensore);
  return r;
}

// Funzione di output migliorata con visualizzazione valori
static void gestisciRisultatoValidazione(RisultatoValidazione r, const char* unitaMisura = "") {
  if (r.codiceErrore != STATUS_OK) {
    Serial.print("   X ERRORE: ");
    Serial.println(r.messaggioErrore);
  } else {
    Serial.print("   OK ");
    Serial.print(r.messaggioErrore);
    Serial.print(" -> ");
    Serial.print(r.valorePulito, 2);
    Serial.print(" ");
    Serial.println(unitaMisura);
  }
}

static int verificaSoglie(float valore, float sogliaMin, float sogliaMax, const char* nomeSensore) {
  if (valore > sogliaMax) {
    Serial.print("   ! ALERT: "); 
    Serial.print(nomeSensore); 
    Serial.print(" SOPRA SOGLIA: ");
    Serial.print(valore, 2); 
    Serial.print(" > "); 
    Serial.println(sogliaMax, 2);
    return ALERT_THRESHOLD_HIGH;
  }
  if (valore < sogliaMin) {
    Serial.print("   ! ALERT: "); 
    Serial.print(nomeSensore); 
    Serial.print(" SOTTO SOGLIA: ");
    Serial.print(valore, 2); 
    Serial.print(" < "); 
    Serial.println(sogliaMin, 2);
    return ALERT_THRESHOLD_LOW;
  }
  return STATUS_OK;
}

// =====================================================
// PIN / HW CONFIG per ESP32-CAM + BREADBOARD
// =====================================================
// DS18B20 (OneWire)
static const int PIN_ONEWIRE_DS18B20 = 2;

// SHT21 / HTU21D (I2C)
static const int PIN_I2C_SDA = 14;
static const int PIN_I2C_SCL = 15;

// HX711 (opzionale - sensore peso)
static const int PIN_HX711_DOUT = 13;
static const int PIN_HX711_SCK  = 12;

// =====================================================
// DS18B20 - Temperatura interna arnia
// =====================================================
static OneWire ds18_oneWire(PIN_ONEWIRE_DS18B20);
static DallasTemperature ds18_sensors(&ds18_oneWire);
static DeviceAddress ds18_insideThermometer;

static float _ds18_sogliaMin = 30.0f;
static float _ds18_sogliaMax = 37.0f;
static unsigned long _ds18_intervallo = MODALITA_TEST ? INTERVALLO_TEST_RAPIDO : INTERVALLO_NORMALE;
static bool _ds18_abilitato = true;
static bool _ds18_inizializzato = false;
static int _ds18_contatore = 0;
static float _ds18_ultima_lettura = 0.0f;

static ConfigValidazioneSensore _cfgVal_ds18 = {
  -40.0f,    // rangeMin
  85.0f,     // rangeMax
  true,      // permettiNegativi
  true,      // richiedeTimestamp
  25.0f,     // valoreDefault
  "DS18B20"  // nomeSensore
};

static void setup_ds18b20() {
  Serial.println(F("\n-> Inizializzazione DS18B20 (GPIO 2)..."));
  ds18_sensors.begin();

  if (ds18_sensors.getDeviceCount() == 0) {
    Serial.println(F("  X Nessun DS18B20 trovato"));
    Serial.println(F("  ! Verifica: resistenza 4.7kOhm tra DATA e 3.3V?"));
    _ds18_inizializzato = false;
    return;
  }

  Serial.print(F("  + Trovati ")); 
  Serial.print(ds18_sensors.getDeviceCount()); 
  Serial.println(F(" sensori DS18B20"));

  if (ds18_sensors.getAddress(ds18_insideThermometer, 0)) {
    ds18_sensors.setResolution(ds18_insideThermometer, 12);
    ds18_sensors.setWaitForConversion(true);
    _ds18_inizializzato = true;
    Serial.println(F("  + DS18B20 configurato e pronto"));
  } else {
    Serial.println(F("  X DS18B20: indirizzo non letto"));
    _ds18_inizializzato = false;
  }
}

static RisultatoValidazione read_temperature_ds18b20() {
  RisultatoValidazione r{};
  r.valido = false;
  r.codiceErrore = STATUS_OK;
  r.valorePulito = _cfgVal_ds18.valoreDefault;
  strcpy(r.messaggioErrore, "[DS18B20] INIT");

  if (!_ds18_abilitato) {
    r.codiceErrore = ERR_SENSOR_OFFLINE;
    r.valorePulito = _cfgVal_ds18.valoreDefault;
    strcpy(r.messaggioErrore, "[DS18B20] Sensore disabilitato");
    return r;
  }
  if (!_ds18_inizializzato) {
    r.codiceErrore = ERR_SENSOR_NOT_READY;
    r.valorePulito = _cfgVal_ds18.valoreDefault;
    strcpy(r.messaggioErrore, "[DS18B20] Sensore non inizializzato");
    return r;
  }

  ds18_sensors.requestTemperatures();
  float tempC = ds18_sensors.getTempC(ds18_insideThermometer);
  bool sensoreReady = (tempC != DEVICE_DISCONNECTED_C);

  r = validaDatoSensore(tempC, millis(), sensoreReady, _cfgVal_ds18);

  if (r.valido) {
    _ds18_ultima_lettura = r.valorePulito;
    verificaSoglie(r.valorePulito, _ds18_sogliaMin, _ds18_sogliaMax, "Temp. Interna");
    _ds18_contatore++;
  }
  return r;
}

// =====================================================
// SHT21 / HTU21D - Umidita + Temperatura
// =====================================================
static Adafruit_HTU21DF sht21 = Adafruit_HTU21DF();

static float _sht_hum_sogliaMin = 40.0f;
static float _sht_hum_sogliaMax = 70.0f;
static unsigned long _sht_hum_intervallo = MODALITA_TEST ? INTERVALLO_TEST_RAPIDO : INTERVALLO_NORMALE;
static bool _sht_hum_abilitato = true;
static int _sht_hum_contatore = 0;
static float _sht_hum_ultima_lettura = 0.0f;

static float _sht_temp_sogliaMin = 20.0f;
static float _sht_temp_sogliaMax = 40.0f;
static unsigned long _sht_temp_intervallo = MODALITA_TEST ? INTERVALLO_TEST_RAPIDO : INTERVALLO_NORMALE;
static bool _sht_temp_abilitato = true;
static int _sht_temp_contatore = 0;
static float _sht_temp_ultima_lettura = 0.0f;

static bool _sht_inizializzato = false;

static ConfigValidazioneSensore _cfgVal_sht_hum = {
  0.0f,         // rangeMin
  100.0f,       // rangeMax
  false,        // permettiNegativi
  true,         // richiedeTimestamp
  50.0f,        // valoreDefault
  "SHT21_HUM"   // nomeSensore
};

static ConfigValidazioneSensore _cfgVal_sht_temp = {
  -40.0f,       // rangeMin
  125.0f,       // rangeMax
  true,         // permettiNegativi
  true,         // richiedeTimestamp
  25.0f,        // valoreDefault
  "SHT21_TEMP"  // nomeSensore
};

static void setup_sht21() {
  Serial.println(F("\n-> Inizializzazione SHT21 (I2C: SDA=GPIO14, SCL=GPIO15)..."));
  
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  delay(100);

  if (!sht21.begin()) {
    Serial.println(F("  X SHT21 non trovato!"));
    Serial.println(F("  ! Verifica collegamenti I2C:"));
    Serial.println(F("    - SDA -> GPIO 14"));
    Serial.println(F("    - SCL -> GPIO 15"));
    Serial.println(F("    - VCC -> 3.3V"));
    Serial.println(F("    - GND -> GND"));
    _sht_inizializzato = false;
  } else {
    Serial.println(F("  + SHT21 rilevato e pronto"));
    _sht_inizializzato = true;
  }
}

static RisultatoValidazione read_humidity_sht21() {
  if (!_sht_hum_abilitato) {
    RisultatoValidazione r{};
    r.valido = false;
    r.codiceErrore = ERR_SENSOR_OFFLINE;
    r.valorePulito = _cfgVal_sht_hum.valoreDefault;
    strcpy(r.messaggioErrore, "[SHT21_HUM] Sensore disabilitato");
    return r;
  }
  if (!_sht_inizializzato) {
    RisultatoValidazione r{};
    r.valido = false;
    r.codiceErrore = ERR_SENSOR_NOT_READY;
    r.valorePulito = _cfgVal_sht_hum.valoreDefault;
    strcpy(r.messaggioErrore, "[SHT21_HUM] Sensore non inizializzato");
    return r;
  }

  float hum = sht21.readHumidity();
  bool sensoreReady = !isnan(hum);

  RisultatoValidazione r = validaDatoSensore(hum, millis(), sensoreReady, _cfgVal_sht_hum);

  if (r.valido) {
    _sht_hum_ultima_lettura = r.valorePulito;
    verificaSoglie(r.valorePulito, _sht_hum_sogliaMin, _sht_hum_sogliaMax, "Umidita");
    _sht_hum_contatore++;
  }
  return r;
}

static RisultatoValidazione read_temperature_sht21() {
  if (!_sht_temp_abilitato) {
    RisultatoValidazione r{};
    r.valido = false;
    r.codiceErrore = ERR_SENSOR_OFFLINE;
    r.valorePulito = _cfgVal_sht_temp.valoreDefault;
    strcpy(r.messaggioErrore, "[SHT21_TEMP] Sensore disabilitato");
    return r;
  }
  if (!_sht_inizializzato) {
    RisultatoValidazione r{};
    r.valido = false;
    r.codiceErrore = ERR_SENSOR_NOT_READY;
    r.valorePulito = _cfgVal_sht_temp.valoreDefault;
    strcpy(r.messaggioErrore, "[SHT21_TEMP] Sensore non inizializzato");
    return r;
  }

  float t = sht21.readTemperature();
  bool sensoreReady = !isnan(t);

  RisultatoValidazione r = validaDatoSensore(t, millis(), sensoreReady, _cfgVal_sht_temp);

  if (r.valido) {
    _sht_temp_ultima_lettura = r.valorePulito;
    verificaSoglie(r.valorePulito, _sht_temp_sogliaMin, _sht_temp_sogliaMax, "Temp. Esterna");
    _sht_temp_contatore++;
  }
  return r;
}

// =====================================================
// UTILITY: Print status dettagliato
// =====================================================
static void printStatus() {
  Serial.println(F("\n===================================================="));
  Serial.println(F("          RIEPILOGO SENSORI ESP32-CAM              "));
  Serial.println(F("===================================================="));
  
  // DS18B20
  Serial.print(F(" DS18B20 (GPIO 2) | "));
  if (_ds18_inizializzato) {
    Serial.print(F("ONLINE  | "));
    Serial.print(_ds18_ultima_lettura, 1);
    Serial.print(F("C"));
  } else {
    Serial.print(F("OFFLINE | --"));
  }
  Serial.print(F(" | Letture: "));
  Serial.println(_ds18_contatore);
  
  // SHT21 Temperatura
  Serial.print(F(" SHT21 Temp       | "));
  if (_sht_inizializzato) {
    Serial.print(F("ONLINE  | "));
    Serial.print(_sht_temp_ultima_lettura, 1);
    Serial.print(F("C"));
  } else {
    Serial.print(F("OFFLINE | --"));
  }
  Serial.print(F(" | Letture: "));
  Serial.println(_sht_temp_contatore);
  
  // SHT21 Umidità
  Serial.print(F(" SHT21 Umidita    | "));
  if (_sht_inizializzato) {
    Serial.print(F("ONLINE  | "));
    Serial.print(_sht_hum_ultima_lettura, 1);
    Serial.print(F("% "));
  } else {
    Serial.print(F("OFFLINE | --"));
  }
  Serial.print(F(" | Letture: "));
  Serial.println(_sht_hum_contatore);
  
  Serial.println(F("===================================================="));
  Serial.print(F(" Uptime: "));
  unsigned long secondi = millis() / 1000;
  unsigned long minuti = secondi / 60;
  unsigned long ore = minuti / 60;
  Serial.print(ore);
  Serial.print(F("h "));
  Serial.print(minuti % 60);
  Serial.print(F("m "));
  Serial.print(secondi % 60);
  Serial.println(F("s"));
  Serial.println(F("====================================================\n"));
}

// =====================================================
// MAIN (setup / loop)
// =====================================================
static unsigned long last_ds18 = 0;
static unsigned long last_sht_hum  = 0;
static unsigned long last_sht_temp = 0;
static unsigned long last_status = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Tempo per aprire Serial Monitor
  
  Serial.println(F("\n\n"));
  Serial.println(F("======================================================="));
  Serial.println(F("     SISTEMA MONITORAGGIO SENSORI ARNIA v2.1          "));
  Serial.println(F("              ESP32-CAM + Breadboard                   "));
  Serial.println(F("======================================================="));

  Serial.println(F("\nConfigurazione Hardware:"));
  Serial.println(F("   - DS18B20 -> GPIO 2 (con pull-up 4.7kOhm a 3.3V)"));
  Serial.println(F("   - SHT21 SDA -> GPIO 14"));
  Serial.println(F("   - SHT21 SCL -> GPIO 15"));
  Serial.println(F("   - Alimentazione: 3.3V e GND distribuiti su breadboard"));
  
  Serial.print(F("\nModalita: "));
  if (MODALITA_TEST) {
    Serial.println(F("TEST RAPIDO (lettura ogni 10 secondi)"));
  } else {
    Serial.println(F("NORMALE (lettura ogni 6 minuti)"));
  }
  
  Serial.println(F("\nInizializzazione sensori in corso...\n"));
  Serial.println(F("======================================================="));

  // Setup sensori
  setup_ds18b20();
  setup_sht21();

  Serial.println(F("\n======================================================="));
  Serial.println(F("Inizializzazione completata!"));
  Serial.println(F("=======================================================\n"));
  
  if (MODALITA_TEST) {
    Serial.println(F("INIZIO TEST - Letture ogni 10 secondi\n"));
  } else {
    Serial.println(F("ACQUISIZIONE AVVIATA - Letture ogni 6 minuti\n"));
  }
  
  // Forza prima lettura immediata
  last_ds18 = millis() - _ds18_intervallo;
  last_sht_hum = millis() - _sht_hum_intervallo;
  last_sht_temp = millis() - _sht_temp_intervallo;
  last_status = millis() - 60000; // Forza primo status dopo 1 minuto
}

void loop() {
  unsigned long now = millis();

  // DS18B20 - Temperatura interna
  if (_ds18_abilitato && (now - last_ds18 >= _ds18_intervallo)) {
    last_ds18 = now;
    Serial.println(F("-------------------------------------------"));
    Serial.println(F("  TEMPERATURA INTERNA (DS18B20)"));
    Serial.println(F("-------------------------------------------"));
    RisultatoValidazione r = read_temperature_ds18b20();
    gestisciRisultatoValidazione(r, "C");
    Serial.println();
  }

  // SHT21 - Temperatura esterna
  if (_sht_temp_abilitato && (now - last_sht_temp >= _sht_temp_intervallo)) {
    last_sht_temp = now;
    Serial.println(F("-------------------------------------------"));
    Serial.println(F("  TEMPERATURA ESTERNA (SHT21)"));
    Serial.println(F("-------------------------------------------"));
    RisultatoValidazione r = read_temperature_sht21();
    gestisciRisultatoValidazione(r, "C");
    Serial.println();
  }

  // SHT21 - Umidità
  if (_sht_hum_abilitato && (now - last_sht_hum >= _sht_hum_intervallo)) {
    last_sht_hum = now;
    Serial.println(F("-------------------------------------------"));
    Serial.println(F("  UMIDITA RELATIVA (SHT21)"));
    Serial.println(F("-------------------------------------------"));
    RisultatoValidazione r = read_humidity_sht21();
    gestisciRisultatoValidazione(r, "%");
    Serial.println();
  }

  // Status periodico (ogni 60 secondi)
  if (now - last_status >= 60000) {
    last_status = now;
    printStatus();
  }

  delay(100);  // Loop ogni 100ms
}
