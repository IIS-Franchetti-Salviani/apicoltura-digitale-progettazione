Istruzioni:
ogni json è la risposta ad una get ma per effettuare la post è necessario costruire un payload con le sole chiavi del json senza underscore (che sono dati in sola lettura)
base_path: https://clone7-b263.restdb.io/rest/

Risorse:
/apiari/
{
  "_id": "696f6a75c4ff4849000000d2",
  "api_id": 6,
  "api_luogo": "Voc. Rotetino, Monte Santa Maria Tiberina",
  "api_nome": "Apicoltura Santucci - apiario principale",
  "_created": "2026-01-20T11:43:49.878Z",
  "_changed": "2026-01-20T11:43:49.878Z",
  "_createdby": "francesco.adriani@franchettisalviani.net",
  "_changedby": "francesco.adriani@franchettisalviani.net",
  "_version": 0,
  "api_lon": 12.203588,
  "api_lat": 43.385117
}

/arnie/
{
  "_id": "6970ac55c4ff484900000562",
  "arn_dataInst": "2026-01-20T00:00:00.000Z",
  "arn_piena": true,
  "arn_MacAddress": "FB:3F:18:47:FC:3F",
  "arn_id": 5,
  "arn_api_id": 6,
  "_created": "2026-01-21T10:37:09.981Z",
  "_changed": "2026-01-21T10:37:09.981Z",
  "_createdby": "massimialunni.marco@franchettisalviani.net",
  "_changedby": "massimialunni.marco@franchettisalviani.net",
  "_version": 0
}

/sensoriarnia/
{
  "_id": "696f6aacc4ff4849000000d7",
  "sea_tip_id": 11,
  "sea_stato": true,
  "sea_max": 80,
  "sea_min": 46,
  "sea_id": 0,
  "sea_arn_id": 6,
  "_created": "2026-01-20T11:44:44.275Z",
  "_changed": "2026-01-30T08:00:29.002Z",
  "_createdby": "francesco.adriani@franchettisalviani.net",
  "_changedby": "api",
  "_version": 17,
  "sea_attivo": true,
  "sea_on": true
}

/sensori/
{
  "_id": "696e1a65c4ff484900000061",
  "sen_modello": "DS18B20",
  "sen_id": 3,
  "_created": "2026-01-19T11:49:57.242Z",
  "_changed": "2026-01-19T11:49:57.242Z",
  "_createdby": "bambagiotti.andrea@franchettisalviani.net",
  "_changedby": "bambagiotti.andrea@franchettisalviani.net",
  "_version": 0
}

/rilevazioni/
{
  "_id": "696f6cb7c4ff4849000000e4",
  "ril_id": 4,
  "ril_dato": 50,
  "ril_dataOra": "2026-01-20T12:05:00.000Z",
  "ril_sea_id": 0,
  "_created": "2026-01-20T11:53:27.936Z",
  "_changed": "2026-01-22T07:37:47.382Z",
  "_createdby": "francesco.adriani@franchettisalviani.net",
  "_changedby": "massimialunni.marco@franchettisalviani.net",
  "_version": 1,
  "_recent_changed": false
}

/tipirilevazione/
{
  "_id": "6968c5ba86d69d6c00005cd6",
  "tip_id": 11,
  "_created": "2026-01-15T10:47:22.272Z",
  "_changed": "2026-01-22T07:44:06.228Z",
  "_createdby": "ionescu.giuliamaria@franchettisalviani.net",
  "_changedby": "massimialunni.marco@franchettisalviani.net",
  "_version": 1,
  "tip_tipologia": "Peso",
  "tip_sen_id": 0,
  "tip_unita": "Kg"
}

/notifiche/
{
  "_id": "697322afc4ff4849000013b6",
  "not_id": 0,
  "not_titolo": "PESO TROPPO ELEVATO",
  "not_dex": "ALLARME: PESO TROPPO ELEVATO 50Kg",
  "not_ril_id": 4,
  "_created": "2026-01-23T07:26:39.515Z",
  "_changed": "2026-01-23T07:27:58.706Z",
  "_createdby": "camilli.cristiano@franchettisalviani.net",
  "_changedby": "camilli.cristiano@franchettisalviani.net",
  "_version": 2
}

