#include "arduinoFFT.h"

#define SENSOR_PIN 4          
#define SAMPLES 512           
#define SAMPLING_FREQUENCY 2000.0 // Nota il .0 per renderlo double

double vReal[SAMPLES];
double vImag[SAMPLES];

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

unsigned int sampling_period_us;

void setup() {
  Serial.begin(115200);
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  pinMode(SENSOR_PIN, INPUT);
  Serial.println("Analizzatore di frequenza pronto...");
}

void loop() {

  for (int i = 0; i < SAMPLES; i++) {
    unsigned long microseconds = micros();
    vReal[i] = analogRead(SENSOR_PIN);
    vImag[i] = 0;
    while (micros() < (microseconds + sampling_period_us)) {
      // Attesa
    }
  }


  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD); 
  FFT.compute(FFT_FORWARD);                       
  FFT.complexToMagnitude();                       


  double peakFrequency = FFT.majorPeak();


  Serial.print("Frequenza Rilevata: ");
  Serial.print(peakFrequency);
  Serial.println(" Hz");

  analizzaStatoApi((int)peakFrequency);

  delay(100); 
}

void analizzaStatoApi(int hz) {
  if (hz < 20) return; 

  if (hz >= 20 && hz <= 100) {
    Serial.println(" -> STATO: Api tranquille (Attività Normale)");
  } else if (hz > 100 && hz <= 150) {
    Serial.println(" -> ATTENZIONE: Possibile Pre-Sciamatura o Ventilazione");
  } else if (hz > 150 && hz <= 250) {
    Serial.println(" -> ALLARME: Sciamatura imminente / Difesa");
  } else if (hz > 250 && hz <= 350) {
    Serial.println(" -> ALLARME CRITICO: PERICOLO");
  }else{
    Serial.println(" -> RUMORE: Fuori range api");
  }
}
