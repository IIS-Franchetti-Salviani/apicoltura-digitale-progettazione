<?php
// =============================================================
// Endpoint: /configurazioni
// GET /configurazioni                  -> Lista configurazioni scheda
// GET /configurazioni/{id}             -> Configurazione singola per ID
// GET /configurazioni?q={"macAddress":"..."}
//                                     -> Payload compatibile firmware ESP
// POST /configurazioni                 -> Crea configurazione scheda (+ sensori opzionali)
// PUT /configurazioni/{id}             -> Aggiorna configurazione scheda (+ sensori opzionali)
// DELETE /configurazioni/{id}          -> Elimina configurazione scheda
// =============================================================

require_once __DIR__ . '/../config.php';

$method = $_SERVER['REQUEST_METHOD'];

function boolToInt($value) {
    return $value ? 1 : 0;
}

function getTipIdByCodice($conn, $tipCodice) {
    $tipCodiceEsc = $conn->real_escape_string($tipCodice);
    $res = $conn->query("SELECT tip_id FROM TipoRilevazione WHERE tip_codice = '$tipCodiceEsc' LIMIT 1");
    if ($res && $res->num_rows > 0) {
        $row = $res->fetch_assoc();
        return (int)$row['tip_id'];
    }
    return null;
}

function upsertSensoreArniaFromPayload($conn, $arnId, $tipCodice, $payload) {
    if (!is_array($payload)) {
        return;
    }

    $tipId = getTipIdByCodice($conn, $tipCodice);
    if ($tipId === null) {
        return;
    }

    $seaMin = array_key_exists('sea_min', $payload) ? (float)$payload['sea_min'] : 'NULL';
    $seaMax = array_key_exists('sea_max', $payload) ? (float)$payload['sea_max'] : 'NULL';
    $intervallo = array_key_exists('intervallo', $payload) ? (int)$payload['intervallo'] : 360000;
    $stato = array_key_exists('sea_stato', $payload) ? boolToInt((bool)$payload['sea_stato']) : 1;

    $resSea = $conn->query("SELECT sea_id FROM SensoreArnia WHERE sea_arn_id = $arnId AND sea_tip_id = $tipId LIMIT 1");
    if ($resSea && $resSea->num_rows > 0) {
        $sea = $resSea->fetch_assoc();
        $seaId = (int)$sea['sea_id'];

        $sqlUpd = "UPDATE SensoreArnia
                   SET sea_stato = $stato,
                       sea_min = $seaMin,
                       sea_max = $seaMax,
                       sea_intervallo_ms = $intervallo
                   WHERE sea_id = $seaId";
        $conn->query($sqlUpd);
    } else {
        $sqlIns = "INSERT INTO SensoreArnia
                   (sea_arn_id, sea_tip_id, sea_stato, sea_attivo, sea_on, sea_min, sea_max, sea_intervallo_ms)
                   VALUES
                   ($arnId, $tipId, $stato, 1, 1, $seaMin, $seaMax, $intervallo)";
        $conn->query($sqlIns);
    }
}

