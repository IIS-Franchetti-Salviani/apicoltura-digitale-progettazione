<?php
// =============================================================
// Endpoint: /notifiche
// GET /notifiche         -> Lista tutte le notifiche con rilevazione (partial)
// GET /notifiche/{id}    -> Dettaglio singola notifica
// POST /notifiche        -> Crea una nuova notifica/allarme
// PUT /notifiche/{id}    -> Aggiorna una notifica
// DELETE /notifiche/{id} -> Elimina una notifica
// =============================================================

require_once __DIR__ . '/../config.php';

$method = $_SERVER['REQUEST_METHOD'];

function notificaColumnExists($conn, $column) {
    static $cache = [];
    if (isset($cache[$column])) {
        return $cache[$column];
    }

    $colEsc = $conn->real_escape_string($column);
    $res = $conn->query("SHOW COLUMNS FROM `Notifica` LIKE '$colEsc'");
    $exists = ($res && $res->num_rows > 0);
    $cache[$column] = $exists;
    return $exists;
}

function notificaLevelToString($level) {
    switch ((int)$level) {
        case 0: return 'INFO';
        case 1: return 'WARNING';
        case 2: return 'ERROR';
        case 3: return 'CRITICAL';
        default: return 'UNKNOWN';
    }
}

function toSqlDatetimeOrNull($value) {
    if ($value === null || $value === '') {
        return null;
    }

    if (is_numeric($value)) {
        $ts = (int)$value;
        if ($ts > 1000000000000) {
            $ts = (int)round($ts / 1000);
        }
        if ($ts > 0) {
            return date('Y-m-d H:i:s', $ts);
        }
        return null;
    }

    $parsed = strtotime((string)$value);
    if ($parsed === false) {
        return null;
    }
    return date('Y-m-d H:i:s', $parsed);
}

function buildNotificaResponseRow($row, $baseUrl, $rilevazione = null) {
    $item = [
        "not_id" => (int)$row["not_id"],
        "not_ril_id" => isset($row["not_ril_id"]) ? (is_null($row["not_ril_id"]) ? null : (int)$row["not_ril_id"]) : null,
        "not_titolo" => $row["not_titolo"],
        "not_dex" => $row["not_dex"],
        "rilevazione" => $rilevazione,
        "link" => "$baseUrl/notifiche/{$row["not_id"]}"
    ];

    if (array_key_exists("not_macAddress", $row)) {
        $item["not_macAddress"] = $row["not_macAddress"];
    }
    if (array_key_exists("not_tipoSensore", $row)) {
        $item["not_tipoSensore"] = $row["not_tipoSensore"];
    }
    if (array_key_exists("not_valoreRiferimento", $row)) {
        $item["not_valoreRiferimento"] = is_null($row["not_valoreRiferimento"]) ? null : (float)$row["not_valoreRiferimento"];
    }
    if (array_key_exists("not_timestamp", $row)) {
        $item["not_timestamp"] = $row["not_timestamp"];
    }
    if (array_key_exists("not_livello", $row)) {
        $item["not_livello"] = (int)$row["not_livello"];
    }
    if (array_key_exists("not_livelloStr", $row)) {
        $item["not_livelloStr"] = $row["not_livelloStr"];
    }
    if (array_key_exists("not_letto", $row)) {
        $item["not_letto"] = (bool)$row["not_letto"];
    }

    return $item;
}

