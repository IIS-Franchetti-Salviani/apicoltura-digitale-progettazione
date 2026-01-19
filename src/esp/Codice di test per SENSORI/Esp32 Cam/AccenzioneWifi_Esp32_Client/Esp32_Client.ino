#include <WiFi.h>

// nome rete
const char *ssid = "nome";
// password rete
const char *password = "12345678910";

void setup() {
  Serial.begin(115200);
  Serial.println("Setup started");

  // Modalità Client (Station)
  WiFi.mode(WIFI_MODE_STA);

  // Connessione alla rete WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connessione a ");
  Serial.println(ssid);

  // Attesa connessione
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("CONNESSO ALLA RETE!");
  Serial.print("IP assegnato: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Vuoto, non serve nulla
}
