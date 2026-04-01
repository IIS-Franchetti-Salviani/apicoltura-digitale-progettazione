// ESP32-CAM - Main Controller Sistema Monitoraggio Arnia
// ============================================================================

// adding url https://jihulab.com/esp-mirror/espressif/arduino-esp32.git
// board esp32 Expressif 3.3.5


#include <WiFi.h>
#include <WiFiMulti.h>
#include <esp_task_wdt.h>
#include "SensorValidation.h"
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <Update.h>
#include <esp_mac.h>
#include <time.h>
#include <math.h>
uint32_t last_ota_time = 0;

// ============================================================================
// CONFIGURAZIONE WI-FI MULTIPLI
// ============================================================================
const char* WIFI_NETWORKS[][2] = {
  {"Gruppo4Network", "Networks"},
  {"ASUS_RiceWLan", "pippoplutopaperinominnie"},
  {"didattica", "FdWt101099stdZ%("}
};
const int NUM_NETWORKS = 3;

// Configurazione server REST
const char* REST_URL = "https://www.flip-flop.it/apicoltura-digitale/rest/";
const char* REST_KEY = "ijy31qysljvd99d1pdelbyfemsje29nz";
const int REST_TIMEOUT = 10000;
const char* OTA_AUTH_PASSWORD = "!hJp^%RmYj7fQNmUjcd%";
const char* WEB_FW_UPLOAD_USER = "admin";

// Configurazione watchdog
#define WDT_TIMEOUT_SEC 30

WiFiMulti wifiMulti;
WebServer deviceWebServer(80);

// ============================================================================
// VARIABILI DEVICE
// ============================================================================
char deviceMacAddress[18] = "";
// GPIO4 e' assegnato all'HX711 come clock. Disabilitiamo il LED di stato
// onboard per evitare impulsi spurii sul sensore peso.
int ledPin = -1;

// ============================================================================
// DICHIARAZIONE FUNZIONI DATA MANAGER (connection_manager.ino)
// ============================================================================
extern void init_data_manager(ServerConfig* config);
extern bool is_data_manager_ready();
extern ConfigData fetch_sensor_config(const char* macAddress);
extern bool save_sensor_data(SensorData* data);
extern bool save_value(const char* sensorId, float valore, unsigned long timestamp);
extern bool save_value_with_context(const char* sensorId, const char* macAddress, const char* tipoSensore,
                                    float valore, unsigned long timestamp, int codiceStato);
extern bool send_sensor_runtime_status(const char* macAddress, const char* tipoSensore,
                                       const char* sensorId, bool abilitato,
                                       const char* evento, const char* causaCodice,
                                       const char* causaDettaglio, int codiceStato,
                                       float valore, unsigned long timestamp);
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

// ========================================================================
// DASHBOARD WEB - STATO ULTIME LETTURE
// ========================================================================
struct SensorRuntimeSnapshot {
  bool disponibile;
  bool valido;
  bool inviatoServer;
  bool abilitato;
  float valore;
  int codiceStato;
  unsigned long unixTs;
  char nota[96];
};

SensorRuntimeSnapshot snapshotDs18b20 = {false, false, false, false, NAN, 0, 0, "Nessuna lettura"};
SensorRuntimeSnapshot snapshotSht21Hum = {false, false, false, false, NAN, 0, 0, "Nessuna lettura"};
SensorRuntimeSnapshot snapshotSht21Temp = {false, false, false, false, NAN, 0, 0, "Nessuna lettura"};
SensorRuntimeSnapshot snapshotHx711 = {false, false, false, false, NAN, 0, 0, "Nessuna lettura"};

