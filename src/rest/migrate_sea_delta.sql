-- ============================================================================
-- MIGRAZIONE: aggiunta sea_delta a SensoreArnia
-- Eseguire sul DB esistente (non ricrea le tabelle).
-- ============================================================================

USE Sql1287228_4;  -- <-- adattare al nome del database in produzione

-- 1. Aggiunta colonna (idempotente: fallisce silenziosamente se già esiste)
ALTER TABLE SensoreArnia
    ADD COLUMN sea_delta DOUBLE DEFAULT NULL
        COMMENT 'Variazione minima del dato per invio anticipato al server (NULL = disabilitato)'
    AFTER sea_intervallo_ms;

-- 2. Valori default per tipo sensore (basati su tip_codice)
--    Regolare in base alle esigenze operative dell'arnia.
UPDATE SensoreArnia sa
JOIN TipoRilevazione t ON t.tip_id = sa.sea_tip_id
SET sa.sea_delta = CASE t.tip_codice
    WHEN 'ds18b20'           THEN 0.5    -- °C: segnala variazioni di mezzo grado
    WHEN 'sht21_temperature' THEN 0.5    -- °C
    WHEN 'sht21_humidity'    THEN 2.0    -- %RH: variazioni di 2 punti percentuali
    WHEN 'hx711'             THEN 1.0   -- kg: variazioni di 1kg
    ELSE NULL                            -- altri sensori: delta disabilitato
END
WHERE sa.sea_delta IS NULL;             -- non sovrascrive valori già impostati

-- 3. Verifica risultato
SELECT
    a.arn_MacAddress,
    t.tip_codice,
    sa.sea_intervallo_ms,
    sa.sea_delta,
    sa.sea_min,
    sa.sea_max,
    sa.sea_stato
FROM SensoreArnia sa
JOIN Arnia a ON a.arn_id = sa.sea_arn_id
JOIN TipoRilevazione t ON t.tip_id = sa.sea_tip_id
ORDER BY a.arn_MacAddress, t.tip_codice;
