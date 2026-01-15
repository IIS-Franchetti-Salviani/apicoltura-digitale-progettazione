#include <OneWire.h>
#include <DallasTemperature.h>

// --- Configurazione Pin e Tempi ---
#define ONE_WIRE_BUS 2
const unsigned long ATTESA_6_MINUTI = 360000; 

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

// Variabili globali come da diagramma
float sogliaMax; 
float sogliaMin;
int cont = 0; // Il contatore del diagramma

void setup(void) {
  Serial.begin(115200);
  sensors.begin();
  sensors.setResolution(insideThermometer, 9);

  // Inizializzazione corrispondente al blocco "Start" del diagramma
  inizializzaCiclo();
}

// Funzione che accorpa i blocchi: cont=0, leggiSogliaMin, leggiSogliaMax
void inizializzaCiclo() {
  cont = 0;
  sogliaMin = leggiSogliaMinDBSimulato();
  sogliaMax = leggiSogliaMaxDBSimulato();
  Serial.println("--- Nuove soglie caricate e contatore resettato ---");
}

// Simulazione lettura dal database (blocchi leggiSogliaMin/Max)
float leggiSogliaMinDBSimulato() { return 30.0; }
float leggiSogliaMaxDBSimulato() { return 35.0; }

void inviaDatiAlDatabase(float temp, String stato) {
  Serial.print("Invio -> Stato: " + stato);
  if (temp != -999) Serial.print(" | Temp: " + String(temp));
  Serial.println();
}

void gestioneErrore(String tipo) {
  Serial.println("LOG: " + tipo);
  inviaDatiAlDatabase(-999, "ERRORE: " + tipo);
}

void loop(void) {
  // 1. Misura Temperatura
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(insideThermometer);

  // 2. Diamante: Temperatura non null / errore nel sensore?
  if (tempC == DEVICE_DISCONNECTED_C || tempC < -50.0 || tempC > 125.0) {
    // Percorso NO: Gestione Errore
    gestioneErrore("Sensore non disponibile o fuori range");
  } 
  else {
    // Percorso SI: Controllo contatore (Diamante cont != 5)
    while (cont != 5) {
      // Percorso V (Vero): Controllo soglie
      if (tempC > sogliaMax) {
        inviaDatiAlDatabase(tempC, "Alert Soglia Massima");
      } 
      else if (tempC < sogliaMin) {
        inviaDatiAlDatabase(tempC, "Alert Soglia Minima");
      } 
      else {
        // Il diagramma non specifica un'azione se in range, 
        // ma tipicamente si invia un log "OK"
        inviaDatiAlDatabase(tempC, "OK");
      }
      
      cont++; // Incrementiamo il contatore ad ogni misura valida
      Serial.print("Ciclo: "); Serial.println(cont);
    } 
    else {
      // Percorso F (Falso): Torna a START
      inizializzaCiclo();
      // Nota: dopo il reset, il diagramma torna alla misura
      return; // Salta l'attesa per ricalibrare subito o prosegue secondo necessità
    }
  }

  // 3. Attesa 6 minuti (Blocco finale prima del loop)
  Serial.println("Attesa 6 minuti...");
  delay(ATTESA_6_MINUTI);
}