// ========================================================================
// GESTIONE EPOCH / NTP
// ========================================================================
const unsigned long MIN_VALID_UNIX_TS = 1704067200UL; // 2024-01-01T00:00:00Z
const unsigned long NTP_RESYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL; // 6h
unsigned long lastKnownUnixEpoch = 0;
unsigned long lastKnownUnixMillis = 0;
unsigned long lastNtpSyncAttemptMs = 0;

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

    switch (risultato.codiceErrore) {
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

bool isEpochValido(time_t ts) {
  return ts >= (time_t)MIN_VALID_UNIX_TS;
}

void aggiornaEpochCache(unsigned long epoch) {
  lastKnownUnixEpoch = epoch;
  lastKnownUnixMillis = millis();
}

bool sincronizzaTempoNtp(bool force = false) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  unsigned long adessoMs = millis();
  if (!force && (adessoMs - lastNtpSyncAttemptMs) < NTP_RESYNC_INTERVAL_MS) {
    return false;
  }
  lastNtpSyncAttemptMs = adessoMs;

  configTime(0, 0, "pool.ntp.org", "time.cloudflare.com", "time.google.com");

  const int maxTentativi = 20;
  for (int i = 0; i < maxTentativi; i++) {
    time_t now = time(nullptr);
    if (isEpochValido(now)) {
      aggiornaEpochCache((unsigned long)now);
      Serial.print("  + NTP sincronizzato: ");
      Serial.println((unsigned long)now);
      return true;
    }
    delay(250);
    esp_task_wdt_reset();
  }

  Serial.println("  ! NTP non sincronizzato (timeout)");
  return false;
}

bool isValidSeaId(const char* sensorId) {
  if (sensorId == nullptr || sensorId[0] == '\0') {
    return false;
  }

  for (size_t i = 0; sensorId[i] != '\0'; i++) {
    if (!isDigit(sensorId[i])) {
      return false;
    }
  }
  return true;
}

// ============================================================================
// OTTIENI TIMESTAMP UNIX
// ============================================================================
unsigned long getUnixTimestamp() {
  time_t now = time(nullptr);
  if (isEpochValido(now)) {
    aggiornaEpochCache((unsigned long)now);
    return (unsigned long)now;
  }

  // Prova una sincronizzazione NTP al bisogno.
  sincronizzaTempoNtp(false);
  now = time(nullptr);
  if (isEpochValido(now)) {
    aggiornaEpochCache((unsigned long)now);
    return (unsigned long)now;
  }

  // Fallback su cache locale, se disponibile.
  if (lastKnownUnixEpoch > 0) {
    unsigned long elapsedSec = (millis() - lastKnownUnixMillis) / 1000UL;
    return lastKnownUnixEpoch + elapsedSec;
  }

  // Nessuna base temporale affidabile disponibile.
  return 0;
}

const char* getSensorIdByTipo(const char* tipoSensore) {
  if (strcmp(tipoSensore, "ds18b20") == 0) {
    return configSensori.ds18b20.sensorId;
  }
  if (strcmp(tipoSensore, "sht21_humidity") == 0) {
    return configSensori.sht21_humidity.sensorId;
  }
  if (strcmp(tipoSensore, "sht21_temperature") == 0) {
    return configSensori.sht21_temperature.sensorId;
  }
  if (strcmp(tipoSensore, "hx711") == 0) {
    return configSensori.hx711.sensorId;
  }
  return "";
}

bool isSensorAbilitatoByTipo(const char* tipoSensore) {
  if (strcmp(tipoSensore, "ds18b20") == 0) {
    return configSensori.ds18b20.abilitato;
  }
  if (strcmp(tipoSensore, "sht21_humidity") == 0) {
    return configSensori.sht21_humidity.abilitato;
  }
  if (strcmp(tipoSensore, "sht21_temperature") == 0) {
    return configSensori.sht21_temperature.abilitato;
  }
  if (strcmp(tipoSensore, "hx711") == 0) {
    return configSensori.hx711.abilitato;
  }
  return false;
}

SensorRuntimeSnapshot* getSnapshotByTipo(const char* tipoSensore) {
  if (strcmp(tipoSensore, "ds18b20") == 0) {
    return &snapshotDs18b20;
  }
  if (strcmp(tipoSensore, "sht21_humidity") == 0) {
    return &snapshotSht21Hum;
  }
  if (strcmp(tipoSensore, "sht21_temperature") == 0) {
    return &snapshotSht21Temp;
  }
  if (strcmp(tipoSensore, "hx711") == 0) {
    return &snapshotHx711;
  }
  return nullptr;
}

