<?php
// =============================================================
// Endpoint: /rilevazioni
// GET /rilevazioni         → Lista tutte le rilevazioni con sensore arnia (partial)
// GET /rilevazioni/{id}    → Dettaglio singola rilevazione
// POST /rilevazioni        → Crea una nuova rilevazione (usato dall'ESP32)
// PUT /rilevazioni/{id}    → Aggiorna una rilevazione
// DELETE /rilevazioni/{id} → Elimina una rilevazione
// =============================================================

require_once __DIR__ . '/../config.php';

$method = $_SERVER['REQUEST_METHOD'];

function parseRilevazioneDateTime($value) {
    if ($value === null || $value === '') {
        return null;
    }

    if (is_numeric($value)) {
        $ts = (int)$value;
        if ($ts > 1000000000000) {
            $ts = (int)round($ts / 1000);
        }
        if ($ts <= 0) {
            return null;
        }
        return date('Y-m-d H:i:s', $ts);
    }

    $parsed = strtotime((string)$value);
    if ($parsed === false) {
        return null;
    }
    return date('Y-m-d H:i:s', $parsed);
}

if ($method === 'GET') {

    // GET singola rilevazione
    if (isset($_GET['rilevazioneId'])) {
        $rilId = (int)$_GET['rilevazioneId'];
        $sql = "SELECT * FROM Rilevazione WHERE ril_id = $rilId";
        $result = $conn->query($sql);

        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();

            // Sensore arnia (partial)
            $sensoreArnia = null;
            $resSea = $conn->query("SELECT sa.*, tr.tip_tipologia, tr.tip_unita
                                    FROM SensoreArnia sa
                                    JOIN TipoRilevazione tr ON sa.sea_tip_id = tr.tip_id
                                    WHERE sa.sea_id = {$row["ril_sea_id"]}");
            if ($resSea->num_rows > 0) {
                $s = $resSea->fetch_assoc();
                $sensoreArnia = [
                    "sea_id" => (int)$s["sea_id"],
                    "sea_arn_id" => (int)$s["sea_arn_id"],
                    "tip_tipologia" => $s["tip_tipologia"],
                    "tip_unita" => $s["tip_unita"],
                    "isPartial" => true,
                    "link" => "$baseUrl/sensoriarnia/{$s["sea_id"]}"
                ];
            }

            // Notifiche associate (partial)
            $notifiche = [];
            $resNot = $conn->query("SELECT * FROM Notifica WHERE not_ril_id = $rilId");
            while ($n = $resNot->fetch_assoc()) {
                $notifiche[] = [
                    "not_id" => (int)$n["not_id"],
                    "not_titolo" => $n["not_titolo"],
                    "isPartial" => true,
                    "link" => "$baseUrl/notifiche/{$n["not_id"]}"
                ];
            }

            $rilevazione = [
                "ril_id" => (int)$row["ril_id"],
                "ril_sea_id" => (int)$row["ril_sea_id"],
                "ril_dato" => (float)$row["ril_dato"],
                "ril_dataOra" => $row["ril_dataOra"],
                "sensoreArnia" => $sensoreArnia,
                "notifiche" => $notifiche,
                "link" => "$baseUrl/rilevazioni/{$row["ril_id"]}"
            ];

            header('Content-Type: application/json');
            echo json_encode($rilevazione, JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Rilevazione non trovata"]);
        }
    }
    // GET tutte le rilevazioni
    else {
        $queryMeta = restGetCollectionQuery($conn, 'rilevazioni');
        if (!$queryMeta['ok']) {
            http_response_code($queryMeta['status']);
            header('Content-Type: application/json');
            echo json_encode(["errore" => $queryMeta['errore']]);
            $conn->close();
            exit;
        }

        $rilevazioni = [];
        $rows = $queryMeta['rows'];

        foreach ($rows as $row) {
            // Sensore arnia (partial)
            $sensoreArnia = null;
            $resSea = $conn->query("SELECT sa.*, tr.tip_tipologia, tr.tip_unita
                                    FROM SensoreArnia sa
                                    JOIN TipoRilevazione tr ON sa.sea_tip_id = tr.tip_id
                                    WHERE sa.sea_id = {$row["ril_sea_id"]}");
            if ($resSea->num_rows > 0) {
                $s = $resSea->fetch_assoc();
                $sensoreArnia = [
                    "sea_id" => (int)$s["sea_id"],
                    "tip_tipologia" => $s["tip_tipologia"],
                    "tip_unita" => $s["tip_unita"],
                    "isPartial" => true,
                    "link" => "$baseUrl/sensoriarnia/{$s["sea_id"]}"
                ];
            }

            $rilevazioni[] = [
                "ril_id" => (int)$row["ril_id"],
                "ril_sea_id" => (int)$row["ril_sea_id"],
                "ril_dato" => (float)$row["ril_dato"],
                "ril_dataOra" => $row["ril_dataOra"],
                "sensoreArnia" => $sensoreArnia,
                "link" => "$baseUrl/rilevazioni/{$row["ril_id"]}"
            ];
        }

        restSendCollectionResponse($rilevazioni, $queryMeta);
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
    if (!isset($data['ril_sea_id'], $data['ril_dato'], $data['ril_dataOra'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Campi obbligatori: ril_sea_id, ril_dato, ril_dataOra"]);
        $conn->close();
        exit;
    }

    $seaId = (int)$data['ril_sea_id'];
    $dato = (float)$data['ril_dato'];
    $parsedDate = parseRilevazioneDateTime($data['ril_dataOra']);
    if ($parsedDate === null) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "ril_dataOra non valido"]);
        $conn->close();
        exit;
    }
    $dataOra = $conn->real_escape_string($parsedDate);

    $sql = "INSERT INTO Rilevazione (ril_sea_id, ril_dato, ril_dataOra)
            VALUES ($seaId, $dato, '$dataOra')";

    if ($conn->query($sql)) {
        $newId = $conn->insert_id;
        http_response_code(201);
        header('Content-Type: application/json');
        echo json_encode(["ril_id" => $newId, "link" => "$baseUrl/rilevazioni/$newId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'PUT') {
    if (!isset($_GET['rilevazioneId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro rilevazioneId mancante"]);
        exit;
    }

    $rilId = (int)$_GET['rilevazioneId'];
    $data = getRequestJsonBody();
    if (!is_array($data)) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido"]);
        $conn->close();
        exit;
    }
    if (!isset($data['ril_sea_id'], $data['ril_dato'], $data['ril_dataOra'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Campi obbligatori: ril_sea_id, ril_dato, ril_dataOra"]);
        $conn->close();
        exit;
    }

    $seaId = (int)$data['ril_sea_id'];
    $dato = (float)$data['ril_dato'];
    $parsedDate = parseRilevazioneDateTime($data['ril_dataOra']);
    if ($parsedDate === null) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "ril_dataOra non valido"]);
        $conn->close();
        exit;
    }
    $dataOra = $conn->real_escape_string($parsedDate);

    $sql = "UPDATE Rilevazione SET ril_sea_id = $seaId, ril_dato = $dato,
            ril_dataOra = '$dataOra' WHERE ril_id = $rilId";

    if ($conn->query($sql)) {
        header('Content-Type: application/json');
        echo json_encode(["messaggio" => "Rilevazione aggiornata", "link" => "$baseUrl/rilevazioni/$rilId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'DELETE') {
    if (!isset($_GET['rilevazioneId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro rilevazioneId mancante"]);
        exit;
    }

    $rilId = (int)$_GET['rilevazioneId'];

    $check = $conn->query("SELECT ril_id FROM Rilevazione WHERE ril_id = $rilId");
    if ($check->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Rilevazione non trovata"]);
        exit;
    }

    if ($conn->query("DELETE FROM Rilevazione WHERE ril_id = $rilId")) {
        http_response_code(204);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

$conn->close();
