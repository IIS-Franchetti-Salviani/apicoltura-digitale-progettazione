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

// ============================================================
// CONFIGURAZIONE UTENTE (DA MODIFICARE)
// ============================================================
const char* ssid     = "IL_TUO_NOME_WIFI";   // <--- Scrivi qui il nome del WiFi
const char* password = "LA_TUA_PASSWORD";    // <--- Scrivi qui la password

// ============================================================
// CONFIGURAZIONE HARDWARE
// ============================================================
// Pin Segnale: GPIO 14 (Condiviso con MicroSD - Rimuovere la SD!)
// VCC Sensore: Collegato a 3.3V o 5V (Sempre acceso)
// GND Sensore: Collegato a GND
const int PIN_SEGNALE = 14; 

// ============================================================
// CONFIGURAZIONE SOGLIE (LOGICA: 0 = ARIA, ALTO = ACQUA)
// ============================================================
// Se il valore è SOTTO questa soglia (es. 0-400), il secchio è VUOTO/SECCO.
const int SOGLIA_MINIMA_ACQUA = 400; 

// Se il valore è SOPRA questa soglia (es. > 1500), il secchio è PIENO BENISSIMO.
const int SOGLIA_OTTIMALE = 1500;

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000); // Piccolo ritardo per avviare la seriale

  // Impostiamo il pin in lettura
  pinMode(PIN_SEGNALE, INPUT);

  Serial.println("\n----------------------------------------");
  Serial.println(" SISTEMA MONITORAGGIO ACQUA (ESP32-CAM)");
  Serial.println(" Sensore su GPIO 14 - Logica: 0=Secco");
  Serial.println("----------------------------------------");
}

// ============================================================
// LOOP PRINCIPALE
// ============================================================
void loop() {
  
  // ----------------------------------------------------------
  // FASE 1: LETTURA SENSORE (WiFi SPENTO)
  // ----------------------------------------------------------
  // Il GPIO 14 usa l'ADC2, che non funziona se il WiFi è attivo.
  // Dobbiamo spegnerlo brutalmente per leggere pulito.
  
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100); 

  // Facciamo 5 letture e prendiamo la media per essere precisi
  long somma = 0;
  for(int i=0; i<5; i++){
    somma += analogRead(PIN_SEGNALE);
    delay(20);
  }
  int valoreLetto = somma / 5;

  // ----------------------------------------------------------
  // FASE 2: ANALISI LOGICA
  // ----------------------------------------------------------
  
  Serial.print("Lettura Sensore: ");
  Serial.print(valoreLetto);
  
  String statoMessaggio = "";
  
  // LOGICA: Più il numero è basso, meno acqua c'è.
  
  if (valoreLetto < SOGLIA_MINIMA_ACQUA) {
    // Valore vicino a 0 -> Sensore in aria
    statoMessaggio = " -> ALLARME: Secchio VUOTO (o sensore staccato)!";
  } 
  else if (valoreLetto >= SOGLIA_MINIMA_ACQUA && valoreLetto < SOGLIA_OTTIMALE) {
    // Valore intermedio -> C'è acqua ma non è pieno
    statoMessaggio = " -> INFO: Livello medio/basso.";
  } 
  else {
    // Valore alto -> C'è molta conducibilità
    statoMessaggio = " -> OK: Livello OTTIMALE (Pieno).";
  }

  Serial.println(statoMessaggio);

  // ----------------------------------------------------------
  // FASE 3: CONNESSIONE E INVIO (WiFi ACCESO)
  // ----------------------------------------------------------
  
  // Riaccendiamo il WiFi solo ora
  connettiEInvia(valoreLetto, statoMessaggio);

  // ----------------------------------------------------------
  // FASE 4: PAUSA
  // ----------------------------------------------------------
  
  Serial.println("Attesa 5 secondi per il prossimo test...");
  Serial.println("----------------------------------------");
  
  // NOTA: Cambia questo valore a 600000 (10 minuti) quando lo metti nell'arnia
  delay(5000); 
}

// ============================================================
// FUNZIONE AUSILIARIA PER IL WIFI
// ============================================================
void connettiEInvia(int valore, String messaggio) {
  Serial.println("..Attivazione WiFi..");
  WiFi.begin(ssid, password);
  
  int tentativi = 0;
  // Aspettiamo massimo 10 secondi (20 tentativi da 500ms)
  while (WiFi.status() != WL_CONNECTED && tentativi < 20) {
    delay(500);
    Serial.print(".");
    tentativi++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi CONNESSO!");
    Serial.print("IP Assegnato: ");
    Serial.println(WiFi.localIP());
    
    // --- QUI SIMULIAMO L'INVIO AL SERVER ---
    // (In futuro qui metterai il codice Telegram o MQTT)
    Serial.print(">> DATI INVIATI: Valore=");
    Serial.print(valore);
    Serial.print(" Msg=");
    Serial.println(messaggio);
    
  } else {
    Serial.println("\nERRORE: Impossibile connettersi al WiFi.");
  }
  
  // Disconnettiamo subito per liberare le risorse per la prossima lettura
  WiFi.disconnect(true);
}
```

![Alt Text](https://lastminuteengineers.com/wp-content/uploads/arduino/Water-Level-Sensor-Pinout.png)