void aggiornaSnapshotSensore(const char* tipoSensore, const RisultatoValidazione* risultato,
                             bool inviatoServer, const char* nota) {
  SensorRuntimeSnapshot* snapshot = getSnapshotByTipo(tipoSensore);
  if (snapshot == nullptr || risultato == nullptr) {
    return;
  }

  snapshot->disponibile = true;
  snapshot->valido = risultato->valido;
  snapshot->inviatoServer = inviatoServer;
  snapshot->abilitato = isSensorAbilitatoByTipo(tipoSensore);
  snapshot->valore = risultato->valorePulito;
  snapshot->codiceStato = risultato->codiceErrore;
  snapshot->unixTs = getUnixTimestamp();

  if (nota == nullptr) {
    nota = "";
  }
  snprintf(snapshot->nota, sizeof(snapshot->nota), "%s", nota);
}

String formatValueForDashboard(float value, uint8_t decimals) {
  if (isnan(value) || isinf(value)) {
    return "N/A";
  }
  return String(value, decimals);
}

String formatTsForDashboard(unsigned long ts) {
  if (ts == 0) {
    return "N/A";
  }

  time_t raw = (time_t)ts;
  struct tm* tmUtc = gmtime(&raw);
  if (tmUtc == nullptr) {
    return "N/A";
  }

  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S UTC", tmUtc);
  return String(buffer);
}

String escapeHtml(const String& input) {
  String out = input;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  out.replace("'", "&#39;");
  return out;
}

String buildSensorCard(const char* titolo, const char* unita, const SensorRuntimeSnapshot& snapshot) {
  String stato = "MAI LETTO";
  if (!snapshot.abilitato) {
    stato = "DISABILITATO";
  } else if (!snapshot.disponibile) {
    stato = "IN ATTESA";
  } else if (!snapshot.valido) {
    stato = "ERRORE LETTURA";
  } else if (!snapshot.inviatoServer) {
    stato = "NON INVIATO";
  } else {
    stato = "OK";
  }

  String html;
  html.reserve(650);
  html += "<article class='card'>";
  html += "<h2>";
  html += titolo;
  html += "</h2>";
  html += "<div class='value'>";
  html += formatValueForDashboard(snapshot.valore, 2);
  html += " <span>";
  html += unita;
  html += "</span></div>";
  html += "<p><strong>Stato:</strong> ";
  html += stato;
  html += "</p>";
  html += "<p><strong>Codice:</strong> ";
  html += String(snapshot.codiceStato);
  html += "</p>";
  html += "<p><strong>Ts:</strong> ";
  html += formatTsForDashboard(snapshot.unixTs);
  html += "</p>";
  html += "<p><strong>Nota:</strong> ";
  html += escapeHtml(String(snapshot.nota));
  html += "</p>";
  html += "</article>";
  return html;
}

bool isFirmwareRequestAuthenticated() {
  return deviceWebServer.authenticate(WEB_FW_UPLOAD_USER, OTA_AUTH_PASSWORD);
}

bool ensureFirmwareAuth() {
  if (isFirmwareRequestAuthenticated()) {
    return true;
  }
  deviceWebServer.requestAuthentication(BASIC_AUTH, "ESP32 Firmware", "Credenziali richieste");
  return false;
}

