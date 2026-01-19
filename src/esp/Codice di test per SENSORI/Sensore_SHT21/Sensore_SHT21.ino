#include <Wire.h>
#include "Adafruit_HTU21DF.h"

Adafruit_HTU21DF sht21 = Adafruit_HTU21DF();

#define I2C_SDA 15
#define I2C_SCL 14

// Soglie per i controlli
#define TEMP_ALTA 35.0
#define UMID_MATURAZIONE 60.0
#define UMID_SCIAMATURA 85.0
#define UMID_CONDENSA 90.0
#define UMID_ALLAGAMENTO 99.0

// Variabili per tracking
float ultimaUmidita = 0;
float ultimaTemp = 0;
unsigned long tempoUmiditaAlta = 0;
int contatoreValoriFissi = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("========================================");
  Serial.println("  ARNIA DIGITALE - Sistema Monitoraggio");
  Serial.println("  SDA=15, SCL=14");
  Serial.println("========================================");
  
  Wire.begin(I2C_SDA, I2C_SCL);
  
  if (!sht21.begin()) {
    Serial.println("╔══════════════════════════════════════╗");
    Serial.println("║  ⚠️  ERRORE: Sensore non trovato!   ║");
    Serial.println("║  Controlla i collegamenti!          ║");
    Serial.println("╚══════════════════════════════════════╝");
    while (1);
  }
  
  Serial.println("✓ SHT21 Inizializzato Correttamente!");
  Serial.println("✓ Sistema di Allerta Attivo");
  Serial.println("========================================\n");
  delay(1000);
}

void loop() {
  float temperatura = sht21.readTemperature();
  float umidita = sht21.readHumidity();
  
  Serial.print("🌡️  Temperatura: ");
  Serial.print(temperatura, 1);
  Serial.print(" °C  |  ");
  Serial.print("💧 Umidità: ");
  Serial.print(umidita, 1);
  Serial.println(" %");
  
  if (temperatura > TEMP_ALTA) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  🚨 ALERT: TEMPERATURA ELEVATA! 🚨   ║");
    Serial.print("║  Temperatura: ");
    Serial.print(temperatura, 1);
    Serial.println(" °C                ║");
    Serial.println("║  Rischio stress termico per le api   ║");
    Serial.println("╚═══════════════════════════════════════╝");
  }
  
  if (umidita < UMID_MATURAZIONE && ultimaUmidita >= UMID_MATURAZIONE) {
    Serial.println("\n┌───────────────────────────────────────┐");
    Serial.println("│  ℹ️  INFO: Maturazione Miele          │");
    Serial.println("│  L'umidità è scesa sotto il 60%      │");
    Serial.println("│  Il miele sta raggiungendo la giusta │");
    Serial.println("│  densità per il raccolto 🍯          │");
    Serial.println("└───────────────────────────────────────┘");
  }
  
  if (umidita > UMID_SCIAMATURA && ultimaUmidita <= UMID_SCIAMATURA) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  🚨 ALLARME: POSSIBILE SCIAMATURA! 🚨║");
    Serial.print("║  Picco umidità: ");
    Serial.print(umidita, 1);
    Serial.println(" %               ║");
    Serial.println("║  Salita improvvisa rilevata!         ║");
    Serial.println("║  AZIONE: Controlla subito l'arnia!   ║");
    Serial.println("╚═══════════════════════════════════════╝");
  }
  
  if (umidita > UMID_CONDENSA) {
    if (tempoUmiditaAlta == 0) {
      tempoUmiditaAlta = millis();
    } else if (millis() - tempoUmiditaAlta > 3600000) {
      Serial.println("\n┌───────────────────────────────────────┐");
      Serial.println("│  ⚠️  ATTENZIONE: Condensa Prolungata  │");
      Serial.print("│  Umidità > 90% da oltre 1 ora        │");
      Serial.println("│  Rischio gocciolamento sulle api 💧  │");
      Serial.println("│  Verifica ventilazione arnia         │");
      Serial.println("└───────────────────────────────────────┘");
    }
  } else {
    tempoUmiditaAlta = 0;
  }
  
  if (abs(umidita - ultimaUmidita) < 0.1) {
    contatoreValoriFissi++;
    if (contatoreValoriFissi > 10) {
      Serial.println("\n┌───────────────────────────────────────┐");
      Serial.println("│  🔧 MANUTENZIONE: Sensore Bloccato    │");
      Serial.println("│  Valore non reattivo da troppo tempo │");
      Serial.println("│  Probabile presenza di propoli       │");
      Serial.println("│  sulla membrana del sensore          │");
      Serial.println("└───────────────────────────────────────┘");
      contatoreValoriFissi = 0; // Reset per evitare spam
    }
  } else {
    contatoreValoriFissi = 0;
  }
  
  // 5. ALLAGAMENTO / SATURAZIONE (≥ 99%)
  if (umidita >= UMID_ALLAGAMENTO) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  🆘 CRITICO: SATURAZIONE RILEVATA! 🆘║");
    Serial.println("║  Umidità al 99-100%!                 ║");
    Serial.println("║  Possibile allagamento o intrusione  ║");
    Serial.println("║  URGENTE: Verifica integrità arnia!  ║");
    Serial.println("╚═══════════════════════════════════════╝");
  }
  
  // Aggiorna valori precedenti
  ultimaUmidita = umidita;
  ultimaTemp = temperatura;
  
  Serial.println("\n----------------------------------------");
  delay(2000);
}
