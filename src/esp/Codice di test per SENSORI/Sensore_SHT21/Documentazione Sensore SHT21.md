**SHT21 - HTU21 con Sistema di Monitoraggio Arnia**

Questa versione del codice include un **sistema avanzato di controllo e allerta** per il monitoraggio di arnie digitali. Il sensore non si limita a leggere temperatura e umidità, ma **analizza i valori in tempo reale** e genera automaticamente alert basati su condizioni critiche per la salute dell'alveare.

Mostra immagine

**PINOUT**

- VIN (Tensione di ingresso)
- GND (Massa)
- SCL (Segnale di clock I2C)
- SDA (Segnale dati I2C)

**Umidità Relativa**

- Risoluzione: 0,04 %RH (a 12 bit) / 0,7 %RH (a 8 bit)
- Tolleranza accuratezza: tipica ±2 %RH; massima (vedere Figura 2)
- Ripetibilità: ±0,1 %RH
- Isteresi: ±1 %RH
- Non linearità: <0,1 %RH
- Tempo di risposta (τ\\tau τ 63%): 8 s
- Intervallo operativo (esteso): da 0 a 100 %RH
- Deriva a lungo termine (Tipica): <0,25 %RH/anno

**Temperatura**

- Risoluzione: 0,01 °C (a 14 bit) / 0,04 °C (a 12 bit)
- Tolleranza accuratezza: tipica ±0,3 °C; massima (vedere Figura 3)
- Ripetibilità: ±0,1 °C
- Intervallo operativo (esteso): da -40 a 125 °C
- Tempo di risposta (τ\\tau τ 63%): da 5 a 30 s
- Deriva a lungo termine (Tipica): <0,02 °C/anno

**Libreria**

[Adafruit HTU21D v1.1.2](https://github.com/adafruit/Adafruit_HTU21DF_Library)

**Sistema di Monitoraggio e Alert**

Il codice implementa 6 controlli automatici basati sui casi d'uso dell'apicoltura. Ogni controllo utilizza soglie predefinite e confronta i valori attuali con quelli precedenti.

**Soglie Configurabili**

```cpp

# define TEMP_ALTA 35.0

# define UMID_MATURAZIONE 60.0

# define UMID_SCIAMATURA 85.0

# define UMID_CONDENSA 90.0

# define UMID_ALLAGAMENTO 99.0
```
**1\. Controllo Temperatura Elevata**

**Quando:** temperatura > 35°C  
**Tipo:** ALERT  
**Azione:** Avvisa rischio stress termico per le api

```cpp

if (temperatura > TEMP_ALTA) {

Serial.println("🚨 ALERT: TEMPERATURA ELEVATA!");

}
```
**2\. Maturazione Miele**

**Quando:** umidità scende sotto 60%  
**Tipo:** INFO  
**Azione:** Segnala che il miele è pronto per il raccolto

```cpp

if (umidita &lt; UMID_MATURAZIONE && ultimaUmidita &gt;= UMID_MATURAZIONE) {

Serial.println("ℹ️ INFO: Maturazione Miele");

}
```
**3\. Sciamatura**

**Quando:** picco improvviso > 85%  
**Tipo:** ALLARME  
**Azione:** Richiede controllo immediato dell'arnia

```cpp

if (umidita > UMID_SCIAMATURA && ultimaUmidita <= UMID_SCIAMATURA) {

Serial.println("🚨 ALLARME: POSSIBILE SCIAMATURA!");

}
```
**4\. Condensa Invernale**

**Quando:** umidità > 90% per oltre 1 ora  
**Tipo:** ATTENZIONE  
**Azione:** Segnala rischio gocciolamento

```cpp

if (umidita > UMID_CONDENSA) {

if (tempoUmiditaAlta == 0) {

tempoUmiditaAlta = millis();

} else if (millis() - tempoUmiditaAlta > 3600000) {

Serial.println("⚠️ ATTENZIONE: Condensa Prolungata");

}

}
```
**5\. Sensore Bloccato**

**Quando:** 10 letture consecutive identiche  
**Tipo:** MANUTENZIONE  
**Azione:** Indica probabile presenza di propoli

```cpp

if (abs(umidita - ultimaUmidita) < 0.1) {

contatoreValoriFissi++;

if (contatoreValoriFissi > 10) {

Serial.println("🔧 MANUTENZIONE: Sensore Bloccato");

}

}
```
**6\. Allagamento/Saturazione**

**Quando:** umidità ≥ 99%  
**Tipo:** CRITICO  
**Azione:** Richiede verifica urgente integrità arnia

```cpp

if (umidita >= UMID_ALLAGAMENTO) {

Serial.println("🆘 CRITICO: SATURAZIONE RILEVATA!");

}