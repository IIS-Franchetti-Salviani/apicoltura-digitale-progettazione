#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include <time.h>

// ====== RESTDB CLONE 1 ======
const char* BASE_URL = "https://clonedb1-7b36.restdb.io";
const char* API_KEY  = "2e1c9e05dd157fa74d69bfeab6b520b7c1e58";

const char* PATH_RILEVAZIONI = "/rest/rilevazioni";
const char* PATH_SENSORI     = "/rest/sensori";

// ====== AP SETUP ======
const char* AP_SSID = "ESP32-SETUP";
const char* AP_PASS = "12345678";

// ====== HW ======
#define BTN_PIN 0
#define HOLD_RESET_MS 10000

Preferences prefs;
WebServer server(80);

const char* DEVICE_NAME = "esp32-cam-01";

// ====== INTERVALLI (ridotto carico) ======
unsigned long postEveryMs = 60000;    // 60s (default)
unsigned long getEveryMs  = 300000;   // 5 minuti

unsigned long lastPost = 0;
unsigned long lastGet  = 0;

// ---------------- UTIL LOG ----------------
void logLine(const String& s) { Serial.println(s); }

void logHeader(const String& title) {
  Serial.println();
  Serial.println("========================================");
  Serial.println(title);
  Serial.println("========================================");
}

// ---------------- RESET ----------------
void factoryReset() {
  logHeader("FACTORY RESET");
  logLine("Cancello Wi-Fi salvata e riavvio...");

  prefs.begin("cfg", false);
  prefs.clear();
  prefs.end();

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(300);

  ESP.restart();
}

void checkHoldReset() {
  static unsigned long t0 = 0;
  bool pressed = (digitalRead(BTN_PIN) == LOW);

  if (pressed && t0 == 0) {
    t0 = millis();
    logLine("[BTN] BOOT premuto: tienilo 10s per reset");
  }
  if (!pressed && t0 != 0) {
    t0 = 0;
    logLine("[BTN] BOOT rilasciato: reset annullato");
  }
  if (t0 && (millis() - t0 >= HOLD_RESET_MS)) {
    factoryReset();
  }
}

// --------------- SETUP PAGE ---------------
String setupHtml() {
  int n = WiFi.scanNetworks();
  String opt;

  for (int i = 0; i < n && i < 20; i++) {
    String s = WiFi.SSID(i);
    if (s.length() == 0) continue;
    opt += "<option value='" + s + "'>" + s + "</option>";
  }
  if (opt.length() == 0) opt = "<option value=''>Nessuna rete</option>";

  String h =
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Setup</title>"
    "<style>"
    "body{font-family:Arial;background:#111;color:#fff;padding:20px}"
    "select,input,button{width:100%;padding:12px;margin:10px 0;font-size:16px}"
    "</style></head><body>"
    "<h3>Setup Wi-Fi</h3>"
    "<form action='/save' method='POST'>"
    "<select name='ssid' required>" + opt + "</select>"
    "<input name='pass' type='password' placeholder='Password'>"
    "<button type='submit'>Salva</button>"
    "</form>"
    "<button onclick=\"location.href='/'\">Aggiorna lista</button>"
    "<p style='font-size:13px;color:#bbb'>BOOT 10s = reset</p>"
    "</body></html>";

  return h;
}

void handleSetupRoot() {
  server.send(200, "text/html", setupHtml());
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  ssid.trim();

  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID mancante");
    return;
  }

  prefs.begin("cfg", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();

  server.send(200, "text/plain", "Salvato. Riavvio...");
  delay(400);
  ESP.restart();
}

