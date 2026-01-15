

# 🧭 Guida rapida: come si scrivono i requisiti (progetto “Apicoltura Digitale”)

> **Obiettivo:** trasformare idee, appunti e desideri del cliente in frasi **chiare, verificabili e tracciabili** (cioè: controllabili con un test e collegabili alla loro “fonte”).
> Nel nostro progetto, l’obiettivo generale è realizzare un sistema IoT per supportare l’apicoltore con dati remoti su produzione e stato dell’arnia, includendo device IoT, API/REST, base dati e infrastruttura di rete.
> (Contesto progetto: server REST, database, dispositivo IoT e infrastruttura di rete)

## Cosa trovi in questo documento

1. Cos’è un requisito e perché serve
2. Regole d’oro per scriverlo bene (e perché quelli “vaghi” fanno danni)
3. Requisiti **utente** vs requisiti **di sistema**
4. Requisiti **funzionali** vs **non funzionali**
5. Un *template* semplice (da copiare/incollare) per ogni requisito
6. Esempi pronti per i 4 gruppi di lavoro:
- Database su **restdb.io**
- Device **ESP32-CAM**: sensori e hardware
- Software su ESP32-CAM + comunicazione **REST**
- Architettura di rete e infrastruttura
7. Mini nota: cos’è un file **Markdown (.md)** e come si scrive

---

## 1) Cos’è un requisito (e perché non è “una frase a caso”)
Un requisito è una proprietà **richiesta** (o desiderabile) del sistema: descrive **cosa** deve essere realizzato, non *come lo implementerai* nel dettaglio.
Serve perché:
- diventa base per accordarsi con il cliente e non “capirsi a sentimento”;
- guida lo sviluppo e la divisione del lavoro;
- guida i test (“come faccio a verificare che funziona?”).

> Nota didattica: spesso i documenti distinguono tra **definizione dei requisiti** (cosa vuole l’utente/cliente) e **specifica dei requisiti** (come deve comportarsi il sistema in modo più tecnico e misurabile).

---

## 2) Regole d’oro (se ne ignorate una… poi vi rincorre)
Quando scrivete un requisito, controllate sempre questi punti:

- **Specificità:** deve essere chiaro e senza interpretazioni ambigue.
- **Completezza:** copre tutto ciò che serve per quella funzionalità, senza buchi.
- **Verificabilità:** deve esistere un modo pratico per dire “Sì/No, è rispettato”.
- **Consistenza:** non deve contraddire altri requisiti.
- **Priorità:** non tutto è “urgentissimo” allo stesso livello.
- **Tracciabilità:** deve essere possibile risalire a “chi l’ha chiesto” e a cosa è collegato.

Queste regole sono esattamente quelle che useremo come checklist durante la revisione dei vostri documenti.

---

## 3) Requisiti *utente* vs requisiti *di sistema* (stessa cosa, ma a due zoom diversi)
### Requisiti utente (zoom “cliente”)
- Linguaggio naturale.
- Comprensibili anche senza competenze tecniche.
- Esempio: “Voglio vedere sul telefono se la temperatura scende troppo.”

### Requisiti di sistema (zoom “tecnico”)
- Più dettagliati.
- Contengono vincoli, formati dati, tempi, soglie, errori.
- Esempio: “Il sistema invia un allarme push se T < 10°C per più di 30 minuti.”

  

👉 In pratica: **prima** scrivo la versione “utente”, **poi** la rendo misurabile e testabile nella versione “di sistema”.

  

---

  

## 4) Requisiti funzionali vs non funzionali (no, non è una cosa filosofica)
### Funzionali (RF)
Dicono **cosa fa** il sistema (servizi/funzionalità): acquisire dati, inviarli, salvarli, mostrarli, ecc.

### Non funzionali (RNF)
Dicono **come deve essere** il sistema (qualità/vincoli): sicurezza, affidabilità, prestazioni, autonomia, usabilità, portabilità, ecc.

💡 Trucchetto: se una frase contiene “deve essere *veloce/sicuro/robusto*”, di solito è **non funzionale**. Se contiene “deve *inviare/salvare/mostrare*”, è **funzionale**.

---

## 5) Template minimo per ogni requisito (copialo e usalo sempre)
Scegliete un formato standard e mantenetelo uguale per tutto il documento.

