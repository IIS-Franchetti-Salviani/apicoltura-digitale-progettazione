# 🐝 Apicoltura Digitale — Project Work (Classi Quinte Informatica)

Benvenuti nel repository del project work **Apicoltura Digitale** (a.s. 2025/2026).  
L’obiettivo è progettare e sviluppare un sistema **IoT** che raccolga dati dall’arnia e li renda disponibili tramite **server REST**, **database** e **interfaccia web mobile**, con una progettazione completa anche di **rete/infrastruttura**.

---

## Link rapidi
- 📌 **Requisiti (documento principale)**: [REQUISITI.md](./REQUISITI.md)
- 🧭 **Pagina di coordinamento (Notion)**: https://www.notion.so/21260f946e0180c297aacd449b8628fa?v=21260f946e0181a48fa2000c76b52c32&source=copy_link
- 📚 **Guida pratica a Git (commit / push / branch)**: [Guida pratica a Git.pdf](./docs/Guida_pratica_a_Git.pdf) 

---

## Descrizione del progetto (in breve)
Il progetto prevede la realizzazione di un sistema composto da:
1. **Dispositivo IoT (ESP32-CAM + sensori)** per acquisire dati (es. temperatura, umidità, peso, audio, eventi) e inviarli al server.
2. **Server REST in PHP** (sviluppo “nativo”, senza piattaforme) per ricevere i dati e renderli consultabili.
3. **Database MySQL** per memorizzare misure, parametri e storico.
4. **Infrastruttura di rete** per collegare arnie/device e server in modo affidabile e sicuro.
5. **Web app mobile (smartphone)** per visualizzare dati e configurare parametri di registrazione.

> Il progetto si integra con le materie di indirizzo (Informatica, Sistemi e Reti, TPSIT, Gestione Progetto).

---

## Gruppi di lavoro (4 aree)
Ogni team lavora su una parte, ma tutte devono “incastrarsi” bene (come un’arnia fatta bene, senza spifferi 😄).

### Gruppo A — Database (restdb.io / modello dati)
- Modellazione entità e relazioni (Arnia, Sensori, Misure, Allarmi, Utenti…)
- Query/filtri per storico e dashboard
- Esportazioni/backup dati (se richiesto)

### Gruppo B — Hardware (ESP32-CAM + sensori)
- Scelta sensori e vincoli fisici (alimentazione, meteo, installazione)
- Schema collegamenti e posizionamento sensori
- Considerazioni di robustezza/manutenzione

### Gruppo C — Firmware + comunicazione REST
- Ciclo di acquisizione misure, buffer locale, gestione rete instabile
- Formato JSON e chiamate REST (POST misure, GET parametri…)
- Sicurezza minima: autenticazione verso API, gestione credenziali, log eventi

### Gruppo D — Rete e infrastruttura
- Topologia (device → rete → server → web app)
- Indirizzamento, DNS/HTTPS (se previsto), segmentazione rete
- Logging e monitoraggio base (es. “device offline da X ore”)

---

## Come lavorare sul repository (commit e push)
Questa repo non è “un compito da consegnare alla fine”: è un progetto che cresce a step.
Quindi Git si usa **spesso**, ma bene.

### Regole d’oro (da seguire sempre)
1. **Commit solo se compila / gira**  
2. **Un branch per gruppo (per area di competenza)** (es. `feature/network`, `feature/schema-db`)  
3. **Merge solo quando è completo** (niente debug temporanei)  
4. **Un Push per funzionalità e solo se funziona** (state “mettendo roba in casa degli altri”)  

### Workflow consigliato (feature branch)
1. Aggiorna il tuo progetto:
   ```bash
   git pull
   ```
2. Crea un branch dedicato:
   ```bash
   git checkout -b feature/nome-funzionalita
   ```
3. Lavora, poi aggiungi le modifiche:
   ```bash
   git add .
   ```
4. Commit con messaggio fatto bene:
   ```bash
   git commit -m "Aggiunto endpoint /misure con validazione JSON"
   ```
5. Push del branch:
   ```bash
   git push -u origin feature/nome-funzionalita
   ```

### Come scrivere un commit “serio”
**Titolo breve**, specifico, in forma impersonale e in participio passato.  
Esempi buoni:
- `Aggiunto endpoint /status con JSON`
- `Corretto parsing payload misure`
- `Implementato modello Arnia e Sensore`

Da evitare:
- `fix`
- `modifiche`
- `aggiornamento`

Se il commit è importante, aggiungete una descrizione più completa:
- Problema / esigenza
- Soluzione
- Test effettuati
- Note / rischi / TODO

---

## Struttura consigliata della repository
(Se il repository è già impostato diversamente, adattatevi senza fare “guerra di religione” 😄)

```
/
├─ REQUISITI.md
├─ README.md
├─ docs/
│  ├─ Guida_pratica_a_Git.pdf
│  ├─ diagrammi/
│  └─ immagini/
├─ db/
│  ├─ schema.sql
│  └─ seed.sql
├─ firmware/
│  └─ esp32cam/
└─ webapp/
   └─ mobile-dashboard/
```

---

## Consegne (minime)
- **REQUISITI.md** completo e aggiornato (con criteri di accettazione/test)
- Documentazione tecnica essenziale (diagrammi, scelte progettuali, formati dati)
- Codice (API, firmware, web) con commit comprensibili e tracciabili
- Coordinamento aggiornato su Notion (task, decisioni, problemi aperti)

---

## Note finali
Se un requisito non è testabile, non è un requisito: è un desiderio.  
Se un commit non dice cosa cambia, è un rebus.  
E se tutto è urgente… niente lo è. 😉
