-- ============================================================================
-- Script ad-hoc: tabella StatoInvioSensore
-- Da eseguire sul DB REST esistente senza ricreare tutto lo schema.
-- ============================================================================

CREATE TABLE IF NOT EXISTS StatoInvioSensore (
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
