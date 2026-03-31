<?php
// =============================================================
// Endpoint: /arnie
// GET /arnie         → Lista tutte le arnie con apiario e sensori (partial)
// GET /arnie/{id}    → Dettaglio singola arnia
// POST /arnie        → Crea una nuova arnia
// PUT /arnie/{id}    → Aggiorna un'arnia esistente
// DELETE /arnie/{id} → Elimina un'arnia
// =============================================================

require_once __DIR__ . '/../config.php';

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'GET') {

    // GET singola arnia
    if (isset($_GET['arniaId'])) {
        $arniaId = (int)$_GET['arniaId'];
        $sql = "SELECT * FROM Arnia WHERE arn_id = $arniaId";
        $result = $conn->query($sql);

        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();

            // Apiario (partial)
            $apiario = null;
            $resApi = $conn->query("SELECT * FROM Apiario WHERE api_id = {$row["arn_api_id"]}");
            if ($resApi->num_rows > 0) {
                $a = $resApi->fetch_assoc();
                $apiario = [
                    "api_nome" => $a["api_nome"],
                    "isPartial" => true,
                    "link" => "$baseUrl/apiari/{$a["api_id"]}"
                ];
            }

            // Sensori arnia (partial)
            $sensori = [];
            $resSea = $conn->query("SELECT * FROM SensoreArnia WHERE sea_arn_id = $arniaId");
            while ($s = $resSea->fetch_assoc()) {
                $sensori[] = [
                    "sea_id" => (int)$s["sea_id"],
                    "sea_stato" => (bool)$s["sea_stato"],
                    "isPartial" => true,
                    "link" => "$baseUrl/sensoriarnia/{$s["sea_id"]}"
                ];
            }

            $arnia = [
                "arn_id" => (int)$row["arn_id"],
                "arn_api_id" => (int)$row["arn_api_id"],
                "arn_dataInst" => $row["arn_dataInst"],
                "arn_piena" => (bool)$row["arn_piena"],
                "arn_MacAddress" => $row["arn_MacAddress"],
                "apiario" => $apiario,
                "sensoriArnia" => $sensori,
                "link" => "$baseUrl/arnie/{$row["arn_id"]}"
            ];

            header('Content-Type: application/json');
            echo json_encode($arnia, JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Arnia non trovata"]);
        }
    }
    // GET tutte le arnie
    else {
        $queryMeta = restGetCollectionQuery($conn, 'arnie');
        if (!$queryMeta['ok']) {
            http_response_code($queryMeta['status']);
            header('Content-Type: application/json');
            echo json_encode(["errore" => $queryMeta['errore']]);
            $conn->close();
            exit;
        }

        $arnie = [];
        $rows = $queryMeta['rows'];

        foreach ($rows as $row) {
            // Apiario (partial)
            $apiario = null;
            $resApi = $conn->query("SELECT * FROM Apiario WHERE api_id = {$row["arn_api_id"]}");
            if ($resApi->num_rows > 0) {
                $a = $resApi->fetch_assoc();
                $apiario = [
                    "api_nome" => $a["api_nome"],
                    "isPartial" => true,
                    "link" => "$baseUrl/apiari/{$a["api_id"]}"
                ];
            }

            // Sensori arnia (partial)
            $sensori = [];
            $resSea = $conn->query("SELECT * FROM SensoreArnia WHERE sea_arn_id = {$row["arn_id"]}");
            while ($s = $resSea->fetch_assoc()) {
                $sensori[] = [
                    "sea_id" => (int)$s["sea_id"],
                    "sea_stato" => (bool)$s["sea_stato"],
                    "isPartial" => true,
                    "link" => "$baseUrl/sensoriarnia/{$s["sea_id"]}"
                ];
            }

            $arnie[] = [
                "arn_id" => (int)$row["arn_id"],
                "arn_api_id" => (int)$row["arn_api_id"],
                "arn_dataInst" => $row["arn_dataInst"],
                "arn_piena" => (bool)$row["arn_piena"],
                "arn_MacAddress" => $row["arn_MacAddress"],
                "apiario" => $apiario,
                "sensoriArnia" => $sensori,
                "link" => "$baseUrl/arnie/{$row["arn_id"]}"
            ];
        }

        restSendCollectionResponse($arnie, $queryMeta);
    }
}

elseif ($method === 'POST') {
    $data = getRequestJsonBody();
    if (!is_array($data) || !isset($data['arn_api_id'], $data['arn_dataInst'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido o campi mancanti: arn_api_id, arn_dataInst"]);
        $conn->close();
        exit;
    }

    $apiId = (int)$data['arn_api_id'];
    $dataInst = $conn->real_escape_string($data['arn_dataInst']);
    $piena = isset($data['arn_piena']) ? ($data['arn_piena'] ? 1 : 0) : 0;
    $mac = isset($data['arn_MacAddress']) ? "'" . $conn->real_escape_string($data['arn_MacAddress']) . "'" : 'NULL';

    $sql = "INSERT INTO Arnia (arn_api_id, arn_dataInst, arn_piena, arn_MacAddress)
            VALUES ($apiId, '$dataInst', $piena, $mac)";

    if ($conn->query($sql)) {
        $newId = $conn->insert_id;
        http_response_code(201);
        header('Content-Type: application/json');
        echo json_encode(["arn_id" => $newId, "link" => "$baseUrl/arnie/$newId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'PUT') {
    if (!isset($_GET['arniaId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro arniaId mancante"]);
        exit;
    }

    $arniaId = (int)$_GET['arniaId'];
    $data = getRequestJsonBody();
    if (!is_array($data) || !isset($data['arn_api_id'], $data['arn_dataInst'], $data['arn_piena'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido o campi mancanti: arn_api_id, arn_dataInst, arn_piena"]);
        $conn->close();
        exit;
    }

    $apiId = (int)$data['arn_api_id'];
    $dataInst = $conn->real_escape_string($data['arn_dataInst']);
    $piena = $data['arn_piena'] ? 1 : 0;
    $mac = isset($data['arn_MacAddress']) ? "'" . $conn->real_escape_string($data['arn_MacAddress']) . "'" : 'NULL';

    $sql = "UPDATE Arnia SET arn_api_id = $apiId, arn_dataInst = '$dataInst',
            arn_piena = $piena, arn_MacAddress = $mac WHERE arn_id = $arniaId";

    if ($conn->query($sql)) {
        header('Content-Type: application/json');
        echo json_encode(["messaggio" => "Arnia aggiornata", "link" => "$baseUrl/arnie/$arniaId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'DELETE') {
    if (!isset($_GET['arniaId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro arniaId mancante"]);
        exit;
    }

    $arniaId = (int)$_GET['arniaId'];

    $check = $conn->query("SELECT arn_id FROM Arnia WHERE arn_id = $arniaId");
    if ($check->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Arnia non trovata"]);
        exit;
    }

    if ($conn->query("DELETE FROM Arnia WHERE arn_id = $arniaId")) {
        http_response_code(204);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

$conn->close();
