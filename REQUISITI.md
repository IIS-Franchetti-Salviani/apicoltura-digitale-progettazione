
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

### Requisiti funzionali (esempi)
- **RF-DB-01 — Anagrafiche**: Il database deve gestire le entità: `Apiario`, `Arnia`, `Sensore`, `Misura`, `Allarme`, `Utente`.  

### Requisiti non funzionali (esempi)
- **RNF-DB-01 — Retention**: Lo storico misure deve essere conservato per almeno 12 mesi.  

📎 Media consigliati (da inserire nel vostro documento):
- Logo/icone Markdown (se fate una copertina): vedi risorsa “Markdown SVG”.
- Schema concettuale semplice REST (client → API → DB): vedi immagine “REST API basics”.

---

## Gruppo B — ESP32-CAM: progettazione hardware e sensori
> Obiettivo: definire quali sensori servono, dove metterli, e quali vincoli fisici ci sono (meteo, temperatura, acqua, urti…).

### Requisiti funzionali (esempi)
**RF-HW-01 — Peso**: Il device deve misurare il peso dell’arnia per stimare andamento nettare/colonia.  
 - Nota dominio: il peso cresce con nettare e colonia, e diminuisce quando la colonia consuma miele o muore. 
 
**RF-HW-01 — Monitoraggio Peso e Mielometro** Il device deve misurare il peso totale dell'arnia con una risoluzione minima di 100g e calcolare il guadagno netto giornaliero (funzione mielometro).

 - Nota dominio: Il peso fluttua durante il giorno (le api escono a bottinare); la misura valida per il mielometro è la differenza di peso tra due notti consecutive (a bottinatrici rientrate e miele parzialmente deumidificato).
    

**RF-HW-02 — Clima Interno (SHT21/BME280)** Il device deve misurare Temperatura e Umidità Relativa (RH) posizionato nella zona superiore (melario/coprifavo) e/o centrale (nido).

 - Nota dominio:_ L'umidità dell'aria interna è inversamente proporzionale alla maturazione del miele. Una RH interna > 65% costante indica rischio fermentazione o blocco dell'evaporazione. La temperatura del nido deve rimanere stabile a ~35°C per la covata.
    

**RF-HW-03 — Analisi Acustica (Spettro)** Il device deve campionare il rumore interno ed effettuare un'analisi in frequenza (FFT) per identificare picchi specifici tra 200Hz e 600Hz.

-   _Nota dominio:_ Le api emettono frequenze specifiche per stati diversi: "Piping" (regine vergini pre-sciamatura, 400-500Hz), orfanità (bassa frequenza, "lamento"), e ventilazione notturna (asciugatura miele).
    

**RF-HW-04 — Conteggio Flussi (Ingresso/Uscita)** Il device deve contare distintamente il numero di api in uscita e il numero di api in entrata dal predellino di volo.

-   _Nota dominio:_ Un saldo negativo (Uscite > Entrate) prolungato indica moria nei campi (pesticidi) o predatori. Un traffico intenso in entrata indica forte importazione di nettare.
    

**RF-HW-05 — Livello Risorse Idriche** Il device deve misurare la distanza del pelo libero dell'acqua in un serbatoio esterno (secchio) e convertire il dato in percentuale residua.

-   _Nota dominio:_ Le api necessitano di molta acqua per termoregolare l'arnia d'estate. Se l'acqua finisce, la colonia rischia il collasso termico.
    

----------

### Requisiti Non Funzionali (Vincoli e Qualità)

**RNF-HW-01 — Resistenza Ambientale Esterna (IP Rating)** I sensori esterni (Livello Acqua, Bilancia, Elettronica centrale) devono avere un grado di protezione minimo IP65 o IP67.

-   _Nota dominio:_ L'attrezzatura è esposta a pioggia battente, raggi UV diretti, gelo invernale e fango.
    

**RNF-HW-02 — Resistenza Ambientale Interna (Anti-Propoli)** I sensori interni (SHT21, Microfono) devono essere dotati di protezioni fisiche (reti a maglia fine < 2mm) o filtri in PTFE.

-   _Nota dominio:_ Le api "propolizzano" (coprono di resina) qualsiasi corpo estraneo per sterilizzarlo. Un sensore non protetto viene murato e reso inutilizzabile in < 48 ore.
    

**RNF-HW-03 — Resistenza Chimica (Acidi)** I materiali e i contatti esposti all'interno dell'arnia devono resistere alla corrosione da vapori acidi.

-   _Nota dominio:_ Gli apicoltori usano regolarmente Acido Ossalico (sublimato o gocciolato) e Acido Formico per combattere la Varroa. Questi acidi ossidano rapidamente i contatti elettronici standard.
    

**RNF-HW-04 — Basso Consumo (Low Power)** Il sistema deve garantire un'autonomia operativa di almeno 6 mesi senza ricarica o manutenzione (o supportare ricarica solare).

-   _Nota dominio:_ Gli apiari sono spesso situati in zone remote ("nomadismo") prive di rete elettrica.
    

**RNF-HW-05 — Non Intrusività** L'installazione dei sensori non deve alterare i passaggi vitali (spazio d'ape ~8-9mm) né emettere vibrazioni o ultrasuoni nel range udibile dalle api.

-   _Nota dominio:_ Le api comunicano tramite vibrazioni. Interferenze meccaniche o restringimenti dell'ingresso possono causare stress, aggressività o abbandono dell'arnia.
    

**RNF-HW-06 — Stabilità Termica (Celle di Carico)** Il sistema di pesatura deve includere un algoritmo di compensazione termica o hardware specifico a basso drift.

-   _Nota dominio:_ Le bilance elettroniche soffrono molto la dilatazione termica. Una variazione di 10°C tra giorno e notte può falsare la lettura di centinaia di grammi se non compensata, rendendo inutile la stima del miele raccolto. 


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