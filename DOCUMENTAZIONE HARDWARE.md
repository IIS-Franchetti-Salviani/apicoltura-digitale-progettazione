## **DOCUMENTAZIONE REQUISITI HARDWARE**

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

![Alt Text](https://ae01.alicdn.com/kf/H6ae6caf067f64ea4915338c69baa4defr/ESP32-CAM-WiFi-Bluetooth-Module-Camera-Module-Development-Board-ESP32-with-Camera-Module-OV2640-2MP-For.jpg_640x640.jpg)

# SHT21 - HTU21

- VIN (Tensione di ingresso)
- GND (Massa)
- SCL (Segnale di clock I2C)
- SDA (Segnale dati I2C)

## Umidità Relativa

- Risoluzione: 0,04 %RH (a 12 bit) / 0,7 %RH (a 8 bit)
- Tolleranza accuratezza: tipica ±2 %RH; massima (vedere Figura 2)
- Ripetibilità: ±0,1 %RH
- Isteresi: ±1 %RH
- Non linearità: <0,1 %RH
- Tempo di risposta (\$\\tau\$ 63%): 8 s
- Intervallo operativo (esteso): da 0 a 100 %RH
- Deriva a lungo termine (Tipica): <0,25 %RH/anno

## Temperatura

- Risoluzione: 0,01 °C (a 14 bit) / 0,04 °C (a 12 bit)
- Tolleranza accuratezza: tipica ±0,3 °C; massima (vedere Figura 3)
- Ripetibilità: ±0,1 °C
- Intervallo operativo (esteso): da -40 a 125 °C
- Tempo di risposta (\$\\tau\$ 63%): da 5 a 30 s
- Deriva a lungo termine (Tipica): <0,02 °C/anno

![Alt Text](https://www.arduiner.com/wp-content/uploads/2014/06/3788-SHT21-Umidita-and-Temp-Sensore.jpg)

# DS18B20

- **Dati (Data)**
- **VCC (+)**
- **GND (-)**

## SPECIFICHE

- Chip: DS18B20
- Intervallo di temperatura: da -55°C a +125°C (da -67°F a 257°F)
- Interfaccia: 1-Wire (richiede un unico pin digitale per la comunicazione)
- Sonda: acciaio inossidabile, diametro 6mm
- Risoluzione: da 9 a 12 bit (configurabile dall'utente)
- Accuratezza: ±0,5°C (nell'intervallo tra -10°C e +85°C)
- Tempo di conversione (max): 750ms
- Supporto reti multi-punto: permette di collegare più sensori sullo stesso bus dati
- Grado di protezione: impermeabile (resistente all'acqua)
- Tensione di alimentazione: 3,0V ~ 5,5V
- Compatibilità e Programmazione: Arduino e altri microcontrollori
- Lunghezza del cavo: 1 metro
- Diametro della sonda: 6mm
- Peso: 15g

![Alt Text](https://lastminuteengineers.com/wp-content/uploads/arduino/Wiring-Waterproof-DS18B20-Temperature-Sensor-to-Arduino.png)

![Alt Text](https://ampul.eu/18307-medium_default/sonda-di-temperatura-ds18b20-10k-3m.jpg)

# INMP441

## SPECIFICHE

- SCK: Clock dei dati seriali per l'interfaccia I2S.
- SD: Uscita dati seriali per l'interfaccia I2S; il pin va in alta impedenza (tri-state) quando non trasmette attivamente sul canale appropriato.
- WS: Selezione della parola dati seriali (Word Select) per l'interfaccia I2S.
- L/R: Selezione del canale Sinistro/Destro; se impostato su basso l'uscita è nel canale sinistro, se alto nel canale destro.
- GND: Massa (Ground) da collegare al PCB (Pin 5, 6 e 9).
- VDD: Alimentazione da 1,8 V a 3,3 V; deve essere disaccoppiato con un condensatore da 0,1 μF collegato specificamente al Pin 6.
- CHIPEN: Abilitazione del microfono; impostare su alto (VDD​) per abilitarlo o su basso (GND) per la modalità power-down.
- Direzionalità: Omnidirezionale.
- Sensibilità: -26 dBFS (tipica a 1 kHz, 94 dB SPL); intervallo garantito tra -29 e -23 dBFS.
- Rapporto Segnale-Rumore (SNR): 61 dBA (pesato A).
- Rumore Equivalente in Ingresso (EIN): 33 dBA SPL (pesato A).
- Gamma Dinamica: 87 dB (derivata da EIN e massimo ingresso acustico).
- Risposta in Frequenza: Da 60 Hz a 15 kHz (punti di roll-off a -3 dB).
- Distorsione Armonica Totale (THD): 3% a 105 dB SPL.
- Reiezione dell'Alimentazione (PSR): -75 dBFS (con onda quadra da 100 mVp-p a 217 Hz su VDD​ = 1,8 V).
- Massimo Ingresso Acustico: 120 dB SPL (picco).
- Rumore di Fondo (Noise Floor): -87 dBFS (pesato A, RMS).
- Tensione di Alimentazione (VDD​): Intervallo operativo reale tra 1,62 V e 3,63 V.
- Consumo di Corrente (Normal Mode): 1,4 mA a 1,8 V oppure 2,2 mA a 3,3 V.
- Consumo in Standby: 0,8 mA.
- Consumo in Power Down: Da 2 μA (a 1,8 V) a 4,5 μA (a 3,3 V).
- Caratteristiche Filtro Digitale: Ritardo di gruppo di 17,2/fS​ secondi; ripple in banda passante di ±0,04 dB; attenuazione in banda d'arresto di 60 dB.
- Livelli Logici Digitali: VIH​ (Alto) minimo 0,7×VDD​; VIL​ (Basso) massimo 0,25×VDD​.
- Specifiche di Temporizzazione (Timing):
- Frequenza SCK: Da 0,5 MHz a 3,2 MHz.
- Frequenza WS: Da 7,8 kHz a 50 kHz.
- Tempi minimi SCK High/Low (tSCH​,tSCL​): 50 ns.
- Periodo SCK (tSCP​): Minimo 312 ns.
- Setup WS (tWSS​): Minimo 0 ns; Hold WS (tWSH​): Minimo 20 ns.

Note Hardware: La linea SD dovrebbe avere una resistenza di pulldown da 100 k\$\\Omega\$ per scaricare la linea durante il tri-state.

![Alt Text](https://plexylab.com/web/image/product.image/1571/image_1024/inmp441-sensor-microfono-interfaz-i2s-mems-bajo-consumo-002)

# HW-038

- D5 Pin (Signal): Pin di segnale (uscita analogica).
- VCC (+5V): Polo positivo dell'alimentazione.
- GND (Ground): Polo negativo (massa).

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

![Alt Text](https://m.media-amazon.com/images/I/41ZlUsZMClL.jpg)

# HX711 (~40kg)

- Pin 1 (VSUP): Ingresso alimentazione regolatore (2,7 ~ 5,5V).
- Pin 2 (BASE): Uscita controllo regolatore (non collegato se non usato).
- Pin 3 (AVDD): Alimentazione analogica (2,6 ~ 5,5V).
- Pin 4 (VFB): Ingresso controllo regolatore (collegare a AGND se non usato).
- Pin 5 (AGND): Massa analogica.
- Pin 6 (VBG): Uscita bypass riferimento.
- Pin 7 (INA-): Ingresso negativo Canale A.
- Pin 8 (INA+): Ingresso positivo Canale A.
- Pin 9 (INB-): Ingresso negativo Canale B.
- Pin 10 (INB+): Ingresso positivo Canale B.
- Pin 11 (PD_SCK): Controllo spegnimento (attivo alto) e ingresso clock seriale.
- Pin 12 (DOUT): Uscita dati seriale.
- Pin 13 (XO): I/O cristallo (non collegato se non usato).
- Pin 14 (XI): Ingresso cristallo o clock esterno (0 = usa oscillatore interno).
- Pin 15 (RATE): Controllo frequenza dati (0: 10Hz; 1: 80Hz).
- Pin 16 (DVDD): Alimentazione digitale (2,6 ~ 5,5V).

## SPECIFICHE TECNICHE

- Intervallo ingresso differenziale: ±0.5(AVDD/GUADAGNO) V.
- Ingresso modo comune: Da AGND + 1,2V a AVDD - 1,3V.
- Frequenza dati in uscita: 10Hz o 80Hz con oscillatore interno.
- Codifica dati: Complemento a 2, da 800000 a 7FFFFF HEX.
- Tempo di assestamento: 400ms (a 10Hz) o 50ms (a 80Hz).
- Rumore in ingresso (Guadagno 128): 50 nV(rms) a 10Hz o 90 nV(rms) a 80Hz.
- Deriva termica dell'offset: ±6 nV/°C.
- Reiezione dei disturbi (PSRR/CMRR): 100 dB.
- Frequenza clock esterno: Da 1 MHz a 20 MHz.
- Consumo corrente normale: Analogica 1400 µA, Digitale 100 µA.
- Consumo corrente "Power down": Analogica 0,3 µA, Digitale 0,2 µA.

![Alt Text](https://m.media-amazon.com/images/I/61O-sYI2d0L._AC_UF894,1000_QL80_.jpg)