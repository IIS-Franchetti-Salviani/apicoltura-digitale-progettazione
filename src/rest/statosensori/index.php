<?php
// =============================================================
// Endpoint: /statosensori
// GET /statosensori           -> Lista stati invio sensori
// GET /statosensori/{id}      -> Stato singolo
// POST /statosensori          -> Inserisce stato/causa invio dal firmware
// =============================================================

require_once __DIR__ . '/../config.php';

$method = $_SERVER['REQUEST_METHOD'];

function statoSensoreBoolToInt($value) {
    return $value ? 1 : 0;
}

function statoSensoreToSqlDatetime($value) {
    $now = time();
    $minAccepted = strtotime('2025-01-01 00:00:00');
    $maxAccepted = $now + (30 * 24 * 60 * 60);

    if ($value === null || $value === '') {
        return date('Y-m-d H:i:s', $now);
    }

    if (is_numeric($value)) {
        $ts = (int)$value;
        if ($ts > 1000000000000) {
            $ts = (int)round($ts / 1000);
        }
        if ($ts < $minAccepted || $ts > $maxAccepted) {
            return date('Y-m-d H:i:s', $now);
        }
        return date('Y-m-d H:i:s', $ts);
    }

    $parsed = strtotime((string)$value);
    if ($parsed === false || $parsed < $minAccepted || $parsed > $maxAccepted) {
        return date('Y-m-d H:i:s', $now);
    }
    return date('Y-m-d H:i:s', $parsed);
}

