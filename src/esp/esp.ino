// ============================================================================
// ESP32-CAM - Main Controller Sistema Monitoraggio Arnia
// ============================================================================

#include <WiFi.h>
#include <WiFiMulti.h>
#include <esp_task_wdt.h>
#include "SensorValidation.h"
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>
uint32_t last_ota_time = 0;

// ============================================================================
// CONFIGURAZIONE WI-FI MULTIPLI
// ============================================================================
const char* WIFI_NETWORKS[][2] = {
  {"Gruppo4Network", "Networks"},
  {"WIFI_LABORATORIO", "password_lab"},
  {"WIFI_SCUOLA", "password_scuola"}
};
const int NUM_NETWORKS = 3;

// Configurazione server REST
const char* REST_URL = "https://dbarniadigitale-0abe.restdb.io/rest/";
const char* REST_KEY = "1fe80bc79adb61240bff1f82429757190c888";
const int REST_TIMEOUT = 10000;

// Configurazione watchdog
#define WDT_TIMEOUT_SEC 30

WiFiMulti wifiMulti;

// ============================================================================
// VARIABILI DEVICE
// ============================================================================
char deviceMacAddress[18] = "";

// ============================================================================
// DICHIARAZIONE FUNZIONI DATA MANAGER (connection_manager.ino)
// ============================================================================
extern void init_data_manager(ServerConfig* config);
extern bool is_data_manager_ready();
extern ConfigData fetch_sensor_config(const char* macAddress);
extern bool save_sensor_data(SensorData* data);
extern bool save_value(const char* sensorId, float valore, unsigned long timestamp);
extern bool send_notification(NotificationData* notification);
extern bool notify(const char* macAddress, const char* tipoSensore,
                   float valoreRiferimento, unsigned long timestamp,
                   const char* messaggio, int livello);

// ============================================================================
// CONFIGURAZIONE SENSORI (memorizzata per accesso ai sensorId)
// ============================================================================
static ConfigData configSensori;

// ============================================================================
// DICHIARAZIONE FUNZIONI DEI SENSORI
// ============================================================================
extern void setup_ds18b20();
extern void init_ds18b20(SensorConfig* config);
extern RisultatoValidazione read_temperature_ds18b20();
extern unsigned long get_intervallo_ds18b20();
extern bool is_abilitato_ds18b20();

extern void setup_sht21();
extern void init_humidity_sht21(SensorConfig* config);
extern void init_temperature_sht21(SensorConfig* config);
extern RisultatoValidazione read_humidity_sht21();
extern RisultatoValidazione read_temperature_sht21();
extern unsigned long get_intervallo_humidity_sht21();
extern unsigned long get_intervallo_temperature_sht21();
extern bool is_abilitato_humidity_sht21();
extern bool is_abilitato_temperature_sht21();

extern void setup_hx711();
extern void init_hx711(SensorConfig* config);
extern void calibrate_hx711(float calibration_factor, long offset);
extern RisultatoValidazione read_weight_hx711();
extern unsigned long get_intervallo_hx711();
extern bool is_abilitato_hx711();

// ============================================================================
// TIMING
// ============================================================================
unsigned long ultimoCheck_ds18b20 = 0;
unsigned long ultimoCheck_sht21_humidity = 0;
unsigned long ultimoCheck_sht21_temperature = 0;
unsigned long ultimoCheck_hx711 = 0;
unsigned long ultimoCheckWiFi = 0;

// ============================================================================
// STATO SISTEMA
// ============================================================================
bool wifiConnesso = false;
bool configCaricata = false;