void handleDashboardHome() {
  String wifiState = isWiFiConnected() ? "Connesso" : "Disconnesso";
  String ip = isWiFiConnected() ? WiFi.localIP().toString() : "N/A";

  String html;
  html.reserve(5200);
  html += "<!doctype html><html lang='it'><head>";
  html += "<meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Arnia ESP32 Dashboard</title>";
  html += "<style>";
  html += "html,body{margin:0;padding:0;background:#f4f7fb;color:#111;font-family:Arial,sans-serif;}";
  html += "body{min-height:100vh;background:linear-gradient(120deg,#eef5ff,#dce9ff);}main{padding:14px 18px;}";
  html += ".top{display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap;}";
  html += "h1{margin:0;font-size:1.3rem;}a.btn{display:inline-block;background:#0b57d0;color:#fff;padding:10px 14px;border-radius:9px;text-decoration:none;font-weight:600;}";
  html += ".meta{margin:8px 0 14px 0;padding:10px;background:#fff;border-radius:10px;border:1px solid #d2def0;font-size:0.92rem;}";
  html += ".grid{display:grid;grid-template-columns:repeat(4,minmax(220px,1fr));gap:10px;}";
  html += ".card{background:#fff;border:1px solid #ced8e8;border-radius:12px;padding:11px;box-shadow:0 1px 2px rgba(0,0,0,0.05);}";
  html += ".card h2{margin:0 0 8px 0;font-size:1rem;}.value{font-size:1.55rem;font-weight:700;}";
  html += ".value span{font-size:0.92rem;font-weight:500;color:#334155;}.card p{margin:4px 0;font-size:0.83rem;}";
  html += ".hint{margin-top:10px;font-size:0.79rem;color:#334155;}";
  html += "@media (max-width:1200px){.grid{grid-template-columns:repeat(2,minmax(220px,1fr));}}";
  html += "@media (max-width:640px){.grid{grid-template-columns:1fr;}}";
  html += "</style></head><body><main>";
  html += "<div class='top'><h1>Dashboard Arnia ESP32 (Landscape)</h1><a class='btn' href='/fw'>Upload firmware</a></div>";
  html += "<div class='meta'><strong>MAC:</strong> ";
  html += String(deviceMacAddress);
  html += " | <strong>Wi-Fi:</strong> ";
  html += wifiState;
  html += " | <strong>IP:</strong> ";
  html += ip;
  html += " | <strong>Uptime:</strong> ";
  html += String(millis() / 1000UL);
  html += "s</div>";
  html += "<section class='grid'>";
  html += buildSensorCard("DS18B20 Temperatura Interna", "C", snapshotDs18b20);
  html += buildSensorCard("SHT21 Umidita", "%", snapshotSht21Hum);
  html += buildSensorCard("SHT21 Temperatura Ambiente", "C", snapshotSht21Temp);
  html += buildSensorCard("HX711 Peso", "kg", snapshotHx711);
  html += "</section>";
  html += "<div class='hint'>Aggiornamento pagina automatico ogni 10 secondi.</div>";
  html += "<script>setTimeout(function(){location.reload();},10000);</script>";
  html += "</main></body></html>";

  deviceWebServer.send(200, "text/html; charset=utf-8", html);
}

void handleFirmwarePage() {
  if (!ensureFirmwareAuth()) {
    return;
  }

  String html;
  html.reserve(2200);
  html += "<!doctype html><html lang='it'><head>";
  html += "<meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Firmware Upload ESP32</title>";
  html += "<style>";
  html += "body{margin:0;background:#f5f7fb;font-family:Arial,sans-serif;color:#111;}";
  html += "main{max-width:760px;margin:24px auto;padding:16px;}";
  html += ".card{background:#fff;padding:16px;border:1px solid #d2deef;border-radius:12px;}";
  html += "h1{margin-top:0;}input[type=file]{width:100%;padding:8px;border:1px solid #c7d3e6;border-radius:8px;}";
  html += "button{margin-top:12px;background:#0b57d0;color:#fff;border:0;border-radius:8px;padding:10px 15px;font-weight:600;cursor:pointer;}";
  html += "a{display:inline-block;margin-top:14px;color:#0b57d0;text-decoration:none;}";
  html += "</style></head><body><main><section class='card'>";
  html += "<h1>Aggiornamento Firmware</h1>";
  html += "<p>Seleziona un file <code>.bin</code> compilato per questa board ESP32-CAM.</p>";
  html += "<form method='POST' action='/fw/upload' enctype='multipart/form-data'>";
  html += "<input type='file' name='firmware' accept='.bin' required>";
  html += "<button type='submit'>Carica firmware</button>";
  html += "</form>";
  html += "<a href='/'>Torna alla dashboard</a>";
  html += "</section></main></body></html>";

  deviceWebServer.send(200, "text/html; charset=utf-8", html);
}

