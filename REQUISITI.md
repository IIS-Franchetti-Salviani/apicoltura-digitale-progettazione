


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

# 7) Esempi pronti per i 4 gruppi di lavoro

## Gruppo A — Database su restdb.io (modello dati + API)
> Obiettivo: strutturare i dati in modo che siano consultabili, confrontabili e “puliti”.

### Requisiti funzionali
- **RF-DB-01 — Anagrafiche**: Ecco una versione riscritta in **italiano corretto e più chiaro**:

----------

Il database deve gestire le seguenti entità: `Apiario`, `Arnia`, `Sensore`, `Rilevazione`, `Notifica` e `Utente`.
L’entità `Apiario` è collegata ad `Arnia` e rappresenta un insieme di arnie. Essa è caratterizzata dagli attributi id, nome, posizione e luogo. La posizione indica le coordinate geografiche, mentre il luogo è una stringa che rappresenta il nome del posto associato alle coordinate inserite.
L’entità `Arnia` è collegata sia ad `Apiario` sia a `Sensore` e contiene gli attributi id, data di installazione e piena.
L’entità `Sensore` è collegata sia ad`Arnia` sia a `Rilevazione` e presenta gli attributi id, stato (attivo o spento) e tipo, che serve a identificare l’unità di misura da associare al dato rilevato.
L’entità `Rilevazione` è collegata sia a `Sensore` sia a `Notifica` e include come attributi id, dato, data e ora.
Infine, l’entità `Notifica` è collegata a `Rilevazione` e possiede gli attributi id, titolo e descrizione.

- **RF-DB-02 — Tipo utente**: Devono esistere diversi tipi di utente: `Admin`, `Apicoltore`. L' `Admin` ha il compito di gestire il software, lui a differenza dell'apicoltore può gestire gli utenti che hanno fatto l'accesso all'interno del software, esso può leggere le informazioni nella tabella delle entità: `Apiari`, `Arnie`, `Sensori`, `Rilevazioni` e `Notifiche`. Invece può scrivere all'interno delle entità: `Apiari`, `Arnie` e `Sensori` ma con l'eccezione che sui sensori può solo modificare le soglie. L'`Apicoltore` può accedere al software e visualizzare i dati che servono per controllare le arnie, lui può leggere leggere le informazioni nella tabella delle entità: `Apiari`, `Arnie`, `Sensori`, `Rilevazioni` e `Notifiche`. Invece esso può scrivere all'interno delle entità: `Apiari`, `Arnie` e `Sensori` ma con l'eccezione che sui sensori può solo modificare le soglie. 
- **RF-DB-03 — Dati del database**: Il database deve ricevere i dati:
-- Rumore (dB)
-- Peso (kg)
-- Temperatura (°C)
-- Umidità (%)
-- Entrata (bmp)
-- Livello acqua (%)
- **RF-DB-04 — Filtri e Statistiche**: Aggiungere filtri e statistiche varie.
- **RF-DB-05 — Apiari**: Il database deve supportare la gestione di più apiari, intesi come insiemi di arnie e colonie di api, che saranno amministrabili tramite l’applicazione.
Ogni apiario deve essere identificato univocamente da un ID e deve contenere le seguenti informazioni:
--Nome della località in cui è situato
--Coordinate geografiche (latitudine e longitudine)
L’applicazione deve consentire la creazione, visualizzazione e gestione degli apiari presenti nel database.
Per ciascun apiario deve essere possibile aggiungere, gestire e associare più arnie.
Il sistema deve inoltre permettere l’inserimento di nuovi apiari in qualsiasi momento.
- **RF-DB-06 — Arnie**:  Collegare più arnie ad un apiario.