if ($method === 'GET') {
    if (isset($_GET['statoSensoreId'])) {
        $stsId = (int)$_GET['statoSensoreId'];
        $result = $conn->query("SELECT * FROM StatoInvioSensore WHERE sts_id = $stsId LIMIT 1");

        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();
            $row['sts_id'] = (int)$row['sts_id'];
            $row['sts_abilitato'] = (bool)$row['sts_abilitato'];
            $row['sts_codiceStato'] = is_null($row['sts_codiceStato']) ? null : (int)$row['sts_codiceStato'];
            $row['sts_valore'] = is_null($row['sts_valore']) ? null : (float)$row['sts_valore'];
            $row['link'] = "$baseUrl/statosensori/{$row['sts_id']}";

            header('Content-Type: application/json');
            echo json_encode($row, JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Stato sensore non trovato"]);
        }
    } else {
        $queryMeta = restGetCollectionQuery($conn, 'statosensori');
        if (!$queryMeta['ok']) {
            http_response_code($queryMeta['status']);
            header('Content-Type: application/json');
            echo json_encode(["errore" => $queryMeta['errore']]);
            $conn->close();
            exit;
        }

        $rows = [];
        foreach ($queryMeta['rows'] as $row) {
            $rows[] = [
                "sts_id" => (int)$row['sts_id'],
                "sts_macAddress" => $row['sts_macAddress'],
                "sts_tipoSensore" => $row['sts_tipoSensore'],
                "sts_sensorId" => $row['sts_sensorId'],
                "sts_abilitato" => (bool)$row['sts_abilitato'],
                "sts_evento" => $row['sts_evento'],
                "sts_causaCodice" => $row['sts_causaCodice'],
                "sts_causaDettaglio" => $row['sts_causaDettaglio'],
                "sts_codiceStato" => is_null($row['sts_codiceStato']) ? null : (int)$row['sts_codiceStato'],
                "sts_valore" => is_null($row['sts_valore']) ? null : (float)$row['sts_valore'],
                "sts_timestamp" => $row['sts_timestamp'],
                "sts_creato_at" => $row['sts_creato_at'],
                "link" => "$baseUrl/statosensori/{$row['sts_id']}"
            ];
        }

        restSendCollectionResponse($rows, $queryMeta);
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

    $macRaw = $data['sts_macAddress'] ?? ($data['macAddress'] ?? null);
    $tipoRaw = $data['sts_tipoSensore'] ?? ($data['tipoSensore'] ?? null);

    if (!is_string($macRaw) || trim($macRaw) === '' || !is_string($tipoRaw) || trim($tipoRaw) === '') {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Campi obbligatori: sts_macAddress/macAddress, sts_tipoSensore/tipoSensore"]);
        $conn->close();
        exit;
    }

    $eventoRaw = $data['sts_evento'] ?? ($data['evento'] ?? 'CONFIG_SYNC');
    $evento = strtoupper(trim((string)$eventoRaw));
    $eventiAmmessi = ['CONFIG_SYNC', 'INVIO_OK', 'INVIO_BLOCCATO', 'LETTURA_NON_VALIDA', 'WIFI_OFFLINE', 'ERRORE_SERVER'];
    if (!in_array($evento, $eventiAmmessi, true)) {
        $evento = 'CONFIG_SYNC';
    }

    $sensorIdRaw = $data['sts_sensorId'] ?? ($data['sensorId'] ?? null);
    $abilitatoRaw = $data['sts_abilitato'] ?? ($data['abilitato'] ?? true);
    $causaCodiceRaw = $data['sts_causaCodice'] ?? ($data['causaCodice'] ?? null);
    $causaDettaglioRaw = $data['sts_causaDettaglio'] ?? ($data['causaDettaglio'] ?? null);
    $codiceStatoRaw = $data['sts_codiceStato'] ?? ($data['codiceStato'] ?? null);
    $valoreRaw = $data['sts_valore'] ?? ($data['valore'] ?? null);
    $timestampRaw = $data['sts_timestamp'] ?? ($data['timestamp'] ?? null);

    $mac = $conn->real_escape_string(strtoupper(trim((string)$macRaw)));
    $tipo = $conn->real_escape_string(trim((string)$tipoRaw));
    $sensorId = ($sensorIdRaw === null || trim((string)$sensorIdRaw) === '') ? null : $conn->real_escape_string(trim((string)$sensorIdRaw));
    $abilitato = statoSensoreBoolToInt((bool)$abilitatoRaw);
    $causaCodice = ($causaCodiceRaw === null || trim((string)$causaCodiceRaw) === '') ? null : $conn->real_escape_string(trim((string)$causaCodiceRaw));
    $causaDettaglio = ($causaDettaglioRaw === null || trim((string)$causaDettaglioRaw) === '') ? null : $conn->real_escape_string(trim((string)$causaDettaglioRaw));
    $codiceStato = ($codiceStatoRaw === null || $codiceStatoRaw === '') ? null : (int)$codiceStatoRaw;
    $valore = ($valoreRaw === null || $valoreRaw === '') ? null : (float)$valoreRaw;
    $timestamp = $conn->real_escape_string(statoSensoreToSqlDatetime($timestampRaw));

    $sql = "INSERT INTO StatoInvioSensore (
                sts_macAddress, sts_tipoSensore, sts_sensorId, sts_abilitato, sts_evento,
                sts_causaCodice, sts_causaDettaglio, sts_codiceStato, sts_valore, sts_timestamp
            ) VALUES (
                '$mac',
                '$tipo',
                " . ($sensorId === null ? "NULL" : "'$sensorId'") . ",
                $abilitato,
                '" . $conn->real_escape_string($evento) . "',
                " . ($causaCodice === null ? "NULL" : "'$causaCodice'") . ",
                " . ($causaDettaglio === null ? "NULL" : "'$causaDettaglio'") . ",
                " . ($codiceStato === null ? "NULL" : (string)$codiceStato) . ",
                " . ($valore === null ? "NULL" : (string)$valore) . ",
                '$timestamp'
            )";

    if ($conn->query($sql)) {
        $newId = (int)$conn->insert_id;
        http_response_code(201);
        header('Content-Type: application/json');
        echo json_encode(["sts_id" => $newId, "link" => "$baseUrl/statosensori/$newId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

else {
    http_response_code(405);
    header('Content-Type: application/json');
    echo json_encode(["errore" => "Metodo non supportato"]);
}

$conn->close();