void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  delay(200);

  logHeader("SETUP MODE (AP)");
  logLine("Rete: " + String(AP_SSID));
  logLine("Apri: http://192.168.4.1");

  server.on("/", handleSetupRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

// --------------- WIFI STA ---------------
bool connectSaved() {
  prefs.begin("cfg", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.length() == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  logHeader("CONNECT WIFI (STA)");
  logLine("SSID: " + ssid);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    logLine("Connesso ✅");
    logLine("IP: " + WiFi.localIP().toString());
    return true;
  }

  logLine("Connessione fallita ❌");
  return false;
}

// --------------- NTP TIME ---------------
bool timeOk() {
  time_t now = time(nullptr);
  return (now > 1609459200);
}

void initNtp() {
  logHeader("NTP");
  logLine("Sync ora...");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  unsigned long t0 = millis();
  while (!timeOk() && millis() - t0 < 10000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (timeOk()) logLine("NTP OK ✅");
  else logLine("NTP FAIL (continuo comunque) ⚠️");
}

String isoNow() {
  time_t now = time(nullptr);
  struct tm tm_utc;
  gmtime_r(&now, &tm_utc);

  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm_utc);
  return String(buf) + ".000Z";
}

// --------------- SENSORI (stub) ---------------
// sostituisci con letture reali
float readTemperature() { return 25.0; }
float readHumidity()    { return 60.0; }
float readWeight()      { return 1234.0; }
float readSound()       { return 50.0; }

// --------------- HTTPS HELPERS ---------------
void addRestDbHeaders(HTTPClient& https) {
  https.addHeader("content-type", "application/json");
  https.addHeader("cache-control", "no-cache");
  https.addHeader("x-apikey", API_KEY);
}

// POST con retry 1 volta (stesso HTTPClient aperto)
int postOnce(HTTPClient& https, const char* tipo, float value, const String& isoTime) {
  StaticJsonDocument<192> doc;
  doc["ril_dato"] = value;
  doc["ril_dataOra"] = isoTime;
  doc["ril_tipo"] = tipo;
  doc["device"] = DEVICE_NAME;

  String body;
  serializeJson(doc, body);

  int code = https.POST(body);
  if (code > 0) return code;

  delay(300);
  code = https.POST(body);
  return code;
}

bool httpsGetString(const String& url, String& out) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.setTimeout(10000);

  if (!https.begin(client, url)) return false;

  https.addHeader("x-apikey", API_KEY);

  int code = https.GET();
  if (code > 0) out = https.getString();
  https.end();

  return (code > 0);
}

void readCommandsFromSensori() {
  logHeader("GET COMMANDS (sensori)");

  String url = String(BASE_URL) + PATH_SENSORI + "?max=1&sort=_created&dir=-1";

  String payload;
  if (!httpsGetString(url, payload)) {
    logLine("GET fallito");
    return;
  }

  StaticJsonDocument<2048> doc;
  if (deserializeJson(doc, payload)) {
    logLine("JSON parse error");
    return;
  }

  if (!doc.is<JsonArray>()) {
    logLine("Formato non array");
    return;
  }

  JsonArray arr = doc.as<JsonArray>();
  if (arr.size() == 0) {
    logLine("Nessun record");
    return;
  }

  JsonObject last = arr[0];

  if (last.containsKey("interval")) {
    long sec = last["interval"];
    if (sec >= 10 && sec <= 3600) {
      postEveryMs = (unsigned long)sec * 1000UL;
      logLine("Nuovo interval: " + String(sec) + "s");
    } else {
      logLine("interval fuori range (10..3600)");
    }
  } else {
    logLine("Nessun campo interval");
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);

  logHeader("BOOT");

  bool ok = connectSaved();
  if (!ok) {
    startAP();
    return;
  }

  initNtp();
  lastPost = 0;
  lastGet = 0;

  logHeader("RUN MODE");
  logLine("POST ogni " + String(postEveryMs / 1000) + "s");
  logLine("GET  ogni " + String(getEveryMs / 1000) + "s");
}

// ---------------- LOOP ----------------
void loop() {
  checkHoldReset();

  // AP: gestisce solo setup
  if (WiFi.getMode() == WIFI_AP) {
    server.handleClient();
    return;
  }

  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();

  // POST (una sola connessione per ciclo)
  if (now - lastPost >= postEveryMs) {
    lastPost = now;

    String t = isoNow();

    float temp = readTemperature();
    float hum  = readHumidity();
    float w    = readWeight();
    float snd  = readSound();

    logHeader("SEND RILEVAZIONI");
    logLine("Time: " + t);
    logLine("T=" + String(temp, 2) + "  H=" + String(hum, 2) + "  W=" + String(w, 2) + "  S=" + String(snd, 2));

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    https.setTimeout(10000);

    String url = String(BASE_URL) + PATH_RILEVAZIONI;

    if (!https.begin(client, url)) {
      logLine("POST begin failed (tutti)");
    } else {
      addRestDbHeaders(https);

      int c1 = postOnce(https, "temperature", temp, t);
      logLine("POST temperature: " + String(c1));
      delay(200);

      int c2 = postOnce(https, "humidity", hum, t);
      logLine("POST humidity: " + String(c2));
      delay(200);

      int c3 = postOnce(https, "weight", w, t);
      logLine("POST weight: " + String(c3));
      delay(200);

      int c4 = postOnce(https, "sound_level", snd, t);
      logLine("POST sound_level: " + String(c4));

      https.end();
    }
  }

  // GET
  if (now - lastGet >= getEveryMs) {
    lastGet = now;
    readCommandsFromSensori();
  }
}
