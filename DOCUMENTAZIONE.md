



# Documentazione ESP32 Logic

Versione 1.0
Team **Firmware & Comunicazione REST** 

# Overview
Il Gruppo C realizza il firmware per il dispositivo (ESP32-CAM + sensori) e la logica di comunicazione che invia i dati al server REST (PHP) e al database. L’obiettivo è garantire acquisizione affidabile dei sensori, formattazione coerente dei dati, buffering locale in caso di rete assente e consegna sicura/riportata delle misure sul server.

## Giornaliero

### Giorno 1 (12/01/25)
Incontro con apicoltore e primo brainstorming con stesura appunti. Visita esperto architetto del software 

### Giorno 2 (13/01/25)
Introduzione a GitHub, unione alla repository di progettazione e creazione branch del gruppo C, stesura requisiti e trasferimento di essi sul file REQUISITI.md nella cartella condivisa. 

### Giorno 3 (14/01/25)
Incontro con architetto del software, correzione dei requisiti funzionali e stesura documentazione di logica ESP32 e i relativi sensori. 

### Giorno 4 (15/01/25)
Creazione Tabella dei Pin di ogni sensore nei confronti dell'ESP32, aggiornati i cicli di misurazione per standardizzarne la nomenclatura e completarne il significato

# Scope
-   Lettura e processing dei sensori definiti dal progetto (SHT21, DS18B20, HX711, INMP441, HW-038, fotocamera).
-   Formattazione dei dati in payload JSON e invio agli endpoint REST esposti dal server (POST misure, POST 	foto, GET comandi).
-   Buffering locale (RAM/EEPROM/SD) e strategia di retry per garantire resilienza offline.
-   Meccanismi minimi di sicurezza (token API, preferibilmente HTTPS).
-   Interfacce per comandi remoti (es. cambio parametri, scatto foto, OTA).

# Tabelle e registri

## Requisiti

### Requisiti Funzionali