void handleFirmwareUploadStream() {
  if (!isFirmwareRequestAuthenticated()) {
    return;
  }

  HTTPUpload& upload = deviceWebServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("FW upload start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("FW upload completato: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    Serial.println("FW upload annullato");
  }
}

void handleFirmwareUploadResult() {
  if (!ensureFirmwareAuth()) {
    return;
  }

  if (Update.hasError()) {
    deviceWebServer.send(500, "text/plain; charset=utf-8", "Upload firmware fallito");
    return;
  }

  deviceWebServer.send(200, "text/html; charset=utf-8",
    "<html><body><h1>Firmware aggiornato</h1><p>Riavvio in corso...</p></body></html>");
  delay(700);
  ESP.restart();
}

void initDeviceWebServer() {
  deviceWebServer.on("/", HTTP_GET, handleDashboardHome);
  deviceWebServer.on("/fw", HTTP_GET, handleFirmwarePage);
  deviceWebServer.on("/fw/upload", HTTP_POST, handleFirmwareUploadResult, handleFirmwareUploadStream);
  deviceWebServer.onNotFound([]() {
    deviceWebServer.send(404, "application/json; charset=utf-8", "{\"error\":\"Not found\"}");
  });
  deviceWebServer.begin();
  Serial.println("  + Web dashboard locale attiva su porta 80");
  Serial.println("    URL: http://<ip-esp>/");
  Serial.println("    FW user: admin (password uguale a OTA)");
}

void inviaStatoSensoreRuntime(const char* tipoSensore, const char* evento,
                              const char* causaCodice, const char* causaDettaglio,
                              int codiceStato, float valore) {
  if (!isWiFiConnected()) {
    return;
  }

  const char* sensorId = getSensorIdByTipo(tipoSensore);
  bool abilitato = isSensorAbilitatoByTipo(tipoSensore);
  unsigned long ts = getUnixTimestamp();

  bool ok = send_sensor_runtime_status(
    deviceMacAddress,
    tipoSensore,
    sensorId,
    abilitato,
    evento,
    causaCodice,
    causaDettaglio,
    codiceStato,
    valore,
    ts
  );

  if (!ok) {
    Serial.print("  ! Invio stato sensore fallito: ");
    Serial.println(tipoSensore);
  }
}

// ============================================================================
// LETTURA MAC ADDRESS ROBUSTA
// ============================================================================
bool leggiMacAddressDispositivo(char* outMac, size_t outSize) {
  if (outMac == nullptr || outSize < 18) {
    return false;
  }

  uint8_t mac[6] = {0};
  esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);

  if (err != ESP_OK) {
    WiFi.macAddress(mac);
  }

  bool allZero = true;
  for (int i = 0; i < 6; i++) {
    if (mac[i] != 0) {
      allZero = false;
      break;
    }
  }

  // Fallback: alcuni stack possono restituire MAC vuoto se Wi-Fi non pronto.
  if (allZero) {
    uint64_t chipMac = ESP.getEfuseMac();
    mac[0] = (chipMac >> 40) & 0xFF;
    mac[1] = (chipMac >> 32) & 0xFF;
    mac[2] = (chipMac >> 24) & 0xFF;
    mac[3] = (chipMac >> 16) & 0xFF;
    mac[4] = (chipMac >> 8) & 0xFF;
    mac[5] = chipMac & 0xFF;

    allZero = true;
    for (int i = 0; i < 6; i++) {
      if (mac[i] != 0) {
        allZero = false;
        break;
      }
    }
  }

  snprintf(outMac, outSize, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return !allZero;
}

