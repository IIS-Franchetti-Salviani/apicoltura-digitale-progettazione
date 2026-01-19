/*
 * Progetto: Monitoraggio Umidità Arnia
 * Sensore: FC-28 (Flying Fish / MH-Series)
 * Caratteristica: Alimentazione gestita da pin digitale per prevenire corrosione
 */

const int sensorPowerPin = 7; // Pin che alimenta il sensore
const int sensorPin = A0;     // Pin di lettura analogica

// Valori di calibrazione (da testare con il tuo sensore)
const int AIR_VALUE = 1023;   // Valore quando il sensore è all'aria (asciutto)
const int WATER_VALUE = 200;  // Valore quando il sensore è nell'acqua (molto umido)

void setup() {
  pinMode(sensorPowerPin, OUTPUT);
  digitalWrite(sensorPowerPin, LOW); // Partiamo con il sensore spento
  Serial.begin(9600);
  Serial.println("--- Sistema di monitoraggio FC-28 avviato ---");
}

void loop() {
  int umidita = readMoisture();
  
  Serial.print("Umidità rilevata: ");
  Serial.print(umidita);
  Serial.println("%");

  // Per le api non serve leggere ogni secondo. 
  // Una lettura ogni 15-30 minuti è ideale.
  delay(10000); // Test ogni 10 secondi (cambia in 900000 per 15 min)
}

int readMoisture() {
  digitalWrite(sensorPowerPin, HIGH); // Accendo il sensore
  delay(50);                          // Breve attesa per stabilizzare il segnale
  
  int rawValue = analogRead(sensorPin); // Leggo il valore
  
  digitalWrite(sensorPowerPin, LOW);  // Spengo subito il sensore
  
  // Converto il valore grezzo in percentuale
  // Map inverte i valori perché 1023 = asciutto e 200 = bagnato
  int percentage = map(rawValue, AIR_VALUE, WATER_VALUE, 0, 100);
  
  // Vincolo il valore tra 0 e 100 per evitare errori
  return constrain(percentage, 0, 100);
}
