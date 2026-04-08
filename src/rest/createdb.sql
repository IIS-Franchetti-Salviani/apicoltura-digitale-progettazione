-- ============================================================================
-- Database: apicoltura
-- Schema allineato a firmware ESP32 (src/esp) + REST PHP (src/rest)
-- Data: 2026-03-31
-- ============================================================================

DROP DATABASE IF EXISTS apicoltura;
CREATE DATABASE apicoltura CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
USE apicoltura;

SET FOREIGN_KEY_CHECKS = 0;

DROP VIEW IF EXISTS v_configurazioni_esp_flat;
DROP TABLE IF EXISTS ComandoDispositivo;
DROP TABLE IF EXISTS StoricoConfigurazioneSensore;
DROP TABLE IF EXISTS LogAccesso;
DROP TABLE IF EXISTS HeartbeatDispositivo;
DROP TABLE IF EXISTS StatoInvioSensore;
DROP TABLE IF EXISTS ConfigurazioneScheda;
DROP TABLE IF EXISTS Notifica;
DROP TABLE IF EXISTS Rilevazione;
DROP TABLE IF EXISTS SensoreArnia;
DROP TABLE IF EXISTS TipoRilevazione;
DROP TABLE IF EXISTS Sensore;
DROP TABLE IF EXISTS Arnia;
DROP TABLE IF EXISTS Apiario;
DROP TABLE IF EXISTS Utente;

SET FOREIGN_KEY_CHECKS = 1;

