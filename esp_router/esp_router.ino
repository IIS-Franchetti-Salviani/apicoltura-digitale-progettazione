#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#define LED_PIN 4
#define BTN_PIN 0
#define HOLD_RESET_MS 10000

const char* AP_SSID = "ESP32-SETUP";
const char* AP_PASS = "12345678";

WebServer server(80);
Preferences prefs;

bool ledOn = false;

// ---------------- LED ----------------
void ledON()  { digitalWrite(LED_PIN, HIGH); ledOn = true; }
void ledOFF() { digitalWrite(LED_PIN, LOW);  ledOn = false; }

// --------------- RESET ---------------
void factoryReset() {
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

  if (pressed && t0 == 0) t0 = millis();
  if (!pressed) t0 = 0;

  if (t0 && (millis() - t0 >= HOLD_RESET_MS)) {
    factoryReset();
  }
}

// ------------- WEBAPP LED -------------
void pageLed() {
  server.send(200, "text/html", R"rawliteral(
<!doctype html><html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32-CAM</title>
<style>
body{font-family:Arial;text-align:center;background:#000;color:#fff;padding:20px}
button{padding:12px 18px;margin:8px;font-size:18px}
</style>
<script>
async function on(){await fetch('/H')}
async function off(){await fetch('/L')}
setInterval(async ()=>{
  let s = await fetch('/STATO').then(r=>r.json());
  document.getElementById('st').innerHTML = 'LED: ' + s.led + '<br>BTN: ' + s.button;
},500);
</script>
</head><body>
<h2>ESP32-CAM</h2>
<button onclick="on()">ON</button>
<button onclick="off()">OFF</button>
<div id="st">...</div>
<p style="font-size:13px;color:#bbb">BOOT 10s = reset | /reset</p>
</body></html>
)rawliteral");
}

void handleH() { ledON();  server.send(200, "text/plain", "ON"); }
void handleL() { ledOFF(); server.send(200, "text/plain", "OFF"); }

void handleStato() {
  StaticJsonDocument<100> doc;
  doc["led"] = ledOn ? "on" : "off";
  doc["button"] = (digitalRead(BTN_PIN) == LOW) ? "pressed" : "released";
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleReset() {
  server.send(200, "text/plain", "Reset...");
  delay(200);
  factoryReset();
}

// ------------- SETUP PAGE -------------
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
    "<style>body{font-family:Arial;background:#111;color:#fff;padding:20px}"
    "select,input,button{width:100%;padding:12px;margin:10px 0;font-size:16px}"
    "</style></head><body>"
    "<h2>Setup Wi-Fi</h2>"
    "<form action='/save' method='POST'>"
    "<select name='ssid' required>" + opt + "</select>"
    "<input name='pass' type='password' placeholder='Password'>"
    "<button type='submit'>Salva</button>"
    "</form>"
    "<button onclick=\"location.href='/'\">Aggiorna lista</button>"
    "</body></html>";

  return h;
}

void setupPage() {
  server.send(200, "text/html", setupHtml());
}

void saveWifi() {
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

// ----------- WIFI MODES -----------
bool connectSaved() {
  prefs.begin("cfg", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.length() == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(300);
  }

  return (WiFi.status() == WL_CONNECTED);
}

void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  delay(200);
}

// --------------- SETUP --------------
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  ledOFF();

  bool ok = connectSaved();

  server.on("/H", handleH);
  server.on("/L", handleL);
  server.on("/STATO", handleStato);
  server.on("/reset", handleReset);

  if (ok) {
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    server.on("/", pageLed);
  } else {
    startAP();
    Serial.println("AP setup: 192.168.4.1");
    server.on("/", setupPage);
    server.on("/save", HTTP_POST, saveWifi);
  }

  server.begin();
}

void loop() {
  server.handleClient();
  checkHoldReset();
}
