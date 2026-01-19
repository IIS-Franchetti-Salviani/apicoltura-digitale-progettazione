# **ESP32 CAM**

![Alt Text](https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2020/03/ESP32-CAM-pinout-new.png?resize=828%2C368&quality=100&strip=all&ssl=1)

## FEATURES

- Velocità di clock fino a 160 MHz, potenza di calcolo complessiva fino a 600 DMIPS.
- SRAM integrata da 520 KB, PSRAM esterna da 4 MB.
- Supporta le interfacce UART, SPI, I2C, PWM, ADC e DAC.
- Supporta fotocamere OV2640 e OV7670, con flash integrato.
- Supporta il caricamento delle immagini tramite Wi-Fi.
- Supporta schede di memoria TF (MicroSD).
- Supporta molteplici modalità di risparmio energetico (sleep modes).
- Sistemi Lwip e FreeRTOS integrati.
- Supporta le modalità operative STA (Station), AP (Access Point) e STA+AP.
- Supporta le tecnologie Smart Config e AirKiss (per la configurazione rapida della rete).
- Supporta l'aggiornamento del firmware sia locale tramite porta seriale, sia remoto (FOTA - Firmware Over The Air).

## SPECIFICHE

- SPI Flash: predefinita 32Mbit
- RAM: integrata 520 KB + PSRAM esterna 4MB
- Dimensioni: 27_40.5_4.5 (±0.2) mm
- Bluetooth: standard Bluetooth 4.2 BR/EDR e BLE
- Wi-Fi: 802.11b/g/n/e/i
- Interfacce di supporto: UART, SPI, I2C, PWM
- Supporto scheda TF: massimo 4GB
- Porte IO: 9
- Baud-rate porta seriale: predefinito 115200 bps
- Formato uscita immagini: JPEG (solo supporto OV2640), BMP, SCALA DI GRIGI
- Gamma spettro: 2412 ~ 2484MHz
- Antenna: antenna PCB integrata, guadagno 2dBi
- Potenza di trasmissione: 802.11b: 17±2 dBm (@11Mbps); 802.11g: 14±2 dBm (@54Mbps); 802.11n: 13±2 dBm (@MCS7)
- Sensibilità di ricezione: CCK, 1 Mbps: -90dBm; CCK, 11 Mbps: -85dBm; 6 Mbps (1/2 BPSK): -88dBm; 54 Mbps (3/4 64-QAM): -70dBm; MCS7 (65 Mbps, 72.2 Mbps): -67dBm
- Consumo energetico: Con Flash spento: 180mA@5V; Con Flash acceso alla massima luminosità: 310mA@5V; Deep-sleep: minimo 6mA@5V; Modem-sleep: fino a 20mA@5V; Light-sleep: fino a 6.7mA@5V
- Sicurezza: WPA/WPA2/WPA2-Enterprise/WPS
- Range di alimentazione: 5V
- Temperatura operativa: -20 °C ~ 85 °C
- Ambiente di stoccaggio: -40 °C ~ 90 °C, < 90%RH
- Peso: 10.

## Avviamento Microcontrollore
-Aprire Arduino ide
-andare su File -> Impostazioni -> Url Aggiuntivi per gestore schede
-incollare questo url: https://espressif.github.io/arduino-esp32/package_esp32_index.json
- andare su Strumenti -> Scheda -> Gestore schede -> cercare esp32 by espressif system -> scaricarla.
-poi andare su strumenti -> Scheda -> inserire ai tinker esp32 cam

## Setup wifi
-Gestione in modalità Acces Point
-Gestione in modalità Client

![Alt Text](https://ae01.alicdn.com/kf/H6ae6caf067f64ea4915338c69baa4defr/ESP32-CAM-WiFi-Bluetooth-Module-Camera-Module-Development-Board-ESP32-with-Camera-Module-OV2640-2MP-For.jpg_640x640.jpg)