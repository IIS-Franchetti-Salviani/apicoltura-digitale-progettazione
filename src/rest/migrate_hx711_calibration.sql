-- ============================================================================
-- MIGRAZIONE: aggiunge campi di calibrazione HX711 alla tabella SensoreArnia
-- ============================================================================
-- Compatibile con MySQL 5.7+ (NON usa ADD COLUMN IF NOT EXISTS, supportato
-- solo da MariaDB e MySQL 8.0.29+).
--
-- Scopo:
--   1. sea_cal_factor  -> fattore di calibrazione cella di carico (pendenza)
--      Rimane costante per tutta la vita della cella. Determinato una volta
--      con un peso campione noto.
--   2. sea_tare_offset -> offset ADC grezzo che rappresenta il "peso zero"
--      (arnia vuota: cassetta + telaini vuoti). Viene scritto dal firmware
--      quando l'apicoltore esegue una tara manuale dalla dashboard.
--
-- Con questi due valori persistenti il firmware NON fa piu' la tara automatica
-- al boot: carica scale+offset dal server e ottiene letture assolute e
-- confrontabili fra riavvii, OTA, reset watchdog, ecc.
-- ============================================================================

-- ----------------------------------------------------------------------------
-- 1. Aggiunta sea_cal_factor (solo se non esiste)
-- ----------------------------------------------------------------------------
SET @dbname = DATABASE();
SET @tablename = 'SensoreArnia';
SET @columnname = 'sea_cal_factor';
SET @ddl = (
  SELECT IF(
    COUNT(*) = 0,
    CONCAT('ALTER TABLE ', @tablename,
           ' ADD COLUMN ', @columnname,
           ' DOUBLE DEFAULT NULL',
           ' COMMENT ''HX711: fattore di calibrazione cella di carico (setCalFactor)''',
           ' AFTER sea_delta'),
    'SELECT ''Colonna sea_cal_factor gia esistente, skip'' AS info'
  )
  FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = @dbname
    AND TABLE_NAME   = @tablename
    AND COLUMN_NAME  = @columnname
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- ----------------------------------------------------------------------------
-- 2. Aggiunta sea_tare_offset (solo se non esiste)
-- ----------------------------------------------------------------------------
SET @columnname = 'sea_tare_offset';
SET @ddl = (
  SELECT IF(
    COUNT(*) = 0,
    CONCAT('ALTER TABLE ', @tablename,
           ' ADD COLUMN ', @columnname,
           ' BIGINT DEFAULT NULL',
           ' COMMENT ''HX711: offset ADC grezzo zero persistente (setTareOffset)''',
           ' AFTER sea_cal_factor'),
    'SELECT ''Colonna sea_tare_offset gia esistente, skip'' AS info'
  )
  FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = @dbname
    AND TABLE_NAME   = @tablename
    AND COLUMN_NAME  = @columnname
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- ----------------------------------------------------------------------------
-- 3. Popolamento default SOLO per i sensori di tipo peso (hx711).
--    Si assume che nella tabella TipoRilevazione esista una riga con
--    tip_nome LIKE 'peso%' oppure 'hx711%' oppure 'weight%'.
--    Adattare il WHERE se la convenzione locale e' diversa.
-- ----------------------------------------------------------------------------
UPDATE SensoreArnia sa
JOIN TipoRilevazione tr ON sa.sea_tip_id = tr.tip_id
SET
    sa.sea_cal_factor = COALESCE(sa.sea_cal_factor, 696.0)
    -- sea_tare_offset resta NULL: la tara va eseguita in campo dalla dashboard
WHERE LOWER(tr.tip_nome) LIKE 'peso%'
   OR LOWER(tr.tip_nome) LIKE 'hx711%'
   OR LOWER(tr.tip_nome) LIKE 'weight%';

-- ----------------------------------------------------------------------------
-- 4. Verifica: elenca i record peso con i nuovi campi
-- ----------------------------------------------------------------------------
SELECT sa.sea_id,
       sa.sea_arn_id,
       tr.tip_nome,
       sa.sea_cal_factor,
       sa.sea_tare_offset,
       CASE
         WHEN sa.sea_tare_offset IS NULL THEN 'TARA DA ESEGUIRE'
         ELSE 'CALIBRATO'
       END AS stato_calibrazione
FROM SensoreArnia sa
JOIN TipoRilevazione tr ON sa.sea_tip_id = tr.tip_id
WHERE LOWER(tr.tip_nome) LIKE 'peso%'
   OR LOWER(tr.tip_nome) LIKE 'hx711%'
   OR LOWER(tr.tip_nome) LIKE 'weight%';