if ($method === 'GET') {

    // GET singola notifica
    if (isset($_GET['notificaId'])) {
        $notId = (int)$_GET['notificaId'];
        $sql = "SELECT * FROM Notifica WHERE not_id = $notId";
        $result = $conn->query($sql);

        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();

            // Rilevazione associata (partial)
            $rilevazione = null;
            if (!empty($row["not_ril_id"])) {
                $rilId = (int)$row["not_ril_id"];
                $resRil = $conn->query("SELECT r.*, sa.sea_arn_id, tr.tip_tipologia, tr.tip_unita
                                        FROM Rilevazione r
                                        JOIN SensoreArnia sa ON r.ril_sea_id = sa.sea_id
                                        JOIN TipoRilevazione tr ON sa.sea_tip_id = tr.tip_id
                                        WHERE r.ril_id = $rilId");
                if ($resRil && $resRil->num_rows > 0) {
                    $r = $resRil->fetch_assoc();
                    $rilevazione = [
                        "ril_id" => (int)$r["ril_id"],
                        "ril_dato" => (float)$r["ril_dato"],
                        "ril_dataOra" => $r["ril_dataOra"],
                        "tip_tipologia" => $r["tip_tipologia"],
                        "tip_unita" => $r["tip_unita"],
                        "isPartial" => true,
                        "link" => "$baseUrl/rilevazioni/{$r["ril_id"]}"
                    ];
                }
            }

            $notifica = buildNotificaResponseRow($row, $baseUrl, $rilevazione);

            header('Content-Type: application/json');
            echo json_encode($notifica, JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Notifica non trovata"]);
        }
    }
    // GET tutte le notifiche
    else {
        $notifiche = [];
        $sql = "SELECT * FROM Notifica ORDER BY not_id DESC";
        $result = $conn->query($sql);

        while ($result && ($row = $result->fetch_assoc())) {
            // Rilevazione (partial)
            $rilevazione = null;
            if (!empty($row["not_ril_id"])) {
                $rilId = (int)$row["not_ril_id"];
                $resRil = $conn->query("SELECT r.*, tr.tip_tipologia, tr.tip_unita
                                        FROM Rilevazione r
                                        JOIN SensoreArnia sa ON r.ril_sea_id = sa.sea_id
                                        JOIN TipoRilevazione tr ON sa.sea_tip_id = tr.tip_id
                                        WHERE r.ril_id = $rilId");
                if ($resRil && $resRil->num_rows > 0) {
                    $r = $resRil->fetch_assoc();
                    $rilevazione = [
                        "ril_id" => (int)$r["ril_id"],
                        "ril_dato" => (float)$r["ril_dato"],
                        "ril_dataOra" => $r["ril_dataOra"],
                        "tip_tipologia" => $r["tip_tipologia"],
                        "isPartial" => true,
                        "link" => "$baseUrl/rilevazioni/{$r["ril_id"]}"
                    ];
                }
            }

            $notifiche[] = buildNotificaResponseRow($row, $baseUrl, $rilevazione);
        }

        header('Content-Type: application/json');
        echo json_encode($notifiche, JSON_PRETTY_PRINT);
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

    $rilId = array_key_exists('not_ril_id', $data) ? (is_null($data['not_ril_id']) ? null : (int)$data['not_ril_id']) : null;
    $titoloRaw = $data['not_titolo'] ?? null;
    $dexRaw = $data['not_dex'] ?? null;

    // Compatibilita' payload ESP
    $macRaw = $data['not_macAddress'] ?? ($data['macAddress'] ?? null);
    $tipoSensoreRaw = $data['not_tipoSensore'] ?? ($data['tipoSensore'] ?? null);
    $valoreRefRaw = $data['not_valoreRiferimento'] ?? ($data['valoreRiferimento'] ?? null);
    $timestampRaw = $data['not_timestamp'] ?? ($data['timestamp'] ?? null);
    $livelloRaw = $data['not_livello'] ?? ($data['livello'] ?? 1);
    $livelloStrRaw = $data['not_livelloStr'] ?? ($data['livelloStr'] ?? null);
    $lettoRaw = $data['not_letto'] ?? ($data['letto'] ?? false);
    $messaggioRaw = $data['messaggio'] ?? null;

    if ($titoloRaw === null || $titoloRaw === '') {
        $titoloRaw = $tipoSensoreRaw ? ("NOTIFICA " . strtoupper((string)$tipoSensoreRaw)) : "Notifica";
    }
    if ($dexRaw === null || $dexRaw === '') {
        $dexRaw = $messaggioRaw ?: "Notifica generata dal sistema";
    }

    $titolo = $conn->real_escape_string((string)$titoloRaw);
    $dex = $conn->real_escape_string((string)$dexRaw);
    $mac = $macRaw !== null ? $conn->real_escape_string((string)$macRaw) : null;
    $tipoSensore = $tipoSensoreRaw !== null ? $conn->real_escape_string((string)$tipoSensoreRaw) : null;
    $valoreRef = ($valoreRefRaw !== null && $valoreRefRaw !== '') ? (float)$valoreRefRaw : null;
    $timestampSql = toSqlDatetimeOrNull($timestampRaw);
    $livello = (int)$livelloRaw;
    $livelloStr = $livelloStrRaw ? $conn->real_escape_string((string)$livelloStrRaw) : notificaLevelToString($livello);
    $letto = (bool)$lettoRaw ? 1 : 0;

    $fields = [];
    $values = [];

    if (notificaColumnExists($conn, 'not_ril_id')) {
        $fields[] = 'not_ril_id';
        $values[] = ($rilId === null) ? 'NULL' : (string)$rilId;
    }

    $fields[] = 'not_titolo';
    $values[] = "'$titolo'";
    $fields[] = 'not_dex';
    $values[] = "'$dex'";

    if (notificaColumnExists($conn, 'not_macAddress')) {
        $fields[] = 'not_macAddress';
        $values[] = ($mac === null) ? 'NULL' : "'$mac'";
    }
    if (notificaColumnExists($conn, 'not_tipoSensore')) {
        $fields[] = 'not_tipoSensore';
        $values[] = ($tipoSensore === null) ? 'NULL' : "'$tipoSensore'";
    }
    if (notificaColumnExists($conn, 'not_valoreRiferimento')) {
        $fields[] = 'not_valoreRiferimento';
        $values[] = ($valoreRef === null) ? 'NULL' : (string)$valoreRef;
    }
    if (notificaColumnExists($conn, 'not_timestamp')) {
        $fields[] = 'not_timestamp';
        $values[] = ($timestampSql === null) ? 'NULL' : "'" . $conn->real_escape_string($timestampSql) . "'";
    }
    if (notificaColumnExists($conn, 'not_livello')) {
        $fields[] = 'not_livello';
        $values[] = (string)$livello;
    }
    if (notificaColumnExists($conn, 'not_livelloStr')) {
        $fields[] = 'not_livelloStr';
        $values[] = "'" . $conn->real_escape_string($livelloStr) . "'";
    }
    if (notificaColumnExists($conn, 'not_letto')) {
        $fields[] = 'not_letto';
        $values[] = (string)$letto;
    }

    $sql = "INSERT INTO Notifica (" . implode(", ", $fields) . ")
            VALUES (" . implode(", ", $values) . ")";

    if ($conn->query($sql)) {
        $newId = $conn->insert_id;
        http_response_code(201);
        header('Content-Type: application/json');
        echo json_encode(["not_id" => $newId, "link" => "$baseUrl/notifiche/$newId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'PUT') {
    if (!isset($_GET['notificaId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro notificaId mancante"]);
        exit;
    }

    $notId = (int)$_GET['notificaId'];
    $data = getRequestJsonBody();
    if (!is_array($data)) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido"]);
        $conn->close();
        exit;
    }

    $resExisting = $conn->query("SELECT * FROM Notifica WHERE not_id = $notId");
    if (!$resExisting || $resExisting->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Notifica non trovata"]);
        $conn->close();
        exit;
    }
    $existing = $resExisting->fetch_assoc();

    $rilId = array_key_exists('not_ril_id', $data)
        ? (is_null($data['not_ril_id']) ? null : (int)$data['not_ril_id'])
        : (array_key_exists('not_ril_id', $existing) ? (is_null($existing['not_ril_id']) ? null : (int)$existing['not_ril_id']) : null);

    $titoloRaw = $data['not_titolo'] ?? $existing['not_titolo'];
    $dexRaw = $data['not_dex'] ?? $existing['not_dex'];

    $macRaw = $data['not_macAddress'] ?? ($data['macAddress'] ?? ($existing['not_macAddress'] ?? null));
    $tipoSensoreRaw = $data['not_tipoSensore'] ?? ($data['tipoSensore'] ?? ($existing['not_tipoSensore'] ?? null));
    $valoreRefRaw = $data['not_valoreRiferimento'] ?? ($data['valoreRiferimento'] ?? ($existing['not_valoreRiferimento'] ?? null));
    $timestampRaw = $data['not_timestamp'] ?? ($data['timestamp'] ?? ($existing['not_timestamp'] ?? null));
    $livelloRaw = $data['not_livello'] ?? ($data['livello'] ?? ($existing['not_livello'] ?? 1));
    $livelloStrRaw = $data['not_livelloStr'] ?? ($data['livelloStr'] ?? ($existing['not_livelloStr'] ?? null));
    $lettoRaw = $data['not_letto'] ?? ($data['letto'] ?? ($existing['not_letto'] ?? false));
    $messaggioRaw = $data['messaggio'] ?? null;

    if (($titoloRaw === null || $titoloRaw === '') && $tipoSensoreRaw) {
        $titoloRaw = "NOTIFICA " . strtoupper((string)$tipoSensoreRaw);
    }
    if (($dexRaw === null || $dexRaw === '') && $messaggioRaw) {
        $dexRaw = $messaggioRaw;
    }

    $titolo = $conn->real_escape_string((string)$titoloRaw);
    $dex = $conn->real_escape_string((string)$dexRaw);
    $mac = $macRaw !== null ? $conn->real_escape_string((string)$macRaw) : null;
    $tipoSensore = $tipoSensoreRaw !== null ? $conn->real_escape_string((string)$tipoSensoreRaw) : null;
    $valoreRef = ($valoreRefRaw !== null && $valoreRefRaw !== '') ? (float)$valoreRefRaw : null;
    $timestampSql = toSqlDatetimeOrNull($timestampRaw);
    $livello = (int)$livelloRaw;
    $livelloStr = $livelloStrRaw ? $conn->real_escape_string((string)$livelloStrRaw) : notificaLevelToString($livello);
    $letto = (bool)$lettoRaw ? 1 : 0;

    $set = [];
    if (notificaColumnExists($conn, 'not_ril_id')) {
        $set[] = "not_ril_id = " . (($rilId === null) ? "NULL" : (string)$rilId);
    }
    $set[] = "not_titolo = '$titolo'";
    $set[] = "not_dex = '$dex'";

    if (notificaColumnExists($conn, 'not_macAddress')) {
        $set[] = "not_macAddress = " . (($mac === null) ? "NULL" : "'$mac'");
    }
    if (notificaColumnExists($conn, 'not_tipoSensore')) {
        $set[] = "not_tipoSensore = " . (($tipoSensore === null) ? "NULL" : "'$tipoSensore'");
    }
    if (notificaColumnExists($conn, 'not_valoreRiferimento')) {
        $set[] = "not_valoreRiferimento = " . (($valoreRef === null) ? "NULL" : (string)$valoreRef);
    }
    if (notificaColumnExists($conn, 'not_timestamp')) {
        $set[] = "not_timestamp = " . (($timestampSql === null) ? "NULL" : "'" . $conn->real_escape_string($timestampSql) . "'");
    }
    if (notificaColumnExists($conn, 'not_livello')) {
        $set[] = "not_livello = $livello";
    }
    if (notificaColumnExists($conn, 'not_livelloStr')) {
        $set[] = "not_livelloStr = '" . $conn->real_escape_string($livelloStr) . "'";
    }
    if (notificaColumnExists($conn, 'not_letto')) {
        $set[] = "not_letto = $letto";
    }

    $sql = "UPDATE Notifica SET " . implode(", ", $set) . " WHERE not_id = $notId";

    if ($conn->query($sql)) {
        header('Content-Type: application/json');
        echo json_encode(["messaggio" => "Notifica aggiornata", "link" => "$baseUrl/notifiche/$notId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'DELETE') {
    if (!isset($_GET['notificaId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro notificaId mancante"]);
        exit;
    }

    $notId = (int)$_GET['notificaId'];

    $check = $conn->query("SELECT not_id FROM Notifica WHERE not_id = $notId");
    if (!$check || $check->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Notifica non trovata"]);
        exit;
    }

    if ($conn->query("DELETE FROM Notifica WHERE not_id = $notId")) {
        http_response_code(204);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

$conn->close();

