#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#define SERVER_IP "https://miaURL.restdb.io"

#ifndef STASSID
#define STASSID "accessPointName"
#define STAPSK  "accessPointPassword"
#endif

ESP8266WiFiMulti WiFiMulti;

void setup() {
  Serial.begin(115200);
  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();

    HTTPClient https;
    
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, SERVER_IP "/rest/resources")) 
    {
      https.addHeader("content-type", "application/json");
      https.addHeader("cache-control", "no-cache");
      https.addHeader("x-apikey", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
      
      String output="{\"weight\": 1256,\"humidity\": 100,\"temperature\": 50,\"sound level\": 999}";
      
      Serial.print("[HTTP] POST: ");
      Serial.println(output);

      int httpCode = https.POST(output); 
      
      if (httpCode > 0) {
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);
  
        if (httpCode == HTTP_CODE_OK) {
          const String& payload = https.getString();
          Serial.println("received payload:\n<<");
          Serial.println(payload);
          Serial.println(">>");
        }
      } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
  
      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }

  Serial.println("Wait 10s before next round...");
  delay(10000);
}