// ============================================================================
// GESTIONE RISULTATI VALIDAZIONE
// ============================================================================
void gestisciRisultatoSensore(RisultatoValidazione risultato) {
  if (risultato.valido) {
    Serial.println("  ✓ Lettura valida");
    
    if (risultato.codiceErrore == ALERT_THRESHOLD_HIGH) {
      Serial.println("  ⚠ ALERT: Valore sopra soglia massima");
    } else if (risultato.codiceErrore == ALERT_THRESHOLD_LOW) {
      Serial.println("  ⚠ ALERT: Valore sotto soglia minima");
    }
  } else {
    Serial.print("  ✗ Lettura NON valida - Codice errore: ");
    Serial.println(risultato.codiceErrore);
    
    switch(risultato.codiceErrore) {
      case ERROR_SENSOR_NOT_FOUND:
        Serial.println("    Sensore non trovato");
        break;
      case ERROR_READ_FAILED:
        Serial. println("    Lettura fallita");
        break;
      case ERROR_OUT_OF_RANGE:
        Serial.println("    Valore fuori range");
        break;
      case ERROR_SPIKE_DETECTED:
        Serial. println("    Spike rilevato");
        break;
      default:
        Serial.println("    Errore sconosciuto");
    }
  }
}

// ============================================================================
// GESTIONE OVERFLOW MILLIS
// ============================================================================
bool intervalloTrascorso(unsigned long &ultimoCheck, unsigned long intervallo) {
  unsigned long adesso = millis();
  
  if ((adesso - ultimoCheck) >= intervallo) {
    ultimoCheck = adesso;
    return true;
  }
  return false;
}

// ============================================================================
// OTTIENI TIMESTAMP UNIX
// ============================================================================
unsigned long getUnixTimestamp() {
  time_t now = time(nullptr);
  if (now < 1000000000) {
    return millis() / 1000;
  }
  return (unsigned long)now;
}

