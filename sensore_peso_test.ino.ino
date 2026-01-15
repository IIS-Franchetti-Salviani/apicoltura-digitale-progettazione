#include <Arduino.h>

// STANDARD + TEST
static const float TEST_peso_min_kg = 0.0f;
static const float TEST_peso_max_kg = 40.0f;

// Deep sleep TEST: 20 secondi
static const uint64_t TEST_sleep_us = 20ULL * 1000000ULL;


static void taraturaDifferenzaPeso() {
  // TEST: senza sensore, la taratura non fa nulla
}

static float conversioneInKg() {
  // TEST
  float valore_peso_kg = 12.5f;   
  return valore_peso_kg;
}

static void invioAlertPesoFuoriRange(float valore_peso_kg) {
  Serial.print("ALERT: peso fuori range = ");
  Serial.println(valore_peso_kg, 3);
}

static void confermaPesoValido(float valore_peso_kg) {
  Serial.print("Peso OK: ");
  Serial.print(valore_peso_kg, 3);
  Serial.println(" kg");
}

static void ritornoInDeepSleep() {
  Serial.println("Deep Sleep 20 secondi...\n");
  esp_sleep_enable_timer_wakeup(TEST_sleep_us);
  esp_deep_sleep_start();
}

static void cicloSensorePeso() {
  // Taratura peso
  taraturaDifferenzaPeso();

  // Conversione valore in kg
  float valore_peso_kg = conversioneInKg();

  // Controllo limiti
  if (valore_peso_kg < TEST_peso_min_kg || valore_peso_kg > TEST_peso_max_kg) {
    invioAlertPesoFuoriRange(valore_peso_kg);
  } else {
    confermaPesoValido(valore_peso_kg);
  }

  // Ritorno in deep sleep
  ritornoInDeepSleep();
}


void setup() {
  Serial.begin(115200);
  cicloSensorePeso();
}

void loop() {
  // Non usatotutto gestito da deep sleep
}
