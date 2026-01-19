#include <WiFi.h>

// nome rete
const char *ssid = "nome";
// password rete
const char *password = "12345678910";

void setup() {
  Serial.begin(115200);
  Serial.println("Setup started");

  // Modalità Access Point
  WiFi.mode(WIFI_MODE_AP);

  // Avvio AP
  bool apOk = WiFi.softAP(ssid, password);

  if (apOk) {
    Serial.println("ACCESS POINT ATTIVO");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("IP del ESP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("ERRORE: impossibile avviare l'Access Point");
  }
}

void loop() {
  // Vuoto, non serve nulla
}
