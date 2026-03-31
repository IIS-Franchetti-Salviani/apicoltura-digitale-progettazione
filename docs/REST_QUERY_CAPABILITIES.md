# REST Query Capabilities

Questa guida descrive le opzioni di filtro/paginazione/selezione implementate globalmente nel layer REST PHP.

## Architettura

La logica è centralizzata in:
- `src/rest/config.php`
  - policy esposizione risorse: `$restResourceExposure`
  - parser query: `restGetCollectionQuery(...)`
  - output standard: `restSendCollectionResponse(...)`

Le `GET` lista degli endpoint usano questo layer:
- `apiari`, `arnie`, `sensori`, `tipirilevazione`, `sensoriarnia`, `rilevazioni`, `notifiche`, `utenti`, `configurazioni`

## Parametri supportati

| Parametro | Descrizione | Esempio |
|---|---|---|
| `q` | Filtro JSON | `q={"api_nome":{"$regex":"Santucci"}}` |
| `h.$fields` | Selezione campi | `h={"$fields":{"arn_id":1,"arn_MacAddress":1}}` |
| `h.$orderby` | Ordinamento JSON | `h={"$orderby":{"ril_dataOra":-1}}` |
| `sort` + `dir` | Ordinamento semplice | `sort=not_id&dir=-1` |
| `skip` | Offset | `skip=100` |
| `max` | Limite record | `max=50` |
| `totals=true` | Include metadati conteggio | `totals=true` |
| `count=true` | Solo conteggio (`data` vuoto) | `totals=true&count=true` |
| `filter` | Ricerca testuale sui campi `search_fields` della risorsa | `filter=arnia` |

Operatori `q` supportati:
- `$and`, `$or`, `$gt`, `$gte`, `$lt`, `$lte`, `$in`, `$nin`, `$regex`, `$exists`, `$not`, `$bt`

## Shape risposta

Senza `totals=true`:
```json
[
  { "id": 1 },
  { "id": 2 }
]
```

Con `totals=true`:
```json
{
  "data": [{ "id": 1 }, { "id": 2 }],
  "totals": { "count": 123, "skip": 0, "max": 20 }
}
```

Con `totals=true&count=true`:
```json
{
  "data": [],
  "totals": { "count": 123 }
}
```

## Esempi curl (con x-apikey)

Imposta variabili:
```bash
BASE_URL="https://<host>/rest"
API_KEY="<API_KEY>"
```

Apiari:
```bash
curl -G "$BASE_URL/apiari" \
  -H "x-apikey: $API_KEY" \
  --data-urlencode 'q={"api_nome":{"$regex":"Santucci"}}' \
  --data-urlencode 'max=10' \
  --data-urlencode 'totals=true'
```

Arnie:
```bash
curl -G "$BASE_URL/arnie" \
  -H "x-apikey: $API_KEY" \
  --data-urlencode 'q={"arn_api_id":1}' \
  --data-urlencode 'h={"$fields":{"arn_id":1,"arn_MacAddress":1}}'
```

Sensori:
```bash
curl -G "$BASE_URL/sensori" \
  -H "x-apikey: $API_KEY" \
  --data-urlencode 'filter=DS18B20'
```

Tipi rilevazione:
```bash
curl -G "$BASE_URL/tipirilevazione" \
  -H "x-apikey: $API_KEY" \
  --data-urlencode 'q={"tip_codice":{"$in":["ds18b20","hx711"]}}'
```

SensoriArnia:
```bash
curl -G "$BASE_URL/sensoriarnia" \
  -H "x-apikey: $API_KEY" \
  --data-urlencode 'q={"sea_stato":1}' \
  --data-urlencode 'sort=sea_id' \
  --data-urlencode 'dir=-1'
```

Rilevazioni:
```bash
curl -G "$BASE_URL/rilevazioni" \
  -H "x-apikey: $API_KEY" \
  --data-urlencode 'q={"ril_dato":{"$gt":30}}' \
  --data-urlencode 'h={"$orderby":{"ril_dataOra":-1}}' \
  --data-urlencode 'skip=0' \
  --data-urlencode 'max=20'
```

Notifiche:
```bash
curl -G "$BASE_URL/notifiche" \
  -H "x-apikey: $API_KEY" \
  --data-urlencode 'q={"not_livello":{"$gte":2}}' \
  --data-urlencode 'totals=true&count=true'
```

Utenti:
```bash
curl -G "$BASE_URL/utenti" \
  -H "x-apikey: $API_KEY" \
  --data-urlencode 'h={"$fields":{"ute_id":1,"ute_username":1,"ute_admin":1}}'
```

Configurazioni:
```bash
curl -G "$BASE_URL/configurazioni" \
  -H "x-apikey: $API_KEY" \
  --data-urlencode 'q={"cfs_macAddress":{"$regex":"FB:"}}'
```

Compatibilità firmware ESP (`/configurazioni`):
```bash
curl -G "$BASE_URL/configurazioni" \
  -H "x-apikey: $API_KEY" \
  --data-urlencode 'q={"macAddress":"FB:3F:18:47:FC:3F"}'
```
