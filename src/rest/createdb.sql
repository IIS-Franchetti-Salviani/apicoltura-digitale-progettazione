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
    sea_cal_factor DOUBLE DEFAULT NULL,    -- HX711: fattore di calibrazione cella di carico
    sea_tare_offset BIGINT DEFAULT NULL,   -- HX711: offset ADC grezzo per zero persistente (NULL = tara non ancora effettuata)
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
    sts_id BIGINT 