- **RF-SW-01 — TM, Temperatura Interna**: Il sensore misura la temperatura interna dell'arnia. 
- **RF-SW-02 — TM, Unità di Misura**: Il sensore misura la temperatura in gradi Celsius (°C).
- **RF-SW-03 — TM, Tipo di Dato**: Il valore del sensore è ospitato in una variabile di tipo float.
- **RF-SW-04 — TM, Timestamp**: Ad ogni misurazione è associata la data e l'ora di esecuzione (formato dd/MM/yyyy; hh:mm).
- **RF-SW-05 — TM, Alert Soglia Massima**: Al superamento della temperatura di 37 °C, il sistema invia un alert. 
- **RF-SW-06 — TM, Alert Soglia Minima**: Al raggiungimento della temperatura di 30 °C, il sistema invia un alert.
- **RF-SW-07 — TM, Frequenza di Aggiornamento**: Il sensore aggiorna la misurazione ogni 6 minuti.
- **RF-SW-08 — PS, Misurazione**:  Il segnale analogico deve essere acquisito e convertito in valore digitale a 24 bit. 
- **RF-SW-09 — PS, Taratura**:  La funzione di taratura deve esistere per fare la differenza dei pesi.
- **RF-SW-10 — PS, Trasforma dato grezzo**: Il valore grezzo deve essere trasformato in kg.
- **RF-SW-11 — PS, Power Management**: La modalità deep sleep viene disattivata ogni 3 ore.
- **RF-SW-12 — PS, Gestione Data**: Ad ogni misurazione deve essere associata una data.
- **RF-SW-13 — PS, Gestione ora**: Ad ogni misurazione deve essere associata un orario. 
- **RF-SW-14 — MIC, Acquisizione continua**: Il microfono deve acquisire il segnale a intervalli di 10 secondi.
- **RF-SW-15 — MIC, Intensità suono**: Si calcola il volume medio in dB per capire l'agitazione dello sciame.
- **RF-SW-16 — MIC, Frequenza suono**: Si calcola la frequenza per avere una misura più precisa dello stato dello sciame.
- **RF-SW-17 — MIC, Verifica soglia**:  Il microfono confronta i valori rilevati con valori di allarme.
- **RF-SW-18 — MIC, Notifica alert**: Se si supera una certa soglia l'apicoltore riceve una notifica.
- **RF-SW-19 — UM, Misurazione umidità come % nell'aria**: Il sensore misura la percentuale di umidità all'interno dell'arnia.
- **RF-SW-20 — UM, Tipo di dato**: Il valore del sensore è ospitato in una variabile float. 
- **RF-SW-21 — UM, Timestamp**: Ad ogni misurazione è associata la data e l'ora di esecuzione (formato dd/MM/yyyy; hh:mm).
- **RF-SW-22 — UM, Alert Soglia Massima**: Al superamento della soglia di umidità del 70% il sistema invia un alert. 
- **RF-SW-23 — UM, Alert Soglia Minima**: Al raggiungimento della soglia di umidità inferiore del 40% il sistema invia un alert. 
- **RF-SW-24 — UM, Frequenza di Aggiornamento**: Il sensore aggiorna la misurazione ogni 6 minuti. 
- **RF-SW-25 — SA, Misurazione Livello Acqua**: Il sensore misura il livello dell'acqua nel secchio in percentuale.
- **RF-SW-26 — SA, Tipo di Dato**: Il valore del sensore è ospitato in una variabile di tipo float.
- **RF-SW-27 — SA, Timestamp**: Ad ogni misurazione è associata la data e l'ora di esecuzione (formato dd/MM/yyyy; hh:mm).
- **RF-SW-28 — SA, Alert Soglia Massima**: Al superamento della soglia massima del secchio il sistema invia un alert. 
- **RF-SW-29 — SA, Alert Soglia Minima**: Al raggiungimento del livello inferiore al 5% il sistema invia un alert. 
- **RF-SW-30 — SA, Frequenza di Aggiornamento**: Il sensore aggiorna la misurazione ogni 40 minuti.
- **RF-SW-31 — SA, Taratura**: Il sensore deve essere tarato salvando il livello più basso e più alto durante il riempimento. 
- **RF-SW-32 — TC, Registrazione Video**: La telecamera registra video da inviare al database ogni minuto.
- **RF-SW-33 — TC, Attivazione Manuale**: L'apicoltore può attivare la telecamera tramite l'applicazione.
- **RF-SW-34 — TC, Attivazione Automatica**: La telecamera si attiva automaticamente quando la luminosità supera il 20%.
- **RF-SW-35 — TC, Timestamp**: Ad ogni invio di video è associata la data e l'ora di registrazione (formato dd/MM/yyyy; hh:mm).
- **RF-SW-36, Funzione di Lettura**: I sensori devono essere tarati manualmente dall'Utente quindi prendere le informazioni dal Database, inclusa la data.
- **RF-SW-37, Funzione di Scrittura**: Scrivere la funzione inviaDatiAlServer() che prende tutti i valori e li spedisce.
- **RF-SW-38, Struttura il JSON**: Definire come sarà il messaggio da inviare al database. Esempio: {"temp": 32.5, "peso": 45.2, "hum": 60, "alert_temp": false ...}
- **RF-SW-39, Gestione DeepSleep**: Decidere quale modalità di risparmio energetico usare (RF-SW-38) per non spegnere il Wi-Fi mentre i sensori devono ancora inviare i dati.
- **RF-SW-40, Creazione di una Documentazione**, creare un documento in grado di raccontare e spiegare l'intero progetto
- **RF-SW-41, Tabella dei Pin**, aggiornare la documentazione continuamente con il numero di pin prestabiliti per comunicare con gli altri gruppi
- **RF-SW-42, Registro Requisiti**, aggiornare il registro requisiti man mano che viene scritto il codice o si presentano problemi
- **RF-SW-43, Informazioni codice**, commentare approfonditamente il codice e descriverlo minuziosamente

### Requisiti Non Funzionali

- **RNF-SW-01 — TM, Persistenza Dati**: Il sensore salva i dati nella memoria locale dell'ESP32 in caso di assenza di connessione
- **RNF-SW-04 — PS, Peso max**: L'arnia pesa al massimo 80kg. 
- **RNF-SW-05 — PS, Notifica peso alto**: L'apicoltore deve ricevere un alert se il peso è troppo alto. 
- **RNF-SW-06 — PS, Notifica peso basso**: L'apicoltore deve ricevere un alert se il peso è troppo basso.
- **RNF-SW-07 — PS, Latenza**: Il sensore impiega circa 2 secondi per misurare dopo l'accensione.
- **RNF-SW-08 — MIC, Tempestività alert**:  La notifica alert deve arrivare all'apicoltore in un tempo ragionevole.
- **RNF-SW-09 — UM, Persistenza Dati**: Il sensore salva i dati nella memoria locale dell'ESP32 in caso di assenza di connessione.
- **RNF-SW-13 — SA, Connettività**: Il sensore richiede connessione di rete costante per l'invio dei dati al database.
- **RNF-SW-19 — TC, Connettività**: La telecamera richiede connessione di rete costante per l'invio dei video al database.
  

## Tabelle dei PIN

-   **SHT21 (I²C) → ESP32‑CAM (AI‑Thinker)**
    
    -   VIN →  **3V3**  (alimentazione 3.3V)
    -   GND →  **GND**  (massa)
    -   SCL →  **IO14 / GPIO14**  (clock I²C)
    -   SDA →  **IO15 / GPIO15**  (dati I²C)