function buildFirmwareConfigByMac($conn, $baseUrl, $macAddress) {
    $macEsc = $conn->real_escape_string($macAddress);

    $sqlCfg = "SELECT c.*, a.arn_id, a.arn_MacAddress
               FROM ConfigurazioneScheda c
               JOIN Arnia a ON a.arn_id = c.cfs_arn_id
               WHERE c.cfs_macAddress = '$macEsc'
               LIMIT 1";
    $resCfg = $conn->query($sqlCfg);
    if (!$resCfg || $resCfg->num_rows === 0) {
        return null;
    }

    $cfg = $resCfg->fetch_assoc();
    $arnId = (int)$cfg['cfs_arn_id'];

    // Default coerenti con src/esp/connection_manager.ino
    $output = [
        "_id" => (string)$cfg['cfs_id'],
        "macAddress" => $cfg['cfs_macAddress'],
        "cfs_arn_id" => $arnId,
        "calibrationFactor" => (float)$cfg['calibrationFactor'],
        "calibrationOffset" => (int)$cfg['calibrationOffset'],
        "ds18b20" => [
            "_id" => "",
            "sea_min" => 30.0,
            "sea_max" => 37.0,
            "intervallo" => 360000,
            "sea_stato" => true
        ],
        "sht21_humidity" => [
            "_id" => "",
            "sea_min" => 40.0,
            "sea_max" => 70.0,
            "intervallo" => 360000,
            "sea_stato" => true
        ],
        "sht21_temperature" => [
            "_id" => "",
            "sea_min" => 10.0,
            "sea_max" => 45.0,
            "intervallo" => 360000,
            "sea_stato" => true
        ],
        "hx711" => [
            "_id" => "",
            "sea_min" => 10.0,
            "sea_max" => 80.0,
            "intervallo" => 60000,
            "sea_stato" => true
        ],
        "link" => "$baseUrl/configurazioni/{$cfg['cfs_id']}"
    ];

    // Verifica se le colonne HX711 (sea_cal_factor, sea_tare_offset) esistono
    // per essere retro-compatibili con DB non ancora migrati.
    $hasCalFactor = false;
    $hasTareOffset = false;
    $resChkCal = $conn->query("SHOW COLUMNS FROM SensoreArnia LIKE 'sea_cal_factor'");
    if ($resChkCal && $resChkCal->num_rows > 0) $hasCalFactor = true;
    $resChkTare = $conn->query("SHOW COLUMNS FROM SensoreArnia LIKE 'sea_tare_offset'");
    if ($resChkTare && $resChkTare->num_rows > 0) $hasTareOffset = true;

    $selectCols = "sa.sea_id, sa.sea_min, sa.sea_max, sa.sea_intervallo_ms, sa.sea_stato, t.tip_codice";
    if ($hasCalFactor)  $selectCols .= ", sa.sea_cal_factor";
    if ($hasTareOffset) $selectCols .= ", sa.sea_tare_offset";

    $sqlSens = "SELECT $selectCols
                FROM SensoreArnia sa
                JOIN TipoRilevazione t ON t.tip_id = sa.sea_tip_id
                WHERE sa.sea_arn_id = $arnId";
    $resSens = $conn->query($sqlSens);

    while ($resSens && ($row = $resSens->fetch_assoc())) {
        $key = $row['tip_codice'];
        if (!isset($output[$key])) {
            continue;
        }

        $output[$key] = [
            "_id" => (string)$row['sea_id'],
            "sea_min" => isset($row['sea_min']) ? (float)$row['sea_min'] : null,
            "sea_max" => isset($row['sea_max']) ? (float)$row['sea_max'] : null,
            "intervallo" => (int)$row['sea_intervallo_ms'],
            "sea_stato" => (bool)$row['sea_stato']
        ];

        // Per il sensore peso HX711 esponi anche i campi di calibrazione
        // persistente (letti dal firmware in init_hx711 via connection_manager).
        if ($key === 'hx711') {
            if ($hasCalFactor && isset($row['sea_cal_factor']) && $row['sea_cal_factor'] !== null) {
                $output[$key]['cal_factor'] = (float)$row['sea_cal_factor'];
            }
            if ($hasTareOffset && isset($row['sea_tare_offset']) && $row['sea_tare_offset'] !== null) {
                // BIGINT: usa int, non float, per preservare i bit
                $output[$key]['tare_offset'] = (int)$row['sea_tare_offset'];
            }
        }
    }

    return $output;
}