// ============================================================================
// GESTIONE WI-FI CON WiFiMulti
// ============================================================================
void initWiFi() {
  WiFi.mode(WIFI_STA);
  delay(50);

  bool macValido = leggiMacAddressDispositivo(deviceMacAddress, sizeof(deviceMacAddress));

  Serial.print("  MAC Address: ");
  Serial.println(deviceMacAddress);
  if (!macValido) {
    Serial.println("  ! MAC non affidabile, verificare eFuse / inizializzazione Wi-Fi");
  }

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
    sincronizzaTempoNtp(true);
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
        sincronizzaTempoNtp(true);
      }
      wifiConnesso = true;
      sincronizzaTempoNtp(false);
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

  // Report stato sensori ad ogni download/applicazione configurazione.
  if (!configSensori.ds18b20.abilitato) {
    inviaStatoSensoreRuntime("ds18b20", "CONFIG_SYNC", "SENSORE_DISABILITATO_CONFIG", "sea_stato=false", 9000, NAN);
  } else if (!isValidSeaId(configSensori.ds18b20.sensorId)) {
    inviaStatoSensoreRuntime("ds18b20", "CONFIG_SYNC", "SENSOR_ID_NON_VALIDO_CONFIG", "sea_id assente/non numerico", 9000, NAN);
  } else {
    inviaStatoSensoreRuntime("ds18b20", "CONFIG_SYNC", "", "", 9000, NAN);
  }

  if (!configSensori.sht21_humidity.abilitato) {
    inviaStatoSensoreRuntime("sht21_humidity", "CONFIG_SYNC", "SENSORE_DISABILITATO_CONFIG", "sea_stato=false", 9000, NAN);
  } else if (!isValidSeaId(configSensori.sht21_humidity.sensorId)) {
    inviaStatoSensoreRuntime("sht21_humidity", "CONFIG_SYNC", "SENSOR_ID_NON_VALIDO_CONFIG", "sea_id assente/non numerico", 9000, NAN);
  } else {
    inviaStatoSensoreRuntime("sht21_humidity", "CONFIG_SYNC", "", "", 9000, NAN);
  }

  if (!configSensori.sht21_temperature.abilitato) {
    inviaStatoSensoreRuntime("sht21_temperature", "CONFIG_SYNC", "SENSORE_DISABILITATO_CONFIG", "sea_stato=false", 9000, NAN);
  } else if (!isValidSeaId(configSensori.sht21_temperature.sensorId)) {
    inviaStatoSensoreRuntime("sht21_temperature", "CONFIG_SYNC", "SENSOR_ID_NON_VALIDO_CONFIG", "sea_id assente/non numerico", 9000, NAN);
  } else {
    inviaStatoSensoreRuntime("sht21_temperature", "CONFIG_SYNC", "", "", 9000, NAN);
  }

  if (!configSensori.hx711.abilitato) {
    inviaStatoSensoreRuntime("hx711", "CONFIG_SYNC", "SENSORE_DISABILITATO_CONFIG", "sea_stato=false", 9000, NAN);
  } else if (!isValidSeaId(configSensori.hx711.sensorId)) {
    inviaStatoSensoreRuntime("hx711", "CONFIG_SYNC", "SENSOR_ID_NON_VALIDO_CONFIG", "sea_id assente/non numerico", 9000, NAN);
  } else {
    inviaStatoSensoreRuntime("hx711", "CONFIG_SYNC", "", "", 9000, NAN);
  }

  snapshotDs18b20.abilitato = configSensori.ds18b20.abilitato;
  snapshotSht21Hum.abilitato = configSensori.sht21_humidity.abilitato;
  snapshotSht21Temp.abilitato = configSensori.sht21_temperature.abilitato;
  snapshotHx711.abilitato = configSensori.hx711.abilitato;

  Serial.println("\n--- CONFIGURAZIONE APPLICATA ---\n");
}