-   **DS18B20 (1‑Wire) → ESP32‑CAM (AI‑Thinker)**
    
    -   VDD →  **3V3**  (alimentazione 3.3V)
    -   GND →  **GND**  (massa)
    -   DATA/OUT →  **IO2 / GPIO2**  (linea dati 1‑Wire)
    -   **Resistenza pull‑up**:  **4.7 kΩ**  tra  **DATA/OUT**  e  **3V3**
-   **HX711 (celle di carico) → ESP32‑CAM (AI‑Thinker)**
    
    -   **Alimentazione modulo HX711**
        -   VCC →  **3V3**  (consigliato su ESP32‑CAM)
        -   GND →  **GND**
    -   **Segnali digitali (verso ESP32‑CAM)**
        -   DT / DOUT →  **IO13 / GPIO13**  (dati)
        -   SCK / PD_SCK →  **IO12 / GPIO12**  (clock)
    -   **Lato celle di carico (verso la bilancia)**
        -   E+ / E− → eccitazione (alimentazione ponte estensimetrico)
        -   A+ / A− → segnale canale A (tipico per celle a 4 fili)
        -   B+ / B− → canale B (di solito non usato)



#Lista Errori e alert sensori

##Sensore utilizzato: DS18B20 (Sensore Temperatura)

### 1.1 Acquisizione dati

-   Lettura del valore di temperatura (float, °C).
-   Generazione del timestamp associato.
-   Riferimenti:  `RF-SW-01`,  `RF-SW-02`.

### 1.2 Verifica integrità del dato

-   Controllo che il valore non sia nullo (non presente).

### 1.3 Verifica intervallo sensore

-   Controllo che il valore sia entro il range operativo previsto dal sensore; se fuori range trattare come dato non valido.

### 1.4 Verifica stato sensore

-   Controllo se il sensore è offline o non risponde.

### 1.5 Salvataggio su memoria locale

-   Se il dato è valido, memorizzazione permanente su ESP32.
-   Riferimento:  `RNF-SW-01`.

### 1.6 Verifica connettività verso il database

-   Controllo della disponibilità di rete e del database remoto.
-   Riferimento:  `RNF-SW-01`.

### 1.7 Sincronizzazione Cloud/DB

-   Se la connessione è disponibile, upload del valore e del relativo timestamp al cloud/DB.
-   Riferimenti:  `RF-SW-03`,  `RF-SW-04`.

### 1.8 Controllo soglia massima

-   Verifica se la temperatura supera la SogliaMax.
-   Riferimento:  `RF-SW-05`.

### 1.9 Alert per superamento soglia massima

-   Se la temperatura > SogliaMax, invio di notifica/alert per condizione critica alta.

### 1.10 Controllo soglia minima

-   Verifica se la temperatura è inferiore alla SogliaMin.
-   Riferimento:  `RF-SW-06`.

### 1.11 Alert per superamento soglia minima

-   Se la temperatura < SogliaMin, invio di notifica/alert per condizione critica bassa.

### 1.12 Intervallo di campionamento

-   Attesa controllata di 6 minuti prima del prossimo ciclo.
-   Riferimento:  `RF-SW-07`.

### 1.13 Tentativo di ripristino dati accumulati

-   Se la connessione ritorna disponibile, tentativo di upload dei dati locali memorizzati su ESP32.
-   Riferimento:  `RNF-SW-01`.

### 1.14 Chiusura loop

-   Ripartenza del ciclo tornando al punto 1.1.

##Sensore utilizzato: HX711 (Peso)

### 2.1 Lettura ADC non pronta

-   **ERR-PS-2.01**  — Lettura ADC non pronta  
    Lettura del valore a 24 bit effettuata quando l’ADC non ha ancora un campione valido (“data not ready”); il dato ottenuto è inattendibile.

### 2.2 Timeout acquisizione

-   **ERR-PS-2.02**  — Timeout acquisizione  
    Entro il tempo massimo previsto il campione non viene acquisito (ADC non risponde o il campione non arriva); la misura viene persa.

### 2.3 Saturazione ADC

-   **ERR-PS-2.03**  — Saturazione ADC  
    Il valore letto rimane fisso al minimo o al massimo (es. sempre 0 oppure full-scale), indicando una misura non valida da scartare.

### 2.4 Valore instabile (rumore alto)

-   **ERR-PS-2.04**  — Valore instabile (rumore alto)  
    Durante la finestra di misura i campioni oscillano oltre la soglia di stabilità prevista, rendendo il valore non affidabile.

### 2.5 Outlier improvviso

-   **ERR-PS-2.05**  — Outlier improvviso  
    Uno o pochi campioni risultano molto distanti dagli altri (picchi); questi devono essere ignorati o filtrati.

### 2.6 Taratura non presente

-   **ERR-PS-2.06**  — Taratura non presente  
    Mancano i parametri necessari (offset o coefficiente) per convertire il dato grezzo in kg.