### Esempio di scheda requisito (in Markdown)
```md
### RF-DB-01 — Registrazione misure sensori
**Tipo:** Funzionale (RF)
**Descrizione (deve):** Il sistema deve salvare in database le misure di temperatura, umidità, peso e audio, associate a: idArnia, timestamp, qualità del segnale.
**Motivazione:** L’apicoltore deve consultare lo storico e confrontare periodi diversi.
**Priorità:** MUST / SHOULD / COULD
**Criteri di accettazione (test):**
- Dato un payload valido, quando invio la misura, allora la misura compare nello storico entro 2 secondi.
- Se manca `idArnia`, il server rifiuta con errore 400 e messaggio esplicativo.
**Fonte:** Intervista apicoltore / docente / team
**Dipendenze:** RF-API-02 (endpoint POST /measurements)
```

---

### Parole vietate (o quasi)
- “facile”, “molto”, “in tempo reale” (se non specifichi **quanto**), “a breve”, “sempre”, “costantemente” (senza numeri).
Meglio: **misura**, **frequenza**, **soglia**, **tempo massimo**, **formato**.

---

## 6) Da appunto “buttato giù” a requisito decente (mini esempio)
Ecco un esempio tipico di frase *troppo vaga*:

> “il sistema deve monitorare costantemente la temperatura all’interno dell’arnia…”

Diventa un requisito testabile così:

```md
### RF-HW-01 — Misura temperatura interna
**Tipo:** Funzionale (RF)
**Descrizione (deve):** Il device deve acquisire la temperatura interna dell’arnia ogni 60 secondi e associare a ogni misura un timestamp.
**Criteri di accettazione:**
- La misura viene registrata localmente anche senza rete.
- La misura viene inviata al server entro 5 minuti dal ripristino della connessione.
```
Perché è meglio? Perché ora sappiamo **quanto spesso**, **dove finisce il dato**, e **cosa succede se manca la rete**.

---

# 7) Requisiti per i 4 gruppi di lavoro

## Gruppo A — Database su restdb.io (modello dati + API)
> Obiettivo: strutturare i dati in modo che siano consultabili, confrontabili e “puliti”.

### Requisiti funzionali
- **RF-DB-01 — Anagrafiche**:
Il database deve gestire le seguenti entità: `Apiario`, `Arnia`, `Sensore`, `Rilevazione`, `Notifica` e `Utente`.
L’entità `Apiario` è collegata ad `Arnia` e rappresenta un insieme di arnie. Essa è caratterizzata dagli attributi id, nome, posizione e luogo. La posizione indica le coordinate geografiche, mentre il luogo è una stringa che rappresenta il nome del posto associato alle coordinate inserite.
L’entità `Arnia` è collegata sia ad `Apiario` sia a `Sensore` e contiene gli attributi id, data di installazione e piena.
L’entità `Sensore` è collegata sia ad`Arnia` sia a `Rilevazione` e presenta gli attributi id, stato (attivo o spento) e tipo, che serve a identificare l’unità di misura da associare al dato rilevato.
L’entità `Rilevazione` è collegata sia a `Sensore` sia a `Notifica` e include come attributi id, dato, data e ora.
Infine, l’entità `Notifica` è collegata a `Rilevazione` e possiede gli attributi id, titolo e descrizione.

- **RF-DB-02 — Tipo utente**: Devono esistere diversi tipi di utente: `Admin`, `Apicoltore`. L' `Admin` ha il compito di gestire il software, lui a differenza dell'apicoltore può gestire gli utenti che hanno fatto l'accesso all'interno del software, esso può leggere le informazioni nella tabella delle entità: `Apiari`, `Arnie`, `Sensori`, `Rilevazioni` e `Notifiche`. Invece può scrivere all'interno delle entità: `Apiari`, `Arnie` e `Sensori` ma con l'eccezione che sui sensori può solo modificare le soglie. L'`Apicoltore` può accedere al software e visualizzare i dati che servono per controllare le arnie, lui può leggere leggere le informazioni nella tabella delle entità: `Apiari`, `Arnie`, `Sensori`, `Rilevazioni` e `Notifiche`. Invece esso può scrivere all'interno delle entità: `Apiari`, `Arnie` e `Sensori` ma con l'eccezione che sui sensori può solo modificare le soglie. 
- **RF-DB-03 — Dati del database**: Il database deve ricevere i dati:
-- Rumore (dB) 
Misura il livello di rumore all’interno o vicino all'arnia. Serve a individuare attività anomale, presenza di predatori, sciami o stress della colonia.
-- Peso (kg)
Indica il peso complessivo dell’arnia. È un indicatore dello stato di produzione (miele, cera, covata) o di eventuale perdita di api.
-- Temperatura (°C)
Misura la temperatura interna dell’arnia. Valori anomali possono segnalare problemi nella ventilazione o malattie nella colonia.
-- Umidità (%)
Indica il livello di umidità interna. È cruciale per la conservazione del miele e il benessere delle api.
-- Entrata (bmp)
Misura il passaggio di api all’ingresso dell’arnia (contatore ottico o vibrazionale). Permette di valutare l’attività della colonia e il traffico giornaliero.
-- Livello acqua (%)
Indica la quantità d’acqua presente nel serbatoio utilizzato per il raffreddamento o l’alimentazione. Serve a garantire adeguata idratazione e a pianificare ricariche.
- **RF-DB-04 — Filtri e Statistiche**: Aggiungere filtri e statistiche varie.
- **RF-DB-05 — Apiari**: Gestire più apiari.
- **RF-DB-06 — Arnie**:  Collegare più arnie ad un apiario.