if ($method === 'GET') {

    // Modalita' compatibile firmware: /configurazioni?q={"macAddress":"..."}
    if (isset($_GET['q'])) {
        $query = json_decode($_GET['q'], true);
        if (!is_array($query)) {
            http_response_code(400);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Parametro q non valido: atteso JSON"]);
            $conn->close();
            exit;
        }

        // Se q contiene solo macAddress, rispondi nel formato richiesto dal firmware ESP.
        if (isset($query['macAddress']) && count($query) === 1) {
            $config = buildFirmwareConfigByMac($conn, $baseUrl, $query['macAddress']);
            $payload = $config ? [$config] : [];

            header('Content-Type: application/json');
            echo json_encode($payload, JSON_PRETTY_PRINT);
            $conn->close();
            exit;
        }
    }

    // GET singola configurazione per ID
    if (isset($_GET['configurazioneId'])) {
        $cfgId = (int)$_GET['configurazioneId'];
        $sql = "SELECT * FROM ConfigurazioneScheda WHERE cfs_id = $cfgId";
        $result = $conn->query($sql);

        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();
            $row['cfs_id'] = (int)$row['cfs_id'];
            $row['cfs_arn_id'] = (int)$row['cfs_arn_id'];
            $row['calibrationFactor'] = (float)$row['calibrationFactor'];
            $row['calibrationOffset'] = (int)$row['calibrationOffset'];
            $row['rest_timeout_ms'] = (int)$row['rest_timeout_ms'];
            $row['wdt_timeout_sec'] = (int)$row['wdt_timeout_sec'];
            $row['wifi_check_ms'] = (int)$row['wifi_check_ms'];
            $row['ota_abilitato'] = (bool)$row['ota_abilitato'];
            $row['link'] = "$baseUrl/configurazioni/{$row['cfs_id']}";

            header('Content-Type: application/json');
            echo json_encode($row, JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Configurazione non trovata"]);
        }
    }
    // GET lista configurazioni
    else {
        $queryMeta = restGetCollectionQuery($conn, 'configurazioni');
        if (!$queryMeta['ok']) {
            http_response_code($queryMeta['status']);
            header('Content-Type: application/json');
            echo json_encode(["errore" => $queryMeta['errore']]);
            $conn->close();
            exit;
        }

        $configurazioni = [];
        $rows = $queryMeta['rows'];

        foreach ($rows as $row) {
            $arnia = null;
            if (isset($row['cfs_arn_id'])) {
                $arnId = (int)$row['cfs_arn_id'];
                $resArn = $conn->query("SELECT arn_nome, arn_MacAddress FROM Arnia WHERE arn_id = $arnId");
                if ($resArn && $resArn->num_rows > 0) {
                    $arn = $resArn->fetch_assoc();
                    $arnia = [
                        "arn_nome" => $arn['arn_nome'],
                        "arn_MacAddress" => $arn['arn_MacAddress'],
                        "isPartial" => true,
                        "link" => "$baseUrl/arnie/$arnId"
                    ];
                }
            }

            $configurazioni[] = [
                "cfs_id" => (int)$row['cfs_id'],
                "cfs_arn_id" => (int)$row['cfs_arn_id'],
                "cfs_macAddress" => $row['cfs_macAddress'],
                "calibrationFactor" => (float)$row['calibrationFactor'],
                "calibrationOffset" => (int)$row['calibrationOffset'],
                "rest_timeout_ms" => (int)$row['rest_timeout_ms'],
                "wdt_timeout_sec" => (int)$row['wdt_timeout_sec'],
                "wifi_check_ms" => (int)$row['wifi_check_ms'],
                "ota_abilitato" => (bool)$row['ota_abilitato'],
                "arnia" => $arnia,
                "link" => "$baseUrl/configurazioni/{$row['cfs_id']}"
            ];
        }

        restSendCollectionResponse($configurazioni, $queryMeta);
    }
}