### 2.7 Taratura non valida

-   **ERR-PS-2.07**  — Taratura non valida  
    I parametri di taratura risultano incoerenti o fuori dall'intervallo plausibile; la conversione in kg è inaffidabile.

### 2.8 Errore conversione in kg

-   **ERR-PS-2.08**  — Errore conversione in kg  
    La trasformazione da valore grezzo a kg produce un valore non valido (NaN, infinito) o fuori scala.

### 2.9 Peso fuori range min/max

-   **ERR-PS-2.09**  — Peso fuori range min/max  
    Il peso calcolato in kg è inferiore al minimo o superiore al massimo configurato; la misura viene scartata.

### 2.10 Peso negativo

-   **ERR-PS-2.10**  — Peso negativo  
    Dopo tara o taratura il peso risulta negativo oltre la tolleranza prevista; indica misura non valida.

### 2.11 Prima misura non stabile dopo wake-up

-   **ERR-PS-2.11**  — Prima misura non stabile dopo wake-up  
    Le prime letture dopo avvio o risveglio dal deep sleep risultano instabili e devono essere scartate.

### 2.12 Timestamp mancante

-   **ERR-PS-2.12**  — Timestamp mancante  
    Alla misura non viene associata data/ora; il dato non è correttamente tracciabile.

### 2.13 Timestamp non valido

-   **ERR-PS-2.13**  — Timestamp non valido  
    Data o ora associate alla misura non sono plausibili (es. valori di default), causando incoerenza nello storico.

### 2.14 Invio fallito

-   **ERR-PS-2.14**  — Invio fallito  
    Il sistema non riesce a inviare o salvare la misura sul server; gestire retry o salvataggio locale.

### 2.15 Duplicazione misura

-   **ERR-PS-2.15**  — Duplicazione misura  
    La stessa misura viene inviata più volte, generando duplicazioni per assenza di controllo di unicità.

##Sensore utilizzato: SHT21 - HTU21 (umidità)

### 3.1 Impossibile rilevare il sensore dell'umidità

-   Il sensore non è presente o non risponde all'inizializzazione.

### 3.2 Impossibile ottenere informazioni sull'umidità

-   Il sensore risponde ma non fornisce valori validi.

### 3.3 Verifica integrità del dato (non nullo)

-   Ogni misura deve fornire un valore numerico valido (non NULL).

### 3.4 Verifica integrità del dato (non negativo)

-   L’umidità, nella logica applicativa, non deve assumere valori negativi.

### 3.5 Verifica tipo dato (float)

-   Il tipo di dato per l’umidità deve essere floating point per preservare precisione decimale.

### 3.6 Verifica stato dispositivo (ESP32 offline)

-   Caso in cui la board non è connessa o non è alimentata; gestire salvataggio locale e retry di connessione.

### 3.7 Controllo soglia massima (umidità troppo alta)

-   Condizione che deve innescare un allarme/azione secondo le soglie definite.

### 3.8 Controllo soglia minima (umidità troppo bassa)

-   Condizione che deve innescare un allarme/azione secondo le soglie definite.


## Consumi sensori

### Consumi previsti

-   **SHT21 (3.3 V)**
    
    -   standby: ~0,1–1 µA
    -   misure ogni 6 min: aggiunta media trascurabile (<< 1 µA)
    -   **medio realistico:**  ~**0,2–2 µA**
-   **DS18B20 (3.0–5.5 V)**
    
    -   standby: ~1 µA
    -   conversione ogni 6 min:
        -   12 bit: aggiunge ~3,1 µA medi
    -   **medio realistico (12 bit):**  ~**4–5 µA**  (meno se 9–11 bit)
-   **HX711 (modulo sempre alimentato, ma in power-down tra letture)**
    
    -   HX711 in power‑down: tipicamente  **< 1 µA**  (il chip)
    -   **MA**  se la cella resta alimentata, il chip non è il problema.
-   **Cella di carico (sempre alimentata)**
    
    -   Dipende dalla resistenza del ponte (molto spesso  **350 Ω**, a volte 1 kΩ).
    -   Corrente ≈ V / R:
        -   **a 3.3 V con 350 Ω:**  3.3/350 ≈  **9,4 mA**
        -   **a 5.0 V con 350 Ω:**  5/350 ≈  **14,3 mA**
        -   **a 3.3 V con 1 kΩ:**  **3,3 mA**
        -   **a 5.0 V con 1 kΩ:**  **5 mA**

### Totale medio (Sensori + bilancia, VCC sempre presente)

-   SHT21: ~**< 0,002 mA**
-   DS18B20: ~**0,004–0,005 mA**
-   HX711 chip: ~ **~0,001 mA o meno**
-   Cella di carico:  **3–15 mA** 
