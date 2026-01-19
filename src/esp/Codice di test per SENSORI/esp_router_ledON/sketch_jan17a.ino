#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#define LED_PIN 4   // Flash ESP32-CAM
#define BTN_PIN 0
#define HOLD_RESET_MS 10000

const char* AP_SSID = "ESP32-SETUP";
const char* AP_PASS = "12345678";

Preferences prefs;
WebServer server(80);

// ---------- LED ----------
void ledOn()  { digitalWrite(LED_PIN, HIGH); }
void ledOff() { digitalWrite(LED_PIN, LOW); }

void ledBlink(int times, int ms) {
  for (int i = 0; i < times; i++) {
    ledOn();  delay(ms);
    ledOff(); delay(ms);
  }
}

// ---------- LOG ----------
void header(const char* t) {
  Serial.println();
  Serial.println("========================================");
  Serial.println(t);
  Serial.println("========================================");
}

// ---------- RESET ----------
void factoryReset() {
  header("FACTORY RESET");
  Serial.println("Cancello Wi-Fi salvata e riavvio...");

  prefs.begin("cfg", false);
  prefs.clear();
  prefs.end();

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(300);

  ledOff();
  ESP.restart();
}

void checkHoldReset() {
  static unsigned long t0 = 0;
  bool pressed = (digitalRead(BTN_PIN) == LOW);

  if (pressed && t0 == 0) {
    t0 = millis();
    Serial.println("[BTN] BOOT premuto: tienilo 10s per reset");
  }
  if (!pressed && t0 != 0) {
    t0 = 0;
    Serial.println("[BTN] BOOT rilasciato: reset annullato");
  }
  if (t0 && (millis() - t0 >= HOLD_RESET_MS)) {
    factoryReset();
  }
}

// ---------- SETUP HTML ----------
String setupHtml() {
  int n = WiFi.scanNetworks();
  String opt;

  for (int i = 0; i < n && i < 20; i++) {
    String s = WiFi.SSID(i);
    if (s.length() == 0) continue;
    opt += "<option value='" + s + "'>" + s + "</option>";
  }
  if (opt.length() == 0) opt = "<option value=''>Nessuna rete trovata (premi Aggiorna)</option>";

  String h =
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Setup WiFi</title>"
    "<style>"
    "body{font-family:Arial;background:#111;color:#fff;padding:20px}"
    "select,input,button{width:100%;padding:12px;margin:10px 0;font-size:16px}"
    "</style></head><body>"
    "<h3>Setup Wi-Fi</h3>"
    "<form action='/save' method='GET'>"
    "<label>Rete:</label>"
    "<select name='ssid' required>" + opt + "</select>"
    "<label>Password:</label>"
    "<input name='pass' type='password' placeholder='Password'>"
    "<button type='submit'>Salva</button>"
    "</form>"
    "<button onclick=\"location.href='/'\">Aggiorna lista</button>"
    "<p style='font-size:13px;color:#bbb'>BOOT 10s = reset</p>"
    "</body></html>";

  return h;
}

void handleRoot() {
  Serial.println("[HTTP] GET /");
  server.send(200, "text/html", setupHtml());
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  ssid.trim();

  Serial.println("[HTTP] /save");
  Serial.println("SSID: " + ssid);
  Serial.println("PASS len: " + String(pass.length()));

  if (ssid.length() == 0) {
    server.send(400, "text/plain", "Errore: SSID mancante");
    return;
  }

  prefs.begin("cfg", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();

  server.send(200, "text/plain", "OK salvato. Riavvio tra 1s...");
  delay(1000);
  ESP.restart();
}

// ---------- AP MODE ----------
void startAP() {
  // AP + STA per far funzionare bene scanNetworks
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect(true, true);

  // Forza IP 192.168.4.1
  IPAddress ip(192, 168, 4, 1);
  IPAddress gw(192, 168, 4, 1);
  IPAddress mask(255, 255, 255, 0);
  WiFi.softAPConfig(ip, gw, mask);

  WiFi.softAP(AP_SSID, AP_PASS);
  delay(200);

  ledOff();

  header("SETUP MODE (AP)");
  Serial.println(String("Rete: ") + AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP()); // deve essere 192.168.4.1

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_GET, handleSave);
  server.on("/save", HTTP_POST, handleSave);

  // Qualsiasi altra URL -> mostra setup (così non “sparisce” la pagina)
  server.onNotFound([]() {
    Serial.println("[HTTP] NotFound -> redirect setup");
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });

  server.begin();
  Serial.println("[SERVER] Setup avviato");
}

// ---------- STA CONNECT ----------
bool connectSaved() {
  prefs.begin("cfg", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.length() == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  header("CONNECT WIFI (STA)");
  Serial.println("SSID: " + ssid);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connesso ✅");
    Serial.println("IP: " + WiFi.localIP().toString());
    ledBlink(2, 200);
    ledOff();
    return true;
  }

  Serial.println("Connessione fallita ❌");
  ledBlink(3, 600);
  ledOff();
  return false;
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  ledOff();

  header("BOOT");

  if (!connectSaved()) {
    startAP();
  }
}

void loop() {
  checkHoldReset();

  // in AP/AP+STA gestiamo il server
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    server.handleClient();
  }
}
