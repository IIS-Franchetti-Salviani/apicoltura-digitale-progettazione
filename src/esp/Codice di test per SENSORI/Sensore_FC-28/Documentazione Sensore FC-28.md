# FC-28

![Alt Text](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcSzg-WY_pLlK8zBI-NnvKauqbCPD8qNzRDcjA&s)

## PINOUT

- A0: Output Analogico
- D0: Non Utilizzato
- GND: Negativo
- VCC: Positivo

## SPECIFICHE

- Tipo di Sensore: Analogico.
- Tensione di esercizio: DC 3V - 5V.
- Corrente di esercizio: Minore o uguale a 20mA (\$\\le\$ 20mA).
- Area di rilevamento: 40mm x 16mm.
- Temperatura operativa: Da 10°C a 30°C.
- Umidità operativa: Da 10% a 90% (senza condensa).
- Dimensioni del prodotto: 66mm x 20mm x 8mm (nota: una tabella indica una lunghezza di 60mm, l'altra specifica 66mm includendo probabilmente i pin).
- Peso: 3.5g.
- Processo di produzione: FR4 a doppia faccia con livellamento della saldatura ad aria calda (HASL / stagnatura).

## LIBRERIA
[NTC Thermosistor v2.1.0](https://docs.arduino.cc/libraries/ntc_thermistor/)

## CODICE
```
#include <NTC_Thermistor.h>

// --- CONFIGURAZIONE PIN ---
#define PIN_TEMP        2     // Sensore Temperatura (NTC)
#define PIN_FLYING_FISH 13    // Sensore Ostacoli (Flying Fish)

// --- CONFIGURAZIONE TERMISTORE ---
#define R_RIF           10000 // Resistenza 10k
#define R_NOM           10000 // Resistenza a 25 gradi
#define T_NOM           25    // Temp nominale
#define B_VAL           3950  // Beta value

// Creazione oggetto termistore
Thermistor* thermistor;

void setup() {
  // 1. Avvia la comunicazione seriale
  // IMPORTANTE: Ricorda questo numero (115200)

  // 2. Configura il sensore ostacoli
  pinMode(PIN_FLYING_FISH, INPUT);

  // 3. Configura il sensore temperatura
  thermistor = new NTC_Thermistor(PIN_TEMP, R_RIF, R_NOM, T_NOM, B_VAL);
  
  delay(1000);
}

void loop() {
  // Facciamo una media di 10 letture per stabilizzare il valore
  double sommaTemp = 0;
  for(int i=0; i<10; i++){
      sommaTemp += thermistor->readCelsius();
      delay(5);
  }
  double temperatura = sommaTemp / 10;

  // --- LETTURA OSTACOLO ---
  int ostacolo = digitalRead(PIN_FLYING_FISH);
  String statoOstacolo;
  delay(500); // Aggiorna ogni mezzo secondo
}
```