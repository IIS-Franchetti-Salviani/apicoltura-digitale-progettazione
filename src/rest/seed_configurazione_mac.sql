-- ============================================================================
-- Seed configurazione dispositivo ESP32-CAM per MAC address reale
-- Coerente con default firmware in src/esp/connection_manager.ino
-- e con cablaggio in src/esp/SCHEMA_PIN_ESP32CAM.md
-- ============================================================================
--
-- Uso:
-- 1) Sostituisci @MAC_REALE con il MAC letto da seriale (es. 24:6F:28:AA:BB:CC)
-- 2) Imposta @ARN_ID sull'arnia da associare
-- 3) Esegui lo script (idempotente: aggiorna se record gia' esistono)
--

SET @MAC_REALE = '78:1C:3C:F6:8B:64';
SET @ARN_ID = 1;
SET @API_ID = 1;

-- Apiario/Arnia di appoggio (se gia' esistono, vengono aggiornati)
INSERT INTO Apiario (api_id, api_nome, api_luogo, api_lon, api_lat)
VALUES (@API_ID, 'Apiario Principale', 'Monte Santa Maria Tiberina', 12.203588, 43.385117)
ON DUPLICATE KEY UPDATE
    api_nome = VALUES(api_nome),
    api_luogo = VALUES(api_luogo),
    api_lon = VALUES(api_lon),
    api_lat = VALUES(api_lat);

INSERT INTO Arnia (arn_id, arn_api_id, arn_nome, arn_dataInst, arn_piena, arn_MacAddress, arn_attiva)
VALUES (@ARN_ID, @API_ID, CONCAT('Arnia ', LPAD(@ARN_ID, 2, '0')), CURDATE(), 0, @MAC_REALE, 1)
ON DUPLICATE KEY UPDATE
    arn_api_id = VALUES(arn_api_id),
    arn_nome = VALUES(arn_nome),
    arn_MacAddress = VALUES(arn_MacAddress),
    arn_attiva = VALUES(arn_attiva);

-- Catalogo sensori minimo richiesto dal firmware
INSERT INTO Sensore (sen_modello, sen_codice, sen_produttore)
VALUES ('DS18B20', 'DS18B20', 'Maxim')
ON DUPLICATE KEY UPDATE sen_modello = VALUES(sen_modello), sen_produttore = VALUES(sen_produttore);

INSERT INTO Sensore (sen_modello, sen_codice, sen_produttore)
VALUES ('HTU21D', 'HTU21D', 'TE Connectivity')
ON DUPLICATE KEY UPDATE sen_modello = VALUES(sen_modello), sen_produttore = VALUES(sen_produttore);

INSERT INTO Sensore (sen_modello, sen_codice, sen_produttore)
VALUES ('HX711', 'HX711', 'Avia')
ON DUPLICATE KEY UPDATE sen_modello = VALUES(sen_modello), sen_produttore = VALUES(sen_produttore);

-- Tipi rilevazione usati dal firmware
INSERT INTO TipoRilevazione (tip_tipologia, tip_codice, tip_sen_id, tip_unita, tip_futuro)
SELECT 'Temperatura Interna', 'ds18b20', sen_id, 'C', 0
FROM Sensore
WHERE sen_codice = 'DS18B20'
ON DUPLICATE KEY UPDATE
    tip_tipologia = VALUES(tip_tipologia),
    tip_sen_id = VALUES(tip_sen_id),
    tip_unita = VALUES(tip_unita),
    tip_futuro = VALUES(tip_futuro);

INSERT INTO TipoRilevazione (tip_tipologia, tip_codice, tip_sen_id, tip_unita, tip_futuro)
SELECT 'Umidita Ambiente', 'sht21_humidity', sen_id, '%', 0
FROM Sensore
WHERE sen_codice = 'HTU21D'
ON DUPLICATE KEY UPDATE
    tip_tipologia = VALUES(tip_tipologia),
    tip_sen_id = VALUES(tip_sen_id),
    tip_unita = VALUES(tip_unita),
    tip_futuro = VALUES(tip_futuro);

INSERT INTO TipoRilevazione (tip_tipologia, tip_codice, tip_sen_id, tip_unita, tip_futuro)
SELECT 'Temperatura Ambiente', 'sht21_temperature', sen_id, 'C', 0
FROM Sensore
WHERE sen_codice = 'HTU21D'
ON DUPLICATE KEY UPDATE
    tip_tipologia = VALUES(tip_tipologia),
    tip_sen_id = VALUES(tip_sen_id),
    tip_unita = VALUES(tip_unita),
    tip_futuro = VALUES(tip_futuro);

INSERT INTO TipoRilevazione (tip_tipologia, tip_codice, tip_sen_id, tip_unita, tip_futuro)
SELECT 'Peso', 'hx711', sen_id, 'kg', 0
FROM Sensore
WHERE sen_codice = 'HX711'
ON DUPLICATE KEY UPDATE
    tip_tipologia = VALUES(tip_tipologia),
    tip_sen_id = VALUES(tip_sen_id),
    tip_unita = VALUES(tip_unita),
    tip_futuro = VALUES(tip_futuro);

-- Configurazione scheda per MAC (chiave dispositiva)
INSERT INTO ConfigurazioneScheda (
    cfs_arn_id, cfs_macAddress,
    calibrationFactor, calibrationOffset,
    rest_timeout_ms, wdt_timeout_sec, wifi_check_ms, ota_abilitato
)
VALUES (
    @ARN_ID, @MAC_REALE,
    2280.0, 50000,
    10000, 30, 10000, 1
)
ON DUPLICATE KEY UPDATE
    cfs_arn_id = VALUES(cfs_arn_id),
    cfs_macAddress = VALUES(cfs_macAddress),
    calibrationFactor = VALUES(calibrationFactor),
    calibrationOffset = VALUES(calibrationOffset),
    rest_timeout_ms = VALUES(rest_timeout_ms),
    wdt_timeout_sec = VALUES(wdt_timeout_sec),
    wifi_check_ms = VALUES(wifi_check_ms),
    ota_abilitato = VALUES(ota_abilitato);

-- Config runtime sensori su arnia (default firmware correnti)
-- DS18B20: 30..37 C, 360000 ms
INSERT INTO SensoreArnia (
    sea_arn_id, sea_tip_id, sea_stato, sea_attivo, sea_on, sea_min, sea_max, sea_intervallo_ms, sea_note
)
SELECT
    @ARN_ID, tip_id, 1, 1, 1, 30.0, 37.0, 360000, 'ESP default ds18b20'
FROM TipoRilevazione
WHERE tip_codice = 'ds18b20'
ON DUPLICATE KEY UPDATE
    sea_stato = VALUES(sea_stato),
    sea_attivo = VALUES(sea_attivo),
    sea_on = VALUES(sea_on),
    sea_min = VALUES(sea_min),
    sea_max = VALUES(sea_max),
    sea_intervallo_ms = VALUES(sea_intervallo_ms),
    sea_note = VALUES(sea_note);

-- SHT21 Umidita: 40..70 %, 360000 ms
INSERT INTO SensoreArnia (
    sea_arn_id, sea_tip_id, sea_stato, sea_attivo, sea_on, sea_min, sea_max, sea_intervallo_ms, sea_note
)
SELECT
    @ARN_ID, tip_id, 1, 1, 1, 40.0, 70.0, 360000, 'ESP default sht21_humidity'
FROM TipoRilevazione
WHERE tip_codice = 'sht21_humidity'
ON DUPLICATE KEY UPDATE
    sea_stato = VALUES(sea_stato),
    sea_attivo = VALUES(sea_attivo),
    sea_on = VALUES(sea_on),
    sea_min = VALUES(sea_min),
    sea_max = VALUES(sea_max),
    sea_intervallo_ms = VALUES(sea_intervallo_ms),
    sea_note = VALUES(sea_note);

-- SHT21 Temperatura: 10..45 C, 360000 ms
INSERT INTO SensoreArnia (
    sea_arn_id, sea_tip_id, sea_stato, sea_attivo, sea_on, sea_min, sea_max, sea_intervallo_ms, sea_note
)
SELECT
    @ARN_ID, tip_id, 1, 1, 1, 10.0, 45.0, 360000, 'ESP default sht21_temperature'
FROM TipoRilevazione
WHERE tip_codice = 'sht21_temperature'
ON DUPLICATE KEY UPDATE
    sea_stato = VALUES(sea_stato),
    sea_attivo = VALUES(sea_attivo),
    sea_on = VALUES(sea_on),
    sea_min = VALUES(sea_min),
    sea_max = VALUES(sea_max),
    sea_intervallo_ms = VALUES(sea_intervallo_ms),
    sea_note = VALUES(sea_note);

-- HX711: 10..80 kg, 60000 ms (default corrente in connection_manager.ino)
INSERT INTO SensoreArnia (
    sea_arn_id, sea_tip_id, sea_stato, sea_attivo, sea_on, sea_min, sea_max, sea_intervallo_ms, sea_note
)
SELECT
    @ARN_ID, tip_id, 1, 1, 1, 10.0, 80.0, 60000, 'ESP default hx711'
FROM TipoRilevazione
WHERE tip_codice = 'hx711'
ON DUPLICATE KEY UPDATE
    sea_stato = VALUES(sea_stato),
    sea_attivo = VALUES(sea_attivo),
    sea_on = VALUES(sea_on),
    sea_min = VALUES(sea_min),
    sea_max = VALUES(sea_max),
    sea_intervallo_ms = VALUES(sea_intervallo_ms),
    sea_note = VALUES(sea_note);