-- ============================================================================
-- UTENTE
-- ============================================================================
CREATE TABLE Utente (
    ute_id INT NOT NULL AUTO_INCREMENT,
    ute_username VARCHAR(100) NOT NULL,
    ute_password VARCHAR(255) NOT NULL,
    ute_admin BOOLEAN NOT NULL DEFAULT FALSE,
    ute_token VARCHAR(64) DEFAULT NULL,
    ute_scadenzaToken DATETIME DEFAULT NULL,
    ute_creato_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (ute_id),
    UNIQUE KEY uk_utente_username (ute_username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- APIARIO
-- ============================================================================
CREATE TABLE Apiario (
    api_id INT NOT NULL AUTO_INCREMENT,
    api_nome VARCHAR(200) NOT NULL,
    api_luogo VARCHAR(300) NOT NULL,
    api_lon DOUBLE DEFAULT NULL,
    api_lat DOUBLE DEFAULT NULL,
    api_creato_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (api_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- ARNIA
-- ============================================================================
CREATE TABLE Arnia (
    arn_id INT NOT NULL AUTO_INCREMENT,
    arn_api_id INT NOT NULL,
    arn_nome VARCHAR(100) DEFAULT NULL,
    arn_dataInst DATE NOT NULL,
    arn_piena BOOLEAN NOT NULL DEFAULT FALSE,
    arn_MacAddress VARCHAR(17) DEFAULT NULL,
    arn_attiva BOOLEAN NOT NULL DEFAULT TRUE,
    arn_creato_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (arn_id),
    UNIQUE KEY uk_arnia_mac (arn_MacAddress),
    KEY idx_arnia_apiario (arn_api_id),
    CONSTRAINT fk_arnia_apiario
        FOREIGN KEY (arn_api_id)
        REFERENCES Apiario(api_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- SENSORE (modello hardware)
-- ============================================================================
CREATE TABLE Sensore (
    sen_id INT NOT NULL AUTO_INCREMENT,
    sen_modello VARCHAR(100) NOT NULL,
    sen_codice VARCHAR(50) DEFAULT NULL,
    sen_produttore VARCHAR(80) DEFAULT NULL,
    PRIMARY KEY (sen_id),
    UNIQUE KEY uk_sensore_modello (sen_modello),
    UNIQUE KEY uk_sensore_codice (sen_codice)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- TIPO RILEVAZIONE (grandezza logica misurata)
-- tip_codice e' la chiave usata dal firmware per mappare i sensori:
-- ds18b20, sht21_humidity, sht21_temperature, hx711, ...
-- ============================================================================
CREATE TABLE TipoRilevazione (
    tip_id INT NOT NULL AUTO_INCREMENT,
    tip_tipologia VARCHAR(100) NOT NULL,
    tip_codice VARCHAR(50) NOT NULL,
    tip_sen_id INT NOT NULL,
    tip_unita VARCHAR(20) NOT NULL,
    tip_futuro BOOLEAN NOT NULL DEFAULT FALSE,
    PRIMARY KEY (tip_id),
    UNIQUE KEY uk_tip_codice (tip_codice),
    KEY idx_tipo_sensore (tip_sen_id),
    CONSTRAINT fk_tipo_sensore
        FOREIGN KEY (tip_sen_id)
        REFERENCES Sensore(sen_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- SENSORE ARNIA (configurazione runtime per arnia e tipo)
-- sea_intervallo_ms e' richiesto dal firmware per intervalli di campionamento.
-- ============================================================================
CREATE TABLE SensoreArnia (
    sea_id INT NOT NULL AUTO_INCREMENT,
    sea_arn_id INT NOT NULL,
    sea_tip_id INT NOT NULL,
    sea_stato BOOLEAN NOT NULL DEFAULT TRUE,
    sea_attivo BOOLEAN NOT NULL DEFAULT TRUE,
    sea_on BOOLEAN NOT NULL DEFAULT TRUE,
    sea_min DOUBLE DEFAULT NULL,
    sea_max DOUBLE DEFAULT NULL,
    sea_intervallo_ms INT UNSIGNED NOT NULL DEFAULT 360000,
    sea_delta DOUBLE DEFAULT NULL,         -- variazione minima per invio anticipato (NULL = disabilitato)
    sea_note VARCHAR(255) DEFAULT NULL,
    sea_aggiornato_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (sea_id),
    UNIQUE KEY uk_sensore_arnia_tipo (sea_arn_id, sea_tip_id),
    KEY idx_sea_arnia (sea_arn_id),
    KEY idx_sea_tipo (sea_tip_id),
    CONSTRAINT fk_sea_arnia
        FOREIGN KEY (sea_arn_id)
        REFERENCES Arnia(arn_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,
    CONSTRAINT fk_sea_tipo
        FOREIGN KEY (sea_tip_id)
        REFERENCES TipoRilevazione(tip_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- RILEVAZIONE
-- ril_codice_stato salva codici 9xxx/1xxx/2xxx... del firmware.
-- ============================================================================
CREATE TABLE Rilevazione (
    ril_id BIGINT NOT NULL AUTO_INCREMENT,
    ril_sea_id INT NOT NULL,
    ril_dato DOUBLE NOT NULL,
    ril_dataOra DATETIME NOT NULL,
    ril_codice_stato INT NOT NULL DEFAULT 9000,
    ril_creata_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (ril_id),
    KEY idx_ril_sensore (ril_sea_id),
    KEY idx_ril_dataora (ril_dataOra),
    KEY idx_ril_sensore_dataora (ril_sea_id, ril_dataOra),
    CONSTRAINT fk_ril_sensore
        FOREIGN KEY (ril_sea_id)
        REFERENCES SensoreArnia(sea_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- STATO INVIO SENSORE
-- Traccia stato di abilitazione e cause impedimento invio dal firmware.
-- ============================================================================
CREATE TABLE StatoInvioSensore (
    sts_id BIGINT NOT NULL AUTO_INCREMENT,
    sts_macAddress VARCHAR(17) NOT NULL,
    sts_tipoSensore VARCHAR(32) NOT NULL,
    sts_sensorId VARCHAR(32) DEFAULT NULL,
    sts_abilitato BOOLEAN NOT NULL DEFAULT TRUE,
    sts_evento ENUM(
        'CONFIG_SYNC',
        'INVIO_OK',
        'INVIO_BLOCCATO',
        'LETTURA_NON_VALIDA',
        'WIFI_OFFLINE',
        'ERRORE_SERVER'
    ) NOT NULL DEFAULT 'CONFIG_SYNC',
    sts_causaCodice VARCHAR(64) DEFAULT NULL,
    sts_causaDettaglio VARCHAR(255) DEFAULT NULL,
    sts_codiceStato INT DEFAULT NULL,
    sts_valore DOUBLE DEFAULT NULL,
    sts_timestamp DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    sts_creato_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (sts_id),
    KEY idx_sts_mac_ts (sts_macAddress, sts_timestamp),
    KEY idx_sts_tipo_ts (sts_tipoSensore, sts_timestamp),
    KEY idx_sts_evento_ts (sts_evento, sts_timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- NOTIFICA
-- Compatibile sia con il modello legacy (not_ril_id, titolo, descrizione)
-- sia con payload ESP (macAddress, tipoSensore, livello, timestamp...).
-- ============================================================================
CREATE TABLE Notifica (
    not_id BIGINT NOT NULL AUTO_INCREMENT,
    not_ril_id BIGINT DEFAULT NULL,
    not_titolo VARCHAR(200) NOT NULL DEFAULT 'Notifica',
    not_dex TEXT NOT NULL,
    not_macAddress VARCHAR(17) DEFAULT NULL,
    not_tipoSensore VARCHAR(32) DEFAULT NULL,
    not_valoreRiferimento DOUBLE DEFAULT NULL,
    not_timestamp DATETIME DEFAULT NULL,
    not_livello TINYINT NOT NULL DEFAULT 1,
    not_livelloStr VARCHAR(16) DEFAULT NULL,
    not_letto BOOLEAN NOT NULL DEFAULT FALSE,
    not_creata_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (not_id),
    KEY idx_not_rilevazione (not_ril_id),
    KEY idx_not_mac_ts (not_macAddress, not_timestamp),
    KEY idx_not_letto_ts (not_letto, not_timestamp),
    CONSTRAINT fk_not_rilevazione
        FOREIGN KEY (not_ril_id)
        REFERENCES Rilevazione(ril_id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- CONFIGURAZIONE SCHEDA (mancava nel modello originale)
-- Chiave per dispositivo: MAC address.
-- ============================================================================
CREATE TABLE ConfigurazioneScheda (
    cfs_id INT NOT NULL AUTO_INCREMENT,
    cfs_arn_id INT NOT NULL,
    cfs_macAddress VARCHAR(17) NOT NULL,
    calibrationFactor DOUBLE NOT NULL DEFAULT 2280.0,
    calibrationOffset BIGINT NOT NULL DEFAULT 50000,
    rest_timeout_ms INT UNSIGNED NOT NULL DEFAULT 10000,
    wdt_timeout_sec INT UNSIGNED NOT NULL DEFAULT 30,
    wifi_check_ms INT UNSIGNED NOT NULL DEFAULT 10000,
    ota_abilitato BOOLEAN NOT NULL DEFAULT TRUE,
    cfs_aggiornato_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (cfs_id),
    UNIQUE KEY uk_cfs_arnia (cfs_arn_id),
    UNIQUE KEY uk_cfs_mac (cfs_macAddress),
    CONSTRAINT fk_cfs_arnia
        FOREIGN KEY (cfs_arn_id)
        REFERENCES Arnia(arn_id)
        ON UPDATE CASCADE
        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- HEARTBEAT DISPOSITIVO (tabella non prevista ma utile per operativita')
-- ============================================================================
CREATE TABLE HeartbeatDispositivo (
    hbt_id BIGINT NOT NULL AUTO_INCREMENT,
    hbt_arn_id INT DEFAULT NULL,
    hbt_macAddress VARCHAR(17) NOT NULL,
    hbt_ip VARCHAR(45) DEFAULT NULL,
    hbt_ssid VARCHAR(64) DEFAULT NULL,
    hbt_rssi SMALLINT DEFAULT NULL,
    hbt_free_heap INT DEFAULT NULL,
    hbt_uptime_sec INT UNSIGNED DEFAULT NULL,
    hbt_firmware VARCHAR(50) DEFAULT NULL,
    hbt_dataOra DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (hbt_id),
    KEY idx_hbt_mac_dataora (hbt_macAddress, hbt_dataOra),
    KEY idx_hbt_arnia_dataora (hbt_arn_id, hbt_dataOra),
    CONSTRAINT fk_hbt_arnia
        FOREIGN KEY (hbt_arn_id)
        REFERENCES Arnia(arn_id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- LOG ACCESSO (RNF-DB-05)
-- ============================================================================
CREATE TABLE LogAccesso (
    log_id BIGINT NOT NULL AUTO_INCREMENT,
    log_ute_id INT DEFAULT NULL,
    log_username VARCHAR(100) DEFAULT NULL,
    log_ip VARCHAR(45) DEFAULT NULL,
    log_user_agent VARCHAR(255) DEFAULT NULL,
    log_esito ENUM('SUCCESS','FAIL') NOT NULL,
    log_dataOra DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (log_id),
    KEY idx_log_dataora (log_dataOra),
    KEY idx_log_username_dataora (log_username, log_dataOra),
    CONSTRAINT fk_log_utente
        FOREIGN KEY (log_ute_id)
        REFERENCES Utente(ute_id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- STORICO CONFIGURAZIONE SENSORE (tabella non prevista ma consigliata)
-- ============================================================================
CREATE TABLE StoricoConfigurazioneSensore (
    scs_id BIGINT NOT NULL AUTO_INCREMENT,
    scs_sea_id INT NOT NULL,
    scs_old_min DOUBLE DEFAULT NULL,
    scs_old_max DOUBLE DEFAULT NULL,
    scs_old_intervallo_ms INT UNSIGNED DEFAULT NULL,
    scs_old_stato BOOLEAN DEFAULT NULL,
    scs_new_min DOUBLE DEFAULT NULL,
    scs_new_max DOUBLE DEFAULT NULL,
    scs_new_intervallo_ms INT UNSIGNED DEFAULT NULL,
    scs_new_stato BOOLEAN DEFAULT NULL,
    scs_changed_by INT DEFAULT NULL,
    scs_changed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (scs_id),
    KEY idx_scs_sea_changed_at (scs_sea_id, scs_changed_at),
    CONSTRAINT fk_scs_sea
        FOREIGN KEY (scs_sea_id)
        REFERENCES SensoreArnia(sea_id)
        ON UPDATE CASCADE
        ON DELETE CASCADE,
    CONSTRAINT fk_scs_utente
        FOREIGN KEY (scs_changed_by)
        REFERENCES Utente(ute_id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- COMANDO DISPOSITIVO (tabella non prevista, utile per OTA/azioni remote)
-- ============================================================================
CREATE TABLE ComandoDispositivo (
    cmd_id BIGINT NOT NULL AUTO_INCREMENT,
    cmd_arn_id INT NOT NULL,
    cmd_macAddress VARCHAR(17) NOT NULL,
    cmd_tipo ENUM('SYNC_CONFIG','REBOOT','OTA','TARE_HX711') NOT NULL,
    cmd_payload_json LONGTEXT,
    cmd_stato ENUM('PENDING','SENT','ACK','ERROR','CANCELLED') NOT NULL DEFAULT 'PENDING',
    cmd_creato_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    cmd_eseguito_at DATETIME DEFAULT NULL,
    PRIMARY KEY (cmd_id),
    KEY idx_cmd_mac_stato (cmd_macAddress, cmd_stato),
    KEY idx_cmd_arnia_stato (cmd_arn_id, cmd_stato),
    CONSTRAINT fk_cmd_arnia
        FOREIGN KEY (cmd_arn_id)
        REFERENCES Arnia(arn_id)
        ON UPDATE CASCADE
        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- ============================================================================
-- VIEW di supporto per endpoint /configurazioni (flatten)
-- Da questa view puoi comporre il JSON con chiavi ds18b20/sht21_humidity...
-- ============================================================================
CREATE VIEW v_configurazioni_esp_flat AS
SELECT
    a.arn_MacAddress AS macAddress,
    c.calibrationFactor,
    c.calibrationOffset,
    t.tip_codice,
    sa.sea_id,
    sa.sea_stato,
    sa.sea_min,
    sa.sea_max,
    sa.sea_intervallo_ms
FROM Arnia a
JOIN ConfigurazioneScheda c
    ON c.cfs_arn_id = a.arn_id
JOIN SensoreArnia sa
    ON sa.sea_arn_id = a.arn_id
JOIN TipoRilevazione t
    ON t.tip_id = sa.sea_tip_id;

-- ============================================================================
-- DATI DI ESEMPIO COERENTI CON I DEFAULT IN src/esp/connection_manager.ino
-- ============================================================================

-- Utente admin (in produzione usare hash)
INSERT INTO Utente (ute_username, ute_password, ute_admin)
VALUES ('admin', 'admin123', TRUE);

-- Apiario + Arnia
INSERT INTO Apiario (api_nome, api_luogo, api_lon, api_lat)
VALUES ('Apicoltura Santucci - apiario principale', 'Voc. Rotetino, Monte Santa Maria Tiberina', 12.203588, 43.385117);

INSERT INTO Arnia (arn_api_id, arn_nome, arn_dataInst, arn_piena, arn_MacAddress)
VALUES (1, 'Arnia 01', '2026-01-20', TRUE, 'FB:3F:18:47:FC:3F');

-- Catalogo sensori
INSERT INTO Sensore (sen_modello, sen_codice, sen_produttore) VALUES
('DS18B20', 'DS18B20', 'Maxim'),
('HTU21D', 'HTU21D', 'TE Connectivity'),
('HX711', 'HX711', 'Avia'),
('INMP441', 'INMP441', 'InvenSense'),
('HW-038', 'HW038', 'Generic'),
('ESP32-CAM', 'ESP32CAM', 'Espressif');

-- Tipi rilevazione (tip_codice mappa con firmware)
INSERT INTO TipoRilevazione (tip_tipologia, tip_codice, tip_sen_id, tip_unita, tip_futuro) VALUES
('Temperatura Interna', 'ds18b20', 1, 'C', FALSE),
('Umidita Ambiente', 'sht21_humidity', 2, '%', FALSE),
('Temperatura Ambiente', 'sht21_temperature', 2, 'C', FALSE),
('Peso', 'hx711', 3, 'kg', FALSE),
('Audio', 'audio', 4, 'Hz', TRUE),
('Livello Acqua', 'water_level', 5, '%', TRUE),
('Immagine', 'camera', 6, 'frame', TRUE);

-- Config sensori arnia 1
-- Coerente con default connection_manager.ino:
-- ds18b20: min 30 max 37 intervallo 360000
-- sht21_humidity: min 40 max 70 intervallo 360000
-- sht21_temperature: min 10 max 45 intervallo 360000
-- hx711: min 10 max 80 intervallo 60000
INSERT INTO SensoreArnia (
    sea_arn_id, sea_tip_id, sea_stato, sea_attivo, sea_on, sea_min, sea_max, sea_intervallo_ms, sea_note
)
SELECT 1, tip_id, TRUE, TRUE, TRUE,
       CASE tip_codice
           WHEN 'ds18b20' THEN 30.0
           WHEN 'sht21_humidity' THEN 40.0
           WHEN 'sht21_temperature' THEN 10.0
           WHEN 'hx711' THEN 10.0
           ELSE NULL
       END AS sea_min,
       CASE tip_codice
           WHEN 'ds18b20' THEN 37.0
           WHEN 'sht21_humidity' THEN 70.0
           WHEN 'sht21_temperature' THEN 45.0
           WHEN 'hx711' THEN 80.0
           ELSE NULL
       END AS sea_max,
       CASE tip_codice
           WHEN 'ds18b20' THEN 360000
           WHEN 'sht21_humidity' THEN 360000
           WHEN 'sht21_temperature' THEN 360000
           WHEN 'hx711' THEN 60000
           ELSE 360000
       END AS sea_intervallo_ms,
       'Default firmware ESP'
FROM TipoRilevazione
WHERE tip_codice IN ('ds18b20', 'sht21_humidity', 'sht21_temperature', 'hx711');

-- Configurazione scheda per MAC address (mancava)
INSERT INTO ConfigurazioneScheda (
    cfs_arn_id, cfs_macAddress, calibrationFactor, calibrationOffset, rest_timeout_ms, wdt_timeout_sec, wifi_check_ms, ota_abilitato
) VALUES (
    1, 'FB:3F:18:47:FC:3F', 2280.0, 50000, 10000, 30, 10000, TRUE
);

-- Rilevazioni esempio
INSERT INTO Rilevazione (ril_sea_id, ril_dato, ril_dataOra, ril_codice_stato)
SELECT sa.sea_id, 34.8, '2026-03-31 10:00:00', 9000
FROM SensoreArnia sa
JOIN TipoRilevazione t ON t.tip_id = sa.sea_tip_id
WHERE sa.sea_arn_id = 1 AND t.tip_codice = 'ds18b20';

INSERT INTO Rilevazione (ril_sea_id, ril_dato, ril_dataOra, ril_codice_stato)
SELECT sa.sea_id, 56.2, '2026-03-31 10:00:00', 9000
FROM SensoreArnia sa
JOIN TipoRilevazione t ON t.tip_id = sa.sea_tip_id
WHERE sa.sea_arn_id = 1 AND t.tip_codice = 'sht21_humidity';

INSERT INTO Rilevazione (ril_sea_id, ril_dato, ril_dataOra, ril_codice_stato)
SELECT sa.sea_id, 23.6, '2026-03-31 10:00:00', 9000
FROM SensoreArnia sa
JOIN TipoRilevazione t ON t.tip_id = sa.sea_tip_id
WHERE sa.sea_arn_id = 1 AND t.tip_codice = 'sht21_temperature';

INSERT INTO Rilevazione (ril_sea_id, ril_dato, ril_dataOra, ril_codice_stato)
SELECT sa.sea_id, 41.3, '2026-03-31 10:00:00', 9000
FROM SensoreArnia sa
JOIN TipoRilevazione t ON t.tip_id = sa.sea_tip_id
WHERE sa.sea_arn_id = 1 AND t.tip_codice = 'hx711';

-- Stato invio sensori esempio (sync configurazione)
INSERT INTO StatoInvioSensore (
    sts_macAddress, sts_tipoSensore, sts_sensorId, sts_abilitato, sts_evento,
    sts_causaCodice, sts_causaDettaglio, sts_codiceStato, sts_valore, sts_timestamp
) VALUES
('FB:3F:18:47:FC:3F', 'ds18b20', '1', TRUE, 'CONFIG_SYNC', NULL, NULL, 9000, NULL, '2026-03-31 09:55:00'),
('FB:3F:18:47:FC:3F', 'sht21_humidity', '2', TRUE, 'CONFIG_SYNC', NULL, NULL, 9000, NULL, '2026-03-31 09:55:00'),
('FB:3F:18:47:FC:3F', 'sht21_temperature', '3', TRUE, 'CONFIG_SYNC', NULL, NULL, 9000, NULL, '2026-03-31 09:55:00'),
('FB:3F:18:47:FC:3F', 'hx711', '4', TRUE, 'CONFIG_SYNC', NULL, NULL, 9000, NULL, '2026-03-31 09:55:00');

-- Notifica legacy collegata a rilevazione
INSERT INTO Notifica (not_ril_id, not_titolo, not_dex, not_livello, not_livelloStr, not_letto)
SELECT r.ril_id, 'PESO TROPPO ELEVATO', 'ALLARME: PESO TROPPO ELEVATO 41.3kg', 2, 'ERROR', FALSE
FROM Rilevazione r
JOIN SensoreArnia sa ON sa.sea_id = r.ril_sea_id
JOIN TipoRilevazione t ON t.tip_id = sa.sea_tip_id
WHERE sa.sea_arn_id = 1 AND t.tip_codice = 'hx711'
ORDER BY r.ril_dataOra DESC
LIMIT 1;

-- Notifica stile payload ESP (senza ril_id)
INSERT INTO Notifica (
    not_ril_id, not_titolo, not_dex, not_macAddress, not_tipoSensore,
    not_valoreRiferimento, not_timestamp, not_livello, not_livelloStr, not_letto
) VALUES (
    NULL, 'ALERT TEMPERATURA', 'Temperatura sopra soglia massima',
    'FB:3F:18:47:FC:3F', 'ds18b20', 38.2, '2026-03-31 11:05:00', 2, 'ERROR', FALSE
);

-- Heartbeat esempio
INSERT INTO HeartbeatDispositivo (
    hbt_arn_id, hbt_macAddress, hbt_ip, hbt_ssid, hbt_rssi, hbt_free_heap, hbt_uptime_sec, hbt_firmware
) VALUES (
    1, 'FB:3F:18:47:FC:3F', '192.168.1.77', 'Gruppo4Network', -62, 187240, 86400, 'Main Controller v2.4'
);

-- ============================================================================
-- QUERY RAPIDE PER /configurazioni
-- ============================================================================
-- Esempio fetch flat:
-- SELECT * FROM v_configurazioni_esp_flat WHERE macAddress = 'FB:3F:18:47:FC:3F';
--
-- Il tuo endpoint /configurazioni deve trasformare il result set in:
-- [
--   {
--     "macAddress":"FB:3F:18:47:FC:3F",
--     "calibrationFactor":2280.0,
--     "calibrationOffset":50000,
--     "ds18b20":{"_id":"1","sea_min":30,"sea_max":37,"intervallo":360000,"sea_stato":true},
--     "sht21_humidity":{"_id":"2","sea_min":40,"sea_max":70,"intervallo":360000,"sea_stato":true},
--     "sht21_temperature":{"_id":"3","sea_min":10,"sea_max":45,"intervallo":360000,"sea_stato":true},
--     "hx711":{"_id":"4","sea_min":10,"sea_max":80,"intervallo":60000,"sea_stato":true}
--   }
-- ]
