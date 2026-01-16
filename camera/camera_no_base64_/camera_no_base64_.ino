#include "esp_camera.h"
#include "Arduino.h"

// --- CONFIGURAZIONE ---
const unsigned long intervalloFoto = 900000; // 15 minuti (40000 per test da 40 sec)
unsigned long ultimoScatto = 0;
const int BATTERY_PIN = 12; // Pin scelto per evitare conflitti con la camera

// --- CONFIGURAZIONE PIN AI-THINKER ---
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n--- AVVIO SISTEMA ---");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; 
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Errore Camera: 0x%x", err);
    return;
  }
  Serial.println("Camera pronta! Scrivi 'foto' per testare manualmente.");
}

void scattaEReport(String motivo) {
  // Lettura voltaggio (Approssimativa con solo 100k)
  int r = analogRead(BATTERY_PIN);
  float v = (r * 3.3 / 4095.0); 

  Serial.println("\n========================================");
  Serial.println("REPORT: " + motivo);
  Serial.print("Tensione rilevata (Pin 12): "); Serial.print(v); Serial.println("V");
  Serial.println("========================================");

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("ERRORE: Acquisizione fallita!");
    return;
  }

  Serial.printf("Immagine JPEG: %zu bytes\n", fb->len);
  Serial.println("--- INIZIO HEX ---");
  for (size_t i = 0; i < fb->len; i++) {
    if (fb->buf[i] < 0x10) Serial.print("0");
    Serial.print(fb->buf[i], HEX);
    if ((i + 1) % 32 == 0) Serial.println();
    else Serial.print(" ");
  }
  Serial.println("\n--- FINE HEX ---");
  
  esp_camera_fb_return(fb);
}

void loop() {
  unsigned long adesso = millis();

  // 1. Scatto automatico ogni 15 minuti
  if (adesso - ultimoScatto >= intervalloFoto) {
    ultimoScatto = adesso;
    scattaEReport("AUTOMATICO");
  }

  // 2. Comandi da Monitor Seriale
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    if (comando == "foto") {
      scattaEReport("MANUALE");
    }
  }
}
