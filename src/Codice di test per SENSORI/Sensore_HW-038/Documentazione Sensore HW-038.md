# HW-038

## PIN
- D5 Pin (Signal): Pin di segnale (uscita analogica).
- VCC (+5V): Positivo.
- GND: Negativo.


## SPECIFICHE
- Tipo di Sensore: Analogico.
- Tensione di esercizio: DC 3V – 5V.
- Corrente di esercizio: Minore o uguale a 20mA ($\le$ 20mA).
- Area di rilevamento: 40mm x 16mm.
- Temperatura operativa: Da 10°C a 30°C.
- Umidità operativa: Da 10% a 90% (senza condensa).
- Dimensioni del prodotto: 66mm x 20mm x 8mm (nota: una tabella indica una lunghezza di 60mm, l'altra specifica 66mm includendo probabilmente i pin).
- Peso: 3.5g.
- Processo di produzione: FR4 a doppia faccia con livellamento della saldatura ad aria calda (HASL / stagnatura).


## LIBRERIA
N/A

## CODICE
```
#include <WiFi.h>

// CONFIGURAZIONE PIN (ESP32-CAM)
const int PIN_ALIMENTAZIONE = 12; // GPIO 12: Dà corrente al sensore
const int PIN_SEGNALE = 13;       // GPIO 13: Legge il valore (ADC2)

// CONFIGURAZIONE WIFI
const char* ssid = "IL_TUO_WIFI";
const char* password = "LA_TUA_PASSWORD";

// SOGLIE DI CALIBRAZIONE (0 - 4095)
// Nota: ESP32 è a 12-bit. 0 = Bagnatissimo, 4095 = Asciuttissimo
const int SOGLIA_SECCO = 3800;    // Sopra questo valore, il secchio è vuoto (Resistenza alta)
const int SOGLIA_PIENO = 2500;    // Sotto questo valore, c'è molta acqua

void setup() {
  Serial.begin(115200);
  
  // Configura i pin
  pinMode(PIN_ALIMENTAZIONE, OUTPUT);
  digitalWrite(PIN_ALIMENTAZIONE, LOW); // Assicuriamoci che sia SPENTO all'avvio
  
  // Per l'ESP32-CAM, il pin 13 è flottante se non usato, meglio definirlo input
  pinMode(PIN_SEGNALE, INPUT);

  Serial.println("Sistema Avviato - Monitoraggio Api");
}

void loop() {
  // LETTURA SENSORE (A WiFi SPENTO)
  
  // Importante: Disconnettere il WiFi per liberare l'ADC2
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100); 

  // Accendiamo il sensore
  digitalWrite(PIN_ALIMENTAZIONE, HIGH); 
  delay(50); // Attendiamo che il segnale si stabilizzi
  
  // Leggiamo il valore
  // Facciamo 3 letture e prendiamo la media per stabilità
  int somma = 0;
  for(int i=0; i<3; i++){
    somma += analogRead(PIN_SEGNALE);
    delay(10);
  }
  int valoreLetto = somma / 3;

  // Spegniamo subito il sensore (Anti-Corrosione)
  digitalWrite(PIN_ALIMENTAZIONE, LOW);
  
  Serial.print("Valore Sensore: ");
  Serial.println(valoreLetto);

  String statoMessaggio = "";
  
  if (valoreLetto >= SOGLIA_SECCO) {
    // STATO: Sotto la soglia / Asciutto
    statoMessaggio = "ALLARME: Secchio vuoto! Acqua finita.";
  } 
  else if (valoreLetto > SOGLIA_PIENO && valoreLetto < SOGLIA_SECCO) {
    // STATO: Inizio Evaporazione / Intermedio
    statoMessaggio = "INFO: Livello in calo (Evaporazione).";
  } 
  else {
    // STATO: Pieno / OK
    statoMessaggio = "OK: Acqua presente.";
  }

  Serial.println(statoMessaggio);

  // INVIO DATI (A WiFi ACCESO)
  
  connettiEInvia(valoreLetto, statoMessaggio);

  // FASE 4: PAUSA (Risparmio Energetico)
  
  Serial.println("Entro in pausa per 10 minuti...");
  // Nota: Per un vero risparmio batteria, qui si userebbe il Deep Sleep
  // Per ora usiamo un delay lungo per testare facilmente
  delay(600000); // 600.000 ms = 10 minuti
}

// Funzione ausiliaria per connettersi e inviare
void connettiEInvia(int valore, String messaggio) {
  Serial.println("Connessione WiFi in corso...");
  WiFi.begin(ssid, password);
  
  int tentativi = 0;
  while (WiFi.status() != WL_CONNECTED && tentativi < 20) {
    delay(500);
    Serial.print(".");
    tentativi++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connesso!");
    
    // QUI ANDRA' IL CODICE PER INVIARE AL SERVER
    // Esempio simulato:
    Serial.print("Invio al server -> Valore: ");
    Serial.print(valore);
    Serial.print(" | Msg: ");
    Serial.println(messaggio);
    
  } else {
    Serial.println("\nErrore: Impossibile connettersi al WiFi.");
  }
  
  // Disconnetti per sicurezza
  WiFi.disconnect(true);
}
```

![Alt Text](https://lastminuteengineers.com/wp-content/uploads/arduino/Water-Level-Sensor-Pinout.png)