// ============================================================================
// GESTIONE WI-FI CON WiFiMulti
// ============================================================================
void initWiFi() {
  uint8_t mac[6];
  WiFi. macAddress(mac);
  snprintf(deviceMacAddress, sizeof(deviceMacAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Serial.print("  MAC Address: ");
  Serial.println(deviceMacAddress);

  WiFi.mode(WIFI_STA);
  
  Serial.println("\n  Reti Wi-Fi configurate:");
  for (int i = 0; i < NUM_NETWORKS; i++) {
    wifiMulti.addAP(WIFI_NETWORKS[i][0], WIFI_NETWORKS[i][1]);
    Serial.print("    - ");
    Serial.println(WIFI_NETWORKS[i][0]);
  }
  Serial.println();
}

bool connectWiFi() {
  Serial.println("  Connessione alla rete migliore disponibile...");
  
  uint8_t tentativi = 0;
  while (wifiMulti.run() != WL_CONNECTED && tentativi < 20) {
    delay(500);
    Serial.print(".");
    tentativi++;
    esp_task_wdt_reset();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnesso = true;
    Serial.println(" OK");
    Serial.print("    Connesso a: ");
    Serial.println(WiFi. SSID());
    Serial.print("    IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("    RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    return true;
  } else {
    wifiConnesso = false;
    Serial.println(" FALLITO");
    Serial.println("    Nessuna rete disponibile");
    return false;
  }
}

bool isWiFiConnected() {
  wifiConnesso = (WiFi.status() == WL_CONNECTED);
  return wifiConnesso;
}

void checkWiFiConnection() {
  if (intervalloTrascorso(ultimoCheckWiFi, 10000)) {
    
    if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println("\n!  WiFi disconnesso, riconnessione automatica...");
      wifiConnesso = false;
    } else {
      if (!wifiConnesso) {
        Serial.println("\n✓ WiFi riconnesso!");
        Serial.print("  Connesso a: ");
        Serial.println(WiFi.SSID());
        Serial.print("  IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("  RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm\n");
      }
      wifiConnesso = true;
    }
  }
}

// ============================================================================
// CARICAMENTO CONFIG DA SERVER
// ============================================================================
void caricaConfigDaServer() {
  Serial.println("\n--- CARICAMENTO CONFIGURAZIONE ---\n");

  if (!isWiFiConnected()) {
    Serial.println("  ! Wi-Fi non connesso, uso config default");
  }

  configSensori = fetch_sensor_config(deviceMacAddress);

  if (configSensori.success) {
    Serial.println("  + Config ricevuta dal server");
  } else {
    Serial.println("  ! Config non disponibile, uso valori default");
  }

  init_ds18b20(&configSensori.ds18b20);
  init_humidity_sht21(&configSensori.sht21_humidity);
  init_temperature_sht21(&configSensori.sht21_temperature);
  init_hx711(&configSensori.hx711);

  calibrate_hx711(configSensori.calibrationFactor, configSensori.calibrationOffset);

  configCaricata = true;
  Serial.println("\n--- CONFIGURAZIONE APPLICATA ---\n");
}

// ============================================================================
// INVIO DATO SENSORE
// ============================================================================
// tipoSensore: "ds18b20", "sht21_humidity", "sht21_temperature", "hx711"
// ============================================================================
void inviaDatoSensore(const char* tipoSensore, RisultatoValidazione* risultato) {

  if (!isWiFiConnected()) {
    Serial.println("  ! Wi-Fi non connesso, dato non inviato");
    return;
  }

  // Ottieni il sensorId dalla configurazione in base al tipo
  const char* sensorId = "";

  if (strcmp(tipoSensore, "ds18b20") == 0) {
    sensorId = configSensori.ds18b20.sensorId;
  } else if (strcmp(tipoSensore, "sht21_humidity") == 0) {
    sensorId = configSensori.sht21_humidity.sensorId;
  } else if (strcmp(tipoSensore, "sht21_temperature") == 0) {
    sensorId = configSensori.sht21_temperature.sensorId;
  } else if (strcmp(tipoSensore, "hx711") == 0) {
    sensorId = configSensori.hx711.sensorId;
  }

  if (strlen(sensorId) == 0) {
    Serial.print("  ! SensorId non configurato per: ");
    Serial.println(tipoSensore);
    return;
  }

  unsigned long timestamp = getUnixTimestamp();

  bool salvato = save_value(sensorId, risultato->valorePulito, timestamp);

  if (!salvato) {
    Serial.println("  ! Errore salvataggio dato");
  }
}

// ============================================================================
// SETUP PRINCIPALE
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n");
  Serial.println("========================================");
  Serial.println("  SISTEMA MONITORAGGIO ARNIA - PCTO");
  Serial.println("  Main Controller v2.4");
  Serial.println("========================================");
  Serial.println();

  // Inizializza watchdog - CORRETTO per ESP32 IDF v5. 5
  Serial.println("Inizializzazione Watchdog Timer...");
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT_SEC * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);
  Serial.println("  + Watchdog attivo\n");

  // FASE 1: Inizializzazione Wi-Fi
  Serial.println("FASE 1: INIZIALIZZAZIONE WI-FI\n");
  initWiFi();

  if (connectWiFi()) {
    Serial.println("  + Wi-Fi connesso\n");
  } else {
    Serial.println("  !  Wi-Fi non disponibile, modalita' offline\n");
  }

  // Password can be set with plain text (will be hashed internally)
  // The authentication uses PBKDF2-HMAC-SHA256 with 10,000 iterations
  ArduinoOTA.setPassword("!hJp^%RmYj7fQNmUjcd%");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      if (millis() - last_ota_time > 500) {
        Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
        last_ota_time = millis();
      }
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

  ArduinoOTA.begin();

  // FASE 2: Inizializzazione Data Manager
  Serial.println("FASE 2: INIZIALIZZAZIONE DATA MANAGER\n");
  ServerConfig serverConfig;
  
  snprintf(serverConfig.baseUrl, sizeof(serverConfig.baseUrl), "%s", REST_URL);
  snprintf(serverConfig.apiKey, sizeof(serverConfig.apiKey), "%s", REST_KEY);
  serverConfig.timeout = REST_TIMEOUT;
  
  init_data_manager(&serverConfig);

  // FASE 3: Inizializzazione hardware sensori
  Serial.println("\nFASE 3: INIZIALIZZAZIONE HARDWARE\n");
  setup_ds18b20();
  setup_sht21();
  setup_hx711();
  Serial.println("+ Tutti i sensori inizializzati\n");

  // FASE 4: Caricamento configurazione dal server
  Serial.println("FASE 4: CARICAMENTO CONFIGURAZIONE");
  caricaConfigDaServer();

  Serial.println("Intervalli di campionamento:");
  Serial.print("  - DS18B20:     "); Serial.print(get_intervallo_ds18b20() / 1000); Serial.println(" sec");
  Serial.print("  - SHT21 Hum:   "); Serial.print(get_intervallo_humidity_sht21() / 1000); Serial.println(" sec");
  Serial.print("  - SHT21 Temp:   "); Serial.print(get_intervallo_temperature_sht21() / 1000); Serial.println(" sec");
  Serial.print("  - HX711:       "); Serial.print(get_intervallo_hx711() / 1000); Serial.println(" sec");
  Serial.println();

  Serial.println("========================================");
  Serial.println("  -> AVVIO MONITORAGGIO.. .");
  Serial.println("========================================\n");
}

// ============================================================================
// LOOP PRINCIPALE
// ============================================================================
void loop() {
  ArduinoOTA.handle();
  
  esp_task_wdt_reset();
  
  checkWiFiConnection();

  // DS18B20
  if (is_abilitato_ds18b20() && intervalloTrascorso(ultimoCheck_ds18b20, get_intervallo_ds18b20())) {
    Serial.println("\n[DS18B20] LETTURA TEMPERATURA INTERNA");
    RisultatoValidazione risultato = read_temperature_ds18b20();
    gestisciRisultatoSensore(risultato);

    if (risultato.valido) {
      Serial.print("  -> Valore: "); Serial.print(risultato.valorePulito); Serial.println(" C");
      inviaDatoSensore("ds18b20", &risultato);
    }
    Serial.println("---\n");
  }

  // SHT21 - UMIDITA
  if (is_abilitato_humidity_sht21() && intervalloTrascorso(ultimoCheck_sht21_humidity, get_intervallo_humidity_sht21())) {
    Serial.println("\n[SHT21] LETTURA UMIDITA");
    RisultatoValidazione risultato = read_humidity_sht21();
    gestisciRisultatoSensore(risultato);

    if (risultato.valido) {
      Serial.print("  -> Valore: "); Serial.print(risultato.valorePulito); Serial.println(" %");
      inviaDatoSensore("sht21_humidity", &risultato);
    }
    Serial.println("---\n");
  }

  // SHT21 - TEMPERATURA AMBIENTE
  if (is_abilitato_temperature_sht21() && intervalloTrascorso(ultimoCheck_sht21_temperature, get_intervallo_temperature_sht21())) {
    Serial.println("\n[SHT21] LETTURA TEMPERATURA AMBIENTE");
    RisultatoValidazione risultato = read_temperature_sht21();
    gestisciRisultatoSensore(risultato);

    if (risultato.valido) {
      Serial.print("  -> Valore: "); Serial.print(risultato.valorePulito); Serial.println(" C");
      inviaDatoSensore("sht21_temperature", &risultato);
    }
    Serial.println("---\n");
  }

  // HX711 - PESO
  if (is_abilitato_hx711() && intervalloTrascorso(ultimoCheck_hx711, get_intervallo_hx711())) {
    Serial.println("\n[HX711] LETTURA PESO");
    RisultatoValidazione risultato = read_weight_hx711();
    gestisciRisultatoSensore(risultato);

    if (risultato.valido) {
      Serial.print("  -> Valore: "); Serial.print(risultato.valorePulito); Serial.println(" kg");
      inviaDatoSensore("hx711", &risultato);
    }
    Serial.println("---\n");
  }

  delay(100);
}

// ============================================================================
// UTILITY
// ============================================================================
void stampaStatistiche() {
  Serial.println("\n--- STATISTICHE ---");
  Serial.print("MAC:  "); Serial.println(deviceMacAddress);
  Serial.print("Uptime: "); Serial.print(millis() / 1000); Serial.println(" sec");
  Serial.print("Free RAM: "); Serial.println(ESP.getFreeHeap());
  Serial.print("Wi-Fi:  "); Serial.println(isWiFiConnected() ? "Connesso" : "Disconnesso");
  if (isWiFiConnected()) {
    Serial.print("SSID: "); Serial.println(WiFi.SSID());
    Serial.print("RSSI: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  }
  Serial.println();
}