// ============================================================================
// INVIO DATO SENSORE
// ============================================================================
// tipoSensore: "ds18b20", "sht21_humidity", "sht21_temperature", "hx711"
// ============================================================================
bool inviaDatoSensore(const char* tipoSensore, RisultatoValidazione* risultato) {

  if (!isWiFiConnected()) {
    Serial.println("  ! Wi-Fi non connesso, dato non inviato");
    aggiornaSnapshotSensore(tipoSensore, risultato, false, "Wi-Fi disconnesso");
    return false;
  }

  // Ottieni il sensorId dalla configurazione in base al tipo.
  const char* sensorId = getSensorIdByTipo(tipoSensore);

  if (!isValidSeaId(sensorId)) {
    Serial.print("  ! SensorId non configurato per: ");
    Serial.println(tipoSensore);
    inviaStatoSensoreRuntime(
      tipoSensore,
      "INVIO_BLOCCATO",
      "SENSOR_ID_NON_VALIDO",
      "Impossibile inviare: sea_id mancante/non numerico",
      risultato->codiceErrore,
      risultato->valorePulito
    );
    aggiornaSnapshotSensore(tipoSensore, risultato, false, "sea_id non valido");
    return false;
  }

  unsigned long timestamp = getUnixTimestamp();

  bool salvato = save_value_with_context(
    sensorId,
    deviceMacAddress,
    tipoSensore,
    risultato->valorePulito,
    timestamp,
    risultato->codiceErrore
  );

  if (!salvato) {
    Serial.println("  ! Errore salvataggio dato");
    inviaStatoSensoreRuntime(
      tipoSensore,
      "ERRORE_SERVER",
      "POST_RILEVAZIONE_FALLITO",
      "Errore HTTP su /rilevazioni",
      risultato->codiceErrore,
      risultato->valorePulito
    );
    aggiornaSnapshotSensore(tipoSensore, risultato, false, "POST /rilevazioni fallito");
    return false;
  }

  aggiornaSnapshotSensore(tipoSensore, risultato, true, "Dato inviato al server");
  return true;
}

// ============================================================================
// SETUP PRINCIPALE
// ============================================================================
void setup() {
  if (ledPin >= 0) {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
  }

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

    // Inizializza mDNS per OTA discovery
    String hostname = "arnia-" + String(deviceMacAddress).substring(12);
    hostname.replace(":", "");
    if (MDNS.begin(hostname.c_str())) {
      Serial.print("  + mDNS avviato: ");
      Serial.print(hostname);
      Serial.println(".local");
    } else {
      Serial.println("  ! mDNS non avviato");
    }
  } else {
    Serial.println("  !  Wi-Fi non disponibile, modalita' offline\n");
  }

  // Configura OTA con hostname
  String otaHostname = "arnia-" + String(deviceMacAddress).substring(12);
  otaHostname.replace(":", "");
  ArduinoOTA.setHostname(otaHostname.c_str());

  // Password can be set with plain text (will be hashed internally)
  // The authentication uses PBKDF2-HMAC-SHA256 with 10,000 iterations
  ArduinoOTA.setPassword(OTA_AUTH_PASSWORD);
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
  Serial.println("  + OTA pronto\n");

  initDeviceWebServer();

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
  deviceWebServer.handleClient();

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
    } else {
      inviaStatoSensoreRuntime(
        "ds18b20",
        "LETTURA_NON_VALIDA",
        "VALIDAZIONE_FALLITA",
        risultato.messaggioErrore,
        risultato.codiceErrore,
        risultato.valorePulito
      );
      aggiornaSnapshotSensore("ds18b20", &risultato, false, "Lettura non valida");
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
    } else {
      inviaStatoSensoreRuntime(
        "sht21_humidity",
        "LETTURA_NON_VALIDA",
        "VALIDAZIONE_FALLITA",
        risultato.messaggioErrore,
        risultato.codiceErrore,
        risultato.valorePulito
      );
      aggiornaSnapshotSensore("sht21_humidity", &risultato, false, "Lettura non valida");
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
    } else {
      inviaStatoSensoreRuntime(
        "sht21_temperature",
        "LETTURA_NON_VALIDA",
        "VALIDAZIONE_FALLITA",
        risultato.messaggioErrore,
        risultato.codiceErrore,
        risultato.valorePulito
      );
      aggiornaSnapshotSensore("sht21_temperature", &risultato, false, "Lettura non valida");
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
    } else {
      inviaStatoSensoreRuntime(
        "hx711",
        "LETTURA_NON_VALIDA",
        "VALIDAZIONE_FALLITA",
        risultato.messaggioErrore,
        risultato.codiceErrore,
        risultato.valorePulito
      );
      aggiornaSnapshotSensore("hx711", &risultato, false, "Lettura non valida");
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
