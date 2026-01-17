#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

// Configurazione WiFi
#define STASSID "Gruppo4Network"
#define STAPSK  "Networks"

// Configurazione RestDB - CLONE 5 (SCORTA)
#define RESTDB_URL "https://clonedb5dhsjjhhfudii-66f3.restdb.io"
#define API_KEY "28ade382b313db86d3cab6da35d50b0666f2f"

// Configurazione NTP per timestamp
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;      // GMT+1 (Italia)
const int daylightOffset_sec = 3600;  // Ora legale

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32-CAM - Invio Rilevazioni ===");
  Serial.println("Avvio connessione WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi connesso!");
    Serial.print("  IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("  RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    // Sincronizza orologio con NTP
    Serial.println("\n⏰ Sincronizzazione ora...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(2000); // Attendi sincronizzazione
    
    // INVIO DATI ALLA TABELLA RILEVAZIONI
    inviaRilevazione();
    
    // Delay di sicurezza
    Serial.println("\n⏳ Attendo 60 secondi...");
    delay(60000);
    
  } else {
    Serial.println("✗ Connessione WiFi fallita!");
  }
  
  Serial.println("\n=== Pronto ===");
  Serial.println("Premi RESET per inviare una nuova rilevazione");
}

void loop() {
  delay(1000);
}

// ===== FUNZIONE PER OTTENERE TIMESTAMP ISO 8601 =====
String getISOTimestamp() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("⚠️ Impossibile ottenere l'ora");
    return "2026-01-17T12:00:00.000Z"; // Fallback
  }
  
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);
  return String(buffer);
}

// ===== FUNZIONE INVIO RILEVAZIONE =====
void inviaRilevazione() {
  Serial.println("\n--- Invio Rilevazione ---");
  
  WiFiClientSecure *client = new WiFiClientSecure;
  
  if(client) {
    client->setInsecure();
    
    HTTPClient https;
    String endpoint = RESTDB_URL;
    endpoint += "/rest/rilevazioni";
    
    Serial.print("[HTTPS] POST → ");
    Serial.println(endpoint);
    
    if (https.begin(*client, endpoint)) {
      
      // Headers RestDB
      https.addHeader("Content-Type", "application/json");
      https.addHeader("x-apikey", API_KEY);
      https.addHeader("cache-control", "no-cache");
      
      // ===== DATI DA INVIARE =====
      StaticJsonDocument<400> doc;
      
      // CAMPI OBBLIGATORI
      doc["ril_dato"] = 24.5;                    // Il valore della rilevazione
      doc["ril_dataOra"] = getISOTimestamp();    // Timestamp ISO 8601
      
      // CAMPI OPZIONALI (aggiungi se servono)
      // doc["ril_sensore"] = "SENS001";         // ID del sensore
      // doc["ril_tipo"] = "temperatura";        // Tipo di rilevazione
      // doc["ril_unita"] = "°C";                // Unità di misura
      
      // Converti in JSON
      String jsonOutput;
      serializeJson(doc, jsonOutput);
      
      Serial.println("\n[DATI]");
      Serial.println(jsonOutput);
      
      // Invio POST
      Serial.print("\n[POST] Invio...");
      int httpCode = https.POST(jsonOutput);
      Serial.println(" fatto!");
      
      // Gestione risposta
      if (httpCode > 0) {
        Serial.printf("[HTTP] Codice: %d ", httpCode);
        
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
          Serial.println("✓ SUCCESS");
          
          String payload = https.getString();
          Serial.println("\n--- Risposta Server ---");
          Serial.println(payload);
          Serial.println("-----------------------");
          
          // Parsing risposta
          StaticJsonDocument<512> responseDoc;
          DeserializationError error = deserializeJson(responseDoc, payload);
          
          if (!error) {
            Serial.println("\n✓ Rilevazione salvata!");
            
            if (responseDoc.containsKey("_id")) {
              const char* id = responseDoc["_id"];
              Serial.print("  ID Record: ");
              Serial.println(id);
            }
          } else {
            Serial.print("✗ Errore parsing: ");
            Serial.println(error.c_str());
          }
          
        } else if (httpCode == 400) {
          Serial.println("✗ ERRORE VALIDAZIONE");
          String payload = https.getString();
          Serial.println(payload);
        } else if (httpCode == 429) {
          Serial.println("✗ TROPPI INVII!");
          Serial.println("   Aspetta 2-5 minuti");
        } else {
          Serial.println("✗ ERRORE");
          String payload = https.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("\n✗ Errore: %s\n", 
                      https.errorToString(httpCode).c_str());
      }
      
      https.end();
      
    } else {
      Serial.println("✗ Impossibile iniziare connessione!");
    }
    
    delete client;
    
  } else {
    Serial.println("✗ Impossibile creare client!");
  }
}

/*
 * ===== ISTRUZIONI =====
 * 
 * 1. Carica il codice su ESP32-CAM
 * 2. Apri Serial Monitor (115200 baud)
 * 3. Premi RESET per inviare una rilevazione
 * 4. Aspetta almeno 60 secondi tra un RESET e l'altro
 * 
 * ===== CAMPI OBBLIGATORI =====
 * 
 * ril_dato     → Il valore numerico della rilevazione
 * ril_dataOra  → Timestamp in formato ISO 8601
 * 
 * ===== PERSONALIZZAZIONE =====
 * 
 * Modifica riga 96:
 * doc["ril_dato"] = 24.5;  // Metti qui il tuo valore
 * 
 * Il timestamp viene generato automaticamente!
 * 
 * ===== ESEMPI DI VALORI =====
 * 
 * Temperatura:  doc["ril_dato"] = 24.5;
 * Umidità:      doc["ril_dato"] = 65;
 * Peso:         doc["ril_dato"] = 1256;
 * Suono:        doc["ril_dato"] = 45;
 */