### Requisiti non funzionali
- **RNF-DB-01 — Retention**: Lo storico misure deve essere conservato per almeno 2 mesi.  
- **RNF-DB-02 — Misurazione**: Avere misurazioni su richiesta.
- **RNF-DB-03 — Notifica**: Identificare e segnalare problemi o misurazioni con una notifica:
	- Ogni volta che un sensore determina un valore viene inviato al server, che, in caso di anomalie con le soglie 				di massimo e di minimo dei campi prestabiliti (temperatura, umidità, ecc.) crea un'apposita notifica caratterizzata dai seguenti attributi: **ID, Titolo, Descrizione, Valore, Arnia, Apiario**.
	- Dopodichè la notifica verrà inviata al dispositivo dell'utente, che verrà visualizzata con un alert del tipo: **"La temperatura dell'arnia 'X' appartenente all'apiario 'Y' ha SUPERATO la soglia massima!"**.- **RNF-DB-04 — Backup**: Backup automatici ogni settimana e successivamente fare pulizia.(evoluzione futura)
- **RNF-DB-05 — Salvataggio**: Salvare nel log gli accessi.(evoluzione futura)

📎 Media consigliati (da inserire nel vostro documento):
- Logo/icone Markdown (se fate una copertina): vedi risorsa “Markdown SVG”.
- Schema concettuale semplice REST (client → API → DB): vedi immagine “REST API basics”.

---

## Gruppo B — ESP32-CAM: progettazione hardware e sensori
> Obiettivo: definire quali sensori servono, dove metterli, e quali vincoli fisici ci sono (meteo, temperatura, acqua, urti…).

### Requisiti funzionali (esempi)
- **RF-HW-01 — Peso**: Il device deve misurare il peso dell’arnia per stimare andamento nettare/colonia.  
  - Nota dominio: il peso cresce con nettare e colonia, e diminuisce quando la colonia consuma miele o muore.  

### Requisiti non funzionali (esempi)
- **RNF-HW-01 — Resistenza meteo**: Il device deve resistere a pioggia, umidità e sbalzi termici.  

📎 Media consigliati:
- Foto del modulo ESP32-CAM (per descrivere connettori e ingombri).
- Foto/diagramma di un modulo peso con HX711 + cella di carico.
- Immagine di un sensore umidità/temperatura (es. DHT22) come riferimento visivo.
- Diagramma componenti arnia (per “posizionamento sensori”).

---

## Gruppo C — ESP32-CAM: logica software + comunicazione REST
> Obiettivo: definire **come** il device gestisce dati, errori, rete instabile, buffering, formato messaggi.

### Requisiti funzionali (esempi)
- **RF-SW-01 — Ciclo acquisizione**: Il firmware deve acquisire le misure con una frequenza configurabile (es. temperatura ogni 60s; peso ogni 5 min; audio come “indicatori” aggregati).  

### Requisiti non funzionali (esempi)
- **RNF-SW-01 — Robustezza**: In caso di crash, il device deve ripartire automaticamente e non perdere più delle ultime N misure in buffer.  

📎 Media consigliati:
- Diagramma “REST API model” (per spiegare GET/POST e flusso dati).
- Schema a blocchi: sensori → firmware → rete → server → database.

---

## Gruppo D — Architettura di rete e infrastruttura (server, sicurezza, connettività)
> Obiettivo: definire come “parlano” tra loro device, server e interfaccia web (e con quali garanzie).

### Requisiti funzionali (esempi)
- **RF-NET-01 — Connettività device**: Il device deve potersi connettere alla rete dell’apiario (Wi‑Fi locale o hotspot dedicato) per raggiungere il server REST.  

### Requisiti non funzionali (esempi)
- **RNF-NET-01 — Disponibilità**: Il servizio deve essere disponibile in orario di lavoro dell’apicoltore (definire fascia e downtime massimo).  

📎 Media consigliati:
- Un semplice diagramma rete (apiario → internet → VPS/server → smartphone).

---

# 8) Mini nota: cos’è un file Markdown (.md) e perché lo usiamo
**Markdown** è un modo semplice per scrivere documenti usando solo testo, ma con una sintassi leggera per titoli, elenchi, grassetti e blocchi di codice.  
È perfetto per GitHub e per consegne “pulite” in cui versionare modifiche e lavorare in gruppo.

### Mini cheat sheet Markdown
```md
# Titolo
## Sezione
**grassetto**  *corsivo*

- elenco puntato
1. elenco numerato

`codice in linea`
```

```md
| Colonna | Valore |
|--------:|:------ |
| id      | RF-01  |
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
