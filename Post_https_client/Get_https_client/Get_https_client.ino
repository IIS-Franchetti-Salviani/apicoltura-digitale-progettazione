#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Configurazione WiFi
#define STASSID "Gruppo4Network"
#define STAPSK "Networks"

// Configurazione RestDB
#define RESTDB_URL "https://clonedb1-7b36.restdb.io"
#define API_KEY "2e1c9e05dd157fa74d69bfeab6b520b7c1e58"

// ID del record da recuperare (modificalo con un ID valido del tuo DB)
#define RECORD_ID "678a1b2c3d4e5f6g7h8i9j0k"  // ⚠️ SOSTITUISCI CON UN ID REALE

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32-CAM RestDB GET Test ===");
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
    
    // ===== GET SINGOLO RECORD =====
    getSingoloRecord();
    
  } else {
    Serial.println("✗ Connessione WiFi fallita!");
    Serial.println("Verifica SSID e password, poi resetta ESP32");
  }
  
  Serial.println("\n=== Setup completato ===");
  Serial.println("Premi RESET per recuperare nuovamente il record");
}

void loop() {
  delay(1000);
}

// ===== FUNZIONE GET SINGOLO RECORD =====
void getSingoloRecord() {
  Serial.println("\n--- Recupero singolo record ---");
  
  WiFiClientSecure *client = new WiFiClientSecure;
  
  if(client) {
    client->setInsecure();  // Per sviluppo
    
    HTTPClient https;
    
    // Endpoint per singolo record: /rest/COLLECTION/ID
    String endpoint = RESTDB_URL;
    endpoint += "/rest/rilevazioni/";  // Cambia collection se necessario
    endpoint += RECORD_ID;
    
    Serial.print("[HTTPS] Connessione a: ");
    Serial.println(endpoint);
    
    if (https.begin(*client, endpoint)) {
      
      // Headers richiesti
      https.addHeader("Content-Type", "application/json");
      https.addHeader("x-apikey", API_KEY);
      https.addHeader("cache-control", "no-cache");
      
      Serial.print("[HTTP] Richiesta GET in corso");
      int httpCode = https.GET();
      Serial.println(" ...fatto!");
      
      if (httpCode > 0) {
        Serial.printf("[HTTP] Codice risposta: %d ", httpCode);
        
        if (httpCode == HTTP_CODE_OK) {
          Serial.println("✓ SUCCESS");
          
          String payload = https.getString();
          
          Serial.println("\n--- Risposta del server ---");
          Serial.println(payload);
          Serial.println("---------------------------");
          
          // Parsing JSON
          StaticJsonDocument<1024> doc;  // Aumentato per dati completi
          DeserializationError error = deserializeJson(doc, payload);
          
          if (!error) {
            Serial.println("\n✓ Record recuperato con successo!");
            Serial.println("\n--- Dati estratti ---");
            
            // Estrai i campi (adatta ai tuoi campi reali)
            if (doc.containsKey("_id")) {
              Serial.print("  ID: ");
              Serial.println(doc["_id"].as<const char*>());
            }
            
            if (doc.containsKey("weight")) {
              Serial.print("  Peso: ");
              Serial.println(doc["weight"].as<int>());
            }
            
            if (doc.containsKey("humidity")) {
              Serial.print("  Umidità: ");
              Serial.println(doc["humidity"].as<int>());
            }
            
            if (doc.containsKey("temperature")) {
              Serial.print("  Temperatura: ");
              Serial.println(doc["temperature"].as<int>());
            }
            
            if (doc.containsKey("sound_level")) {
              Serial.print("  Livello suono: ");
              Serial.println(doc["sound_level"].as<int>());
            }
            
            Serial.println("---------------------");
            
          } else {
            Serial.print("✗ Errore parsing JSON: ");
            Serial.println(error.c_str());
          }
          
        } else if (httpCode == 404) {
          Serial.println("✗ RECORD NON TROVATO");
          Serial.println("  Verifica che l'ID sia corretto!");
          
        } else {
          Serial.println("✗ ERRORE");
        }
        
      } else {
        Serial.printf("\n✗ [HTTP] Errore connessione: %s\n", 
                     https.errorToString(httpCode).c_str());
      }
      
      https.end();
      
    } else {
      Serial.println("✗ [HTTPS] Impossibile iniziare connessione!");
    }
    
    delete client;
    
  } else {
    Serial.println("✗ Impossibile creare client sicuro!");
  }
}

/*
 * ===== COME USARE QUESTO CODICE =====
 * 
 * 1. Sostituisci RECORD_ID (riga 16) con un ID valido dal tuo database
 *    Esempio: "678a1b2c3d4e5f6g7h8i9j0k"
 * 
 * 2. Carica il codice su ESP32-CAM
 * 
 * 3. Apri Serial Monitor (115200 baud)
 * 
 * 4. Premi RESET - vedrai il record recuperato
 * 
 * ===== COME TROVARE UN ID VALIDO =====
 * 
 * Opzione 1: Usa il tuo codice POST originale
 *   - Invia un record
 *   - Copia l'ID dalla risposta ("_id": "...")
 * 
 * Opzione 2: Vai su RestDB dashboard
 *   - Apri la collection "rilevazioni"
 *   - Clicca su un record
 *   - Copia l'ID mostrato
 * 
 * ===== ALTRE COLLECTION =====
 * 
 * Cambia riga 60 per altre collection:
 * endpoint += "/rest/apiari/";
 * endpoint += "/rest/arnie/";
 * endpoint += "/rest/sensori/";
 * endpoint += "/rest/notifiche/";
 * 
 * ===== VANTAGGI =====
 * 
 * ✓ Recupera un singolo record specifico
 * ✓ Parsing automatico dei dati JSON
 * ✓ Gestione errori (404 se ID non esiste)
 * ✓ Output dettagliato per debugging
 */