elseif ($method === 'POST') {
    $data = getRequestJsonBody();

    if (!is_array($data)) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido"]);
        $conn->close();
        exit;
    }

    $arnId = isset($data['cfs_arn_id']) ? (int)$data['cfs_arn_id'] : 0;
    $mac = isset($data['cfs_macAddress']) ? $conn->real_escape_string($data['cfs_macAddress']) : '';

    if ($arnId <= 0 || $mac === '') {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Campi obbligatori: cfs_arn_id, cfs_macAddress"]);
        $conn->close();
        exit;
    }

    $calibrationFactor = isset($data['calibrationFactor']) ? (float)$data['calibrationFactor'] : 2280.0;
    $calibrationOffset = isset($data['calibrationOffset']) ? (int)$data['calibrationOffset'] : 50000;
    $restTimeout = isset($data['rest_timeout_ms']) ? (int)$data['rest_timeout_ms'] : 10000;
    $wdtTimeout = isset($data['wdt_timeout_sec']) ? (int)$data['wdt_timeout_sec'] : 30;
    $wifiCheck = isset($data['wifi_check_ms']) ? (int)$data['wifi_check_ms'] : 10000;
    $ota = isset($data['ota_abilitato']) ? boolToInt((bool)$data['ota_abilitato']) : 1;

    $sql = "INSERT INTO ConfigurazioneScheda
            (cfs_arn_id, cfs_macAddress, calibrationFactor, calibrationOffset, rest_timeout_ms, wdt_timeout_sec, wifi_check_ms, ota_abilitato)
            VALUES
            ($arnId, '$mac', $calibrationFactor, $calibrationOffset, $restTimeout, $wdtTimeout, $wifiCheck, $ota)";

    if ($conn->query($sql)) {
        $newId = $conn->insert_id;

        // Sensori opzionali nello stesso payload
        upsertSensoreArniaFromPayload($conn, $arnId, 'ds18b20', $data['ds18b20'] ?? null);
        upsertSensoreArniaFromPayload($conn, $arnId, 'sht21_humidity', $data['sht21_humidity'] ?? null);
        upsertSensoreArniaFromPayload($conn, $arnId, 'sht21_temperature', $data['sht21_temperature'] ?? null);
        upsertSensoreArniaFromPayload($conn, $arnId, 'hx711', $data['hx711'] ?? null);

        http_response_code(201);
        header('Content-Type: application/json');
        echo json_encode(["cfs_id" => (int)$newId, "link" => "$baseUrl/configurazioni/$newId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'PUT') {
    if (!isset($_GET['configurazioneId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro configurazioneId mancante"]);
        $conn->close();
        exit;
    }

    $cfgId = (int)$_GET['configurazioneId'];
    $data = getRequestJsonBody();

    if (!is_array($data)) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido"]);
        $conn->close();
        exit;
    }

    $resCfg = $conn->query("SELECT * FROM ConfigurazioneScheda WHERE cfs_id = $cfgId");
    if (!$resCfg || $resCfg->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Configurazione non trovata"]);
        $conn->close();
        exit;
    }
    $cfg = $resCfg->fetch_assoc();

    $arnId = isset($data['cfs_arn_id']) ? (int)$data['cfs_arn_id'] : (int)$cfg['cfs_arn_id'];
    $mac = isset($data['cfs_macAddress']) ? $conn->real_escape_string($data['cfs_macAddress']) : $conn->real_escape_string($cfg['cfs_macAddress']);
    $calibrationFactor = isset($data['calibrationFactor']) ? (float)$data['calibrationFactor'] : (float)$cfg['calibrationFactor'];
    $calibrationOffset = isset($data['calibrationOffset']) ? (int)$data['calibrationOffset'] : (int)$cfg['calibrationOffset'];
    $restTimeout = isset($data['rest_timeout_ms']) ? (int)$data['rest_timeout_ms'] : (int)$cfg['rest_timeout_ms'];
    $wdtTimeout = isset($data['wdt_timeout_sec']) ? (int)$data['wdt_timeout_sec'] : (int)$cfg['wdt_timeout_sec'];
    $wifiCheck = isset($data['wifi_check_ms']) ? (int)$data['wifi_check_ms'] : (int)$cfg['wifi_check_ms'];
    $ota = isset($data['ota_abilitato']) ? boolToInt((bool)$data['ota_abilitato']) : (int)$cfg['ota_abilitato'];

    $sql = "UPDATE ConfigurazioneScheda
            SET cfs_arn_id = $arnId,
                cfs_macAddress = '$mac',
                calibrationFactor = $calibrationFactor,
                calibrationOffset = $calibrationOffset,
                rest_timeout_ms = $restTimeout,
                wdt_timeout_sec = $wdtTimeout,
                wifi_check_ms = $wifiCheck,
                ota_abilitato = $ota
            WHERE cfs_id = $cfgId";

    if ($conn->query($sql)) {
        // Sensori opzionali nello stesso payload
        upsertSensoreArniaFromPayload($conn, $arnId, 'ds18b20', $data['ds18b20'] ?? null);
        upsertSensoreArniaFromPayload($conn, $arnId, 'sht21_humidity', $data['sht21_humidity'] ?? null);
        upsertSensoreArniaFromPayload($conn, $arnId, 'sht21_temperature', $data['sht21_temperature'] ?? null);
        upsertSensoreArniaFromPayload($conn, $arnId, 'hx711', $data['hx711'] ?? null);

        header('Content-Type: application/json');
        echo json_encode(["messaggio" => "Configurazione aggiornata", "link" => "$baseUrl/configurazioni/$cfgId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

// ---------------------------------------------------------------------------
// PATCH /configurazioni/{sea_id}
// ---------------------------------------------------------------------------
// Usato dal firmware ESP32 (connection_manager.ino -> save_hx711_calibration)
// per persistere su server la calibrazione HX711 dopo una tara manuale.
// A dispetto del path, l'ID qui e' il sea_id del SensoreArnia di tipo hx711
// (cosi' come lo riceve nel payload GET /configurazioni?q={"macAddress":...})
// e NON il cfs_id della ConfigurazioneScheda.
// Campi accettati nel body:
//   - sea_cal_factor  (DOUBLE)
//   - sea_tare_offset (BIGINT)
// Esempio:
//   PATCH /configurazioni/42
//   { "sea_cal_factor": 696.0, "sea_tare_offset": 123456 }
// ---------------------------------------------------------------------------
elseif ($method === 'PATCH') {
    if (!isset($_GET['configurazioneId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro id mancante nel path"]);
        $conn->close();
        exit;
    }

    $seaId = (int)$_GET['configurazioneId'];
    $data = getRequestJsonBody();
    if (!is_array($data)) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido"]);
        $conn->close();
        exit;
    }

    // Verifica che il sensore esista
    $resSea = $conn->query("SELECT sea_id FROM SensoreArnia WHERE sea_id = $seaId LIMIT 1");
    if (!$resSea || $resSea->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "SensoreArnia $seaId non trovato"]);
        $conn->close();
        exit;
    }

    // Verifica esistenza colonne (retro-compatibilita' con DB non migrati)
    $resChkCal = $conn->query("SHOW COLUMNS FROM SensoreArnia LIKE 'sea_cal_factor'");
    $hasCalFactor = ($resChkCal && $resChkCal->num_rows > 0);
    $resChkTare = $conn->query("SHOW COLUMNS FROM SensoreArnia LIKE 'sea_tare_offset'");
    $hasTareOffset = ($resChkTare && $resChkTare->num_rows > 0);

    if (!$hasCalFactor && !$hasTareOffset) {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode([
            "errore" => "Schema DB obsoleto: esegui migrate_hx711_calibration.sql"
        ]);
        $conn->close();
        exit;
    }

    $updates = [];
    if ($hasCalFactor && array_key_exists('sea_cal_factor', $data)) {
        $cal = (float)$data['sea_cal_factor'];
        if (is_finite($cal)) {
            $updates[] = "sea_cal_factor = $cal";
        }
    }
    if ($hasTareOffset && array_key_exists('sea_tare_offset', $data)) {
        // BIGINT: usa intval per evitare notazione esponenziale
        $tare = (int)$data['sea_tare_offset'];
        $updates[] = "sea_tare_offset = $tare";
    }

    if (empty($updates)) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode([
            "errore" => "Nessun campo aggiornabile nel body",
            "campi_ammessi" => ["sea_cal_factor", "sea_tare_offset"]
        ]);
        $conn->close();
        exit;
    }

    $setClause = implode(', ', $updates);
    $sql = "UPDATE SensoreArnia SET $setClause WHERE sea_id = $seaId";

    if ($conn->query($sql)) {
        header('Content-Type: application/json');
        echo json_encode([
            "messaggio" => "Calibrazione HX711 aggiornata",
            "sea_id" => $seaId,
            "campi_aggiornati" => array_keys(array_intersect_key(
                $data,
                array_flip(['sea_cal_factor', 'sea_tare_offset'])
            )),
            "link" => "$baseUrl/sensoriarnia/$seaId"
        ]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'DELETE') {
    if (!isset($_GET['configurazioneId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro configurazioneId mancante"]);
        $conn->close();
        exit;
    }

    $cfgId = (int)$_GET['configurazioneId'];

    $check = $conn->query("SELECT cfs_id FROM ConfigurazioneScheda WHERE cfs_id = $cfgId");
    if (!$check || $check->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Configurazione non trovata"]);
        $conn->close();
        exit;
    }

    if ($conn->query("DELETE FROM ConfigurazioneScheda WHERE cfs_id = $cfgId")) {
        http_response_code(204);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

$conn->close();
