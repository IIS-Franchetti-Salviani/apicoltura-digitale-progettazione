# KY-038(Analizzatore di Frequenza per Api) 

## PIN
A0: Output Analogico (Collegato a GPIO 4) - Utilizzato per l\'analisi FFT 
G: Negativo (GND) 
+: (VCC 3.3V - 5V) 
D0: Non Utilizzato

## SPECIFICHE
Modello: KY-038 (Small Sound Sensor)

Tensione Operativa (VCC): 3.3V - 5.5V DC 

Chip Comparatore: LM393

Tipo Microfono: Condensatore Elettret (Omnidirezionale) 

Uscita Analogica: Segnale audio grezzo per calcolo Hz tramite FFT 

Indicatori LED: 1x Alimentazione, 1x Attivazione Soglia (L2) 

Applicazione: Rilevamento stati biologici arnia tramite frequenza sonora 

CASI D\'USO (FREQUENZE) 

Frequenza Stato dell\'Arnia Comunicazione 

0 - 20 Hz Anomalia / Morte 
ALLARME: Assenza segnale o colonia persa 

20 - 100 Hz Api Tranquille Messaggio:
\"Quiescenza / Riposo\" 

100 - 150 Hz Attività Routine Messaggio: \"Stato
Normale\" 

150 - 250 Hz Ventilazione Messaggio: \"Forte Attività /
Raccolto\" 

250 - 350 Hz Pre-Sciamatura AVVISO DI ATTENZIONE: Possibile
sciame 

## LIBRERIA
arduinoFFT (Versione 2.x o superiore) 

## CODICE
```
#include \"arduinoFFT.h\"

#define SENSOR_PIN 4 #define SAMPLES 512 #define SAMPLING_FREQUENCY
2000.0

double vReal\[SAMPLES\]; double vImag\[SAMPLES\];

ArduinoFFT\<double\> FFT = ArduinoFFT\<double\>(vReal, vImag, SAMPLES,
SAMPLING_FREQUENCY);

unsigned int sampling_period_us;

void setup() { Serial.begin(115200); sampling_period_us = round(1000000
\* (1.0 / SAMPLING_FREQUENCY)); pinMode(SENSOR_PIN, INPUT); }

void loop() { for (int i = 0; i \< SAMPLES; i++) {
unsigned long microseconds = micros();
vReal\[i\] = analogRead(SENSOR_PIN);
vImag\[i\] = 0;

while (micros() \< (microseconds + sampling_period_us));
}

FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
FFT.compute(FFT_FORWARD); FFT.complexToMagnitude();

double peakFrequency = FFT.majorPeak();

Serial.print(\"Frequenza Rilevata: \"); Serial.print(peakFrequency);
Serial.println(\" Hz\");

analizzaStatoApi((int)peakFrequency); delay(100); }

void analizzaStatoApi(int hz) { if (hz \< 20) {

Serial.println(\"ALLARME: Anomalia\"); }

 else if(hz \>= 20 && hz \<=100)

{ Serial.println(\"QUIESCENZA: Api in riposo\"); }

else if (hz \>100 && hz \<= 150)

{ Serial.println(\"STATO NORMALE: Attività
standard\"); }

 else if (hz \> 150 && hz \<= 250)

{ nSerial.println(\"FORTE ATTIVITÀ: Ventilazione\"); }
else if (hz \> 250 && hz \<= 350)

{ Serial.println(\"ATTENZIONE: Pre-Sciamatura\"); }
else
{ Serial.println(\"RUMORE: Fuori range\"); } }

```

![Alt Text](https://win.adrirobot.it/sensori/37_in_1/KY-037_KY-038-Microphone-sound-sensor-module/KY-038-pin.jpg)