### Requisiti non funzionali
- **RNF-DB-01 — Retention**: Lo storico misure deve essere conservato per almeno 2 mesi.  
- **RNF-DB-02 — Misurazione**: Avere misurazioni su richiesta.
- **RNF-DB-03 — Notifica**: Identificare e segnalare problemi o misurazioni con una notifica.
- **RNF-DB-04 — Backup**: Backup automatici ogni settimana e successivamente fare pulizia.(evoluzione futura)
- **RNF-DB-05 — Salvataggio**: Salvare nel log gli accessi.(evoluzione futura)
- **RNF-DB-06 — Read/Write**: Il software deve permettere una differente abilità per ogni utente, solo un utente speciale può modificare e avere visione di tutte le arnie, tutti gli altri utenti devono essere abilitati.

📎 Media consigliati (da inserire nel vostro documento):
- Logo/icone Markdown (se fate una copertina): vedi risorsa “Markdown SVG”.
- Schema concettuale semplice REST (client → API → DB): vedi immagine “REST API basics”.

---

## Gruppo B — ESP32-CAM: progettazione hardware e sensori
> Obiettivo: definire quali sensori servono, dove metterli, e quali vincoli fisici ci sono (meteo, temperatura, acqua, urti…).

### Requisiti funzionali (esempi)
**[RF-HW-01 — Peso](DOCUMENTAZIONE%20HARDWARE.md#hx711-rf-hw-01)**: Il device deve misurare il peso dell’arnia per stimare andamento nettare/colonia.  
 - Nota dominio: il peso cresce con nettare e colonia, e diminuisce quando la colonia consuma miele o muore. 
 
**RF-HW-01 — Monitoraggio Peso e Mielometro** Il device deve misurare il peso totale dell'arnia con una risoluzione minima di 100g e calcolare il guadagno netto giornaliero (funzione mielometro).

### Requisiti funzionali (esempi)

**RF-HW-01 — Peso**: Il device deve misurare il peso dell’arnia per stimare andamento nettare/colonia.

- Nota dominio: il peso cresce con nettare e colonia, e diminuisce quando la colonia consuma miele o muore.

**[RF-HW-02 — Clima Interno (SHT21/BME280)](DOCUMENTAZIONE%20HARDWARE.md#sht21-rf-hw-02)** Il device deve misurare Temperatura e Umidità Relativa (RH) posizionato nella zona superiore (melario/coprifavo) e/o centrale (nido).

  

- _Nota dominio:_ L'umidità dell'aria interna è inversamente proporzionale alla maturazione del miele. Una RH interna > 65% costante indica rischio fermentazione o blocco dell'evaporazione. La temperatura del nido deve rimanere stabile a ~35°C per la covata.

**[RF-HW-03 — Analisi Acustica (Spettro)](DOCUMENTAZIONE%20HARDWARE.md#inmp441-rf-hw-03)** Il device deve campionare il rumore interno ed effettuare un'analisi in frequenza (FFT) per identificare picchi specifici tra 200Hz e 600Hz.

  

- _Nota dominio:_ Le api emettono frequenze specifiche per stati diversi: "Piping" (regine vergini pre-sciamatura, 400-500Hz), orfanità (bassa frequenza, "lamento"), e ventilazione notturna (asciugatura miele).

  

**RF-HW-04 — Livello Risorse Idriche** Il device deve misurare la distanza del pelo libero dell'acqua in un serbatoio esterno (secchio) e convertire il dato in percentuale residua in base ad una taratura fatta precedentemente in sui si da un valore max e min (es. max=20L, min=2L).

  

- _Nota dominio:_ Le api necessitano di molta acqua per termoregolare l'arnia d'estate. Se l'acqua finisce, la colonia rischia il collasso termico.

  

**RF-HW-05 — Monitoraggio Visivo (Foto Ingresso)** Il device deve scattare fotografie del predellino di volo (l'entrata dell'arnia) a intervalli regolari o quando richieste dal cliente, inviando l'immagine o un dato riassuntivo.

-   _Nota dominio_: Osservare l'ingresso permette di capire la salute della famiglia senza aprire l'arnia:
    

1.  "Barba" delle api: Se le api si ammucchiano fuori, fa troppo caldo o l'arnia è piena.
    
2.  Polline: I colori del polline sulle zampe indicano cosa stanno raccogliendo.
    
3.  Predatori: Si possono notare calabroni o vespe che attaccano la colonia.
    
  

----------

  

### Requisiti Non Funzionali (Vincoli e Qualità)

  

**RNF-HW-01 — Resistenza Ambientale Esterna (IP Rating)** I sensori esterni (Livello Acqua, Bilancia, Elettronica centrale) devono avere un grado di protezione minimo IP65 o IP67.

  

- _Nota dominio:_ L'attrezzatura è esposta a pioggia battente, raggi UV diretti, gelo invernale e fango.

  

**RNF-HW-02 — Resistenza Ambientale Interna (Anti-Propoli)** I sensori interni (SHT21, Microfono) devono essere dotati di protezioni fisiche (reti a maglia fine < 2mm) o filtri in PTFE.

  

- _Nota dominio:_ Le api "propolizzano" (coprono di resina) qualsiasi corpo estraneo per sterilizzarlo. Un sensore non protetto viene murato e reso inutilizzabile in < 48 ore.

  

**RNF-HW-03 — Resistenza Chimica (Acidi)** I materiali e i contatti esposti all'interno dell'arnia devono resistere alla corrosione da vapori acidi.

  

- _Nota dominio:_ Gli apicoltori usano regolarmente Acido Ossalico (sublimato o gocciolato) e Acido Formico per combattere la Varroa. Questi acidi ossidano rapidamente i contatti elettronici standard.

**RNF-HW-04 — Non Intrusività** L'installazione dei sensori non deve alterare i passaggi vitali (spazio d'ape ~8-9mm) né emettere vibrazioni o ultrasuoni nel range udibile dalle api.

  

- _Nota dominio:_ Le api comunicano tramite vibrazioni. Interferenze meccaniche o restringimenti dell'ingresso possono causare stress, aggressività o abbandono dell'arnia.
  

---

## Gruppo C — ESP32-CAM: logica software + comunicazione REST
> Obiettivo: definire **come** il device gestisce dati, errori, rete instabile, buffering, formato messaggi.

# Requisiti Funzionali

- **RF-SW-01 — TM, Temperatura Interna**: Il sensore misura la temperatura interna dell'arnia. 
- **RF-SW-02 — TM, Unità di Misura**: Il sensore misura la temperatura in gradi Celsius (°C).
- **RF-SW-03 — TM, Tipo di Dato**: Il valore del sensore è ospitato in una variabile di tipo float.
- **RF-SW-04 — TM, Timestamp**: Ad ogni misurazione è associata la data e l'ora di esecuzione letta dal DB.
- **RF-SW-05 — TM, Alert Soglia Massima**: Al superamento della temperatura di soglia del DB, il sistema invia un alert. 
- **RF-SW-06 — TM, Alert Soglia Minima**: Al raggiungimento della temperatura di soglia del DB, il sistema invia un alert.
- **RF-SW-07 — TM, Frequenza di Aggiornamento**: Il sensore aggiorna la misurazione ogni 6 minuti.
- **RF-SW-08 — PS, Misurazione**:  Il segnale analogico deve essere acquisito e convertito in valore digitale a 24 bit. 
- **RF-SW-09 — PS, Taratura**:  La funzione di taratura deve esistere per fare la differenza dei pesi.
- **RF-SW-10 — PS, Trasforma dato grezzo**: Il valore grezzo deve essere trasformato in kg.
- **RF-SW-11 — PS, Power Management**: La modalità deep sleep viene disattivata ogni 3 ore.
- **RF-SW-12 — PS, Gestione Data**: Ad ogni misurazione deve essere associata una data.
- **RF-SW-13 — PS, Gestione ora**: Ad ogni misurazione deve essere associata un orario. 
- **RF-SW-14 — MIC, Acquisizione continua**: Il microfono deve acquisire il segnale a intervalli di 10 secondi.
- **RF-SW-16 — MIC, Frequenza suono**: Si calcola la frequenza per avere una misura più precisa dello stato dello sciame.
- **RF-SW-17 — MIC, Verifica soglia**:  Il microfono confronta i valori rilevati con valori di allarme del DB.
- **RF-SW-18 — MIC, Notifica alert**: Se si supera una certa soglia l'apicoltore riceve una notifica.
- **RF-SW-18.5 -MIC, Time Stamp ogni minuto**, Si accumulano 6 misurazioni per poi inviarle insieme insieme al TimeStamp.
- **RF-SW-19 — UM, Misurazione umidità come % nell'aria**: Il sensore misura la percentuale di umidità all'interno dell'arnia.
- **RF-SW-20 — UM, Tipo di dato**: Il valore del sensore è ospitato in una variabile float. 
- **RF-SW-21 — UM, Timestamp**: Ad ogni misurazione è associata la data e l'ora di esecuzione del DB.
- **RF-SW-22 — UM, Alert Soglia Massima**: Al superamento della soglia del DB il sistema invia un alert. 
- **RF-SW-23 — UM, Alert Soglia Minima**: Al raggiungimento della soglia del DB il sistema invia un alert. 
- **RF-SW-24 — UM, Frequenza di Aggiornamento**: Il sensore aggiorna la misurazione ogni 6 minuti. 
- **RF-SW-25 — SA, Misurazione Livello Acqua**: Il sensore misura il livello dell'acqua nel secchio in percentuale.
- **RF-SW-26 — SA, Tipo di Dato**: Il valore del sensore è ospitato in una variabile di tipo float.
- **RF-SW-27 — SA, Timestamp**: Ad ogni misurazione è associata la data e l'ora di esecuzione di soglia del DB.
- **RF-SW-28 — SA, Alert Soglia Massima**: Al superamento della soglia del DB massima del secchio il sistema invia un alert. 
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

# Requisiti Non Funzionali

- **RNF-SW-01 — TM, Persistenza Dati**: Il sensore salva i dati nella memoria locale dell'ESP32 in caso di assenza di connessione
- **RNF-SW-04 — PS, Peso max**: L'arnia pesa al massimo 80kg. 
- **RNF-SW-05 — PS, Notifica peso alto**: L'apicoltore deve ricevere un alert se il peso è troppo alto. 
- **RNF-SW-06 — PS, Notifica peso basso**: L'apicoltore deve ricevere un alert se il peso è troppo basso.
- **RNF-SW-07 — PS, Latenza**: Il sensore impiega circa 2 secondi per misurare dopo l'accensione.
- **RNF-SW-08 — MIC, Tempestività alert**:  La notifica alert deve arrivare all'apicoltore in un tempo ragionevole.
- **RNF-SW-09 — UM, Persistenza Dati**: Il sensore salva i dati nella memoria locale dell'ESP32 in caso di assenza di connessione.
- **RNF-SW-13 — SA, Connettività**: Il sensore richiede connessione di rete costante per l'invio dei dati al database.
- **RNF-SW-19 — TC, Connettività**: La telecamera richiede connessione di rete costante per l'invio dei video al database.
  
---

## Gruppo D — Architettura di rete e infrastruttura (server, sicurezza, connettività)
> Obiettivo: definire come “parlano” tra loro device, server e interfaccia web (e con quali garanzie).


### Requisiti funzionali

**RF-NET-01 — Router dedicato all’apiario:**  
Deve essere utilizzato un router adeguato lte con sim all'interno di in una scatola(IP67/IP68), in grado di garantire stabilità di connessione.

**RF-NET-02 — Scelta del gestore di rete:**  
Il gestore di connettività deve essere selezionato in base alla copertura reale nella zona dell’apiario, verificata tramite test sul campo (es. test di segnale e velocità). Il router deve poter assegnare indirizzi IP ai dispositivi tramite DHCP o permettere IP statici opzionali.

**RF-NET-03 — Trasmissione dati automatica:**  
Il sistema deve trasmettere i dati raccolti dall’arnia al server senza intervento manuale dell’utente.
-  I dispositivi collocati nell’arnia ricevono un IP locale dal router.
-  Il router funge da gateway per comunicare con lo smartphone e il server.
-  La comunicazione deve garantire la consegna dei dati in tempo reale e la possibilità di controllo remoto dell’arnia tramite smartphone o interfaccia web.
    
**RF-NET-04 — Ridotta necessità di manutenzione:**  
L’architettura di rete deve richiedere il minimo intervento di manutenzione ordinaria da parte dell’apicoltore.

**RF-NET-05 — Ridondanza della comunicazione:**  
Il sistema deve garantire la trasmissione dei dati anche in caso di interruzione temporanea della connessione principale, tramite memorizzazione locale dei dati, assicurando la continuità del servizio.

**RF-NET-06 — Controllo priorità dei dati:**  
Il sistema deve gestire la priorità dei dati trasmessi, dando precedenza ai dati critici (es. allarmi di temperatura o umidità dell’arnia) rispetto ai dati di monitoraggio ordinari, garantendo che le informazioni più importanti arrivino tempestivamente anche in condizioni di rete limitata.

----------

### Requisiti non funzionali

**RNF-NET-01 — Alimentazione autonoma:**  
Il sistema di rete (router e dispositivi) deve essere alimentato tramite batteria integrata, protetta da un box resistente agli agenti atmosferici.

**RNF-NET-02 — Energia rinnovabile:**  
Deve essere previsto un pannello solare posizionato in modo strategico per massimizzare l’esposizione alla luce solare e garantire la ricarica continua della batteria.

**RNF-NET-03 — Continuità operativa senza rete elettrica:**  
In assenza di alimentazione elettrica, il router e i dispositivi devono continuare a funzionare.

**RNF-NET-04 — Efficienza energetica:**  
Il consumo energetico mensile dell’intero sistema deve essere minimo e compatibile con l’alimentazione solare prevista.

**RNF-NET-05 — Resistenza ambientale:**  
I componenti devono operare correttamente in presenza di variazioni di temperatura e umidità, riducendo il rischio di ossidazione e degrado dei materiali.

**RNF-NET-06 — Calcolo e monitoraggio dei consumi energetici:**  
Deve essere effettuato un calcolo preventivo del consumo energetico complessivo del sistema per garantire l’autonomia richiesta e il corretto dimensionamento di batteria e pannello solare.

**RNF-NET-07 — Gestione indirizzi IP e rete:**  
Il sistema deve supportare configurazioni IP dinamiche (DHCP) e statiche per tutti i dispositivi, garantendo la comunicazione sicura e affidabile tra arnia, router, smartphone e server cloud. Deve permettere il monitoraggio remoto degli IP e dei dispositivi connessi, assicurando tracciabilità e continuità della trasmissione dati.

**RNF-NET-08 — Calcolo consumo dati per scelta del gestore:**  
Prima di selezionare l’offerta del gestore di rete, deve essere effettuato un calcolo del consumo dati previsto dal sistema per scegliere l’offerta più adatta ed evitare costi inutili.---

# 8) Mini nota: cos’è un file Markdown (.md) e perché lo usiamo
**Markdown** è un modo semplice per scrivere documenti usando solo testo, ma con una sintassi leggera per titoli, elenchi, grassetti e blocchi di codice.  
È perfetto per GitHub e per consegne “pulite” in cui versionare modifiche e lavorare in gruppo

### Mini cheat sheet Markdown

```md
# Titolo
## Sezione
**grassetto** *corsivo*

- elenco puntato
1. elenco numerato

`codice in linea`
```

```md
| Colonna | Valore |
|--------:|:------ |
| id | RF-01 |

```

---

# 9) Consegna consigliata (struttura del vostro file requirements.md)
1. **Introduzione** (scopo + contesto del progetto)
2. **Glossario** (arnia, alveare, melario, ecc.)
3. **Stakeholder** (apicoltore, docenti, “utente smartphone”, manutentore…)
4. **Requisiti utente** (pochi, chiari)
5. **Requisiti di sistema** (molti, testabili)
6. **Casi d’uso / scenari** (almeno 2: uno normale, uno con errore)
7. **Requisiti non funzionali** (sicurezza, affidabilità, prestazioni…)
8. **Tracciabilità** (tabellina requisito → fonte → test → componente)

Buon lavoro: i requisiti non sono “burocrazia”, sono l’unico modo per non litigare con il futuro voi stessi. 😄
