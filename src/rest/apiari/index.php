<?php
// =============================================================
// Endpoint: /apiari
// GET /apiari         → Lista tutti gli apiari con arnie (partial)
// GET /apiari/{id}    → Dettaglio singolo apiario con arnie
// POST /apiari        → Crea un nuovo apiario
// PUT /apiari/{id}    → Aggiorna un apiario esistente
// DELETE /apiari/{id} → Elimina un apiario
// =============================================================

require_once __DIR__ . '/../config.php';

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'GET') {

    // GET singolo apiario
    if (isset($_GET['apiarioId'])) {
        $apiarioId = (int)$_GET['apiarioId'];
        $sql = "SELECT * FROM Apiario WHERE api_id = $apiarioId";
        $result = $conn->query($sql);

        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();

            // Recupera arnie associate
            $arnie = [];
            $resArnie = $conn->query("SELECT * FROM Arnia WHERE arn_api_id = $apiarioId");
            while ($a = $resArnie->fetch_assoc()) {
                $arnie[] = [
                    "arn_id" => (int)$a["arn_id"],
                    "arn_piena" => (bool)$a["arn_piena"],
                    "arn_MacAddress" => $a["arn_MacAddress"],
                    "isPartial" => true,
                    "link" => "$baseUrl/arnie/{$a["arn_id"]}"
                ];
            }

            $apiario = [
                "api_id" => (int)$row["api_id"],
                "api_nome" => $row["api_nome"],
                "api_luogo" => $row["api_luogo"],
                "api_lon" => (float)$row["api_lon"],
                "api_lat" => (float)$row["api_lat"],
                "arnie" => $arnie,
                "link" => "$baseUrl/apiari/{$row["api_id"]}"
            ];

            header('Content-Type: application/json');
            echo json_encode($apiario, JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Apiario non trovato"]);
        }
    }
    // GET tutti gli apiari
    else {
        $queryMeta = restGetCollectionQuery($conn, 'apiari');
        if (!$queryMeta['ok']) {
            http_response_code($queryMeta['status']);
            header('Content-Type: application/json');
            echo json_encode(["errore" => $queryMeta['errore']]);
            $conn->close();
            exit;
        }

        $apiari = [];
        $rows = $queryMeta['rows'];

        foreach ($rows as $row) {
            $arnie = [];
            $resArnie = $conn->query("SELECT * FROM Arnia WHERE arn_api_id = {$row["api_id"]}");
            while ($a = $resArnie->fetch_assoc()) {
                $arnie[] = [
                    "arn_id" => (int)$a["arn_id"],
                    "arn_piena" => (bool)$a["arn_piena"],
                    "isPartial" => true,
                    "link" => "$baseUrl/arnie/{$a["arn_id"]}"
                ];
            }

            $apiari[] = [
                "api_id" => (int)$row["api_id"],
                "api_nome" => $row["api_nome"],
                "api_luogo" => $row["api_luogo"],
                "api_lon" => (float)$row["api_lon"],
                "api_lat" => (float)$row["api_lat"],
                "arnie" => $arnie,
                "link" => "$baseUrl/apiari/{$row["api_id"]}"
            ];
        }

        restSendCollectionResponse($apiari, $queryMeta);
    }
}

elseif ($method === 'POST') {
    $data = getRequestJsonBody();
    if (!is_array($data) || !isset($data['api_nome'], $data['api_luogo'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido o campi mancanti: api_nome, api_luogo"]);
        $conn->close();
        exit;
    }

    $nome = $conn->real_escape_string($data['api_nome']);
    $luogo = $conn->real_escape_string($data['api_luogo']);
    $lon = isset($data['api_lon']) ? (float)$data['api_lon'] : 'NULL';
    $lat = isset($data['api_lat']) ? (float)$data['api_lat'] : 'NULL';

    $sql = "INSERT INTO Apiario (api_nome, api_luogo, api_lon, api_lat)
            VALUES ('$nome', '$luogo', $lon, $lat)";

    if ($conn->query($sql)) {
        $newId = $conn->insert_id;
        http_response_code(201);
        header('Content-Type: application/json');
        echo json_encode(["api_id" => $newId, "link" => "$baseUrl/apiari/$newId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'PUT') {
    if (!isset($_GET['apiarioId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro apiarioId mancante"]);
        exit;
    }

    $apiarioId = (int)$_GET['apiarioId'];
    $data = getRequestJsonBody();
    if (!is_array($data) || !isset($data['api_nome'], $data['api_luogo'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido o campi mancanti: api_nome, api_luogo"]);
        $conn->close();
        exit;
    }

    $nome = $conn->real_escape_string($data['api_nome']);
    $luogo = $conn->real_escape_string($data['api_luogo']);
    $lon = isset($data['api_lon']) ? (float)$data['api_lon'] : 'NULL';
    $lat = isset($data['api_lat']) ? (float)$data['api_lat'] : 'NULL';

    $sql = "UPDATE Apiario SET api_nome = '$nome', api_luogo = '$luogo',
            api_lon = $lon, api_lat = $lat WHERE api_id = $apiarioId";

    if ($conn->query($sql)) {
        header('Content-Type: application/json');
        echo json_encode(["messaggio" => "Apiario aggiornato", "link" => "$baseUrl/apiari/$apiarioId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'DELETE') {
    if (!isset($_GET['apiarioId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro apiarioId mancante"]);
        exit;
    }

    $apiarioId = (int)$_GET['apiarioId'];

    $check = $conn->query("SELECT api_id FROM Apiario WHERE api_id = $apiarioId");
    if ($check->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Apiario non trovato"]);
        exit;
    }

    if ($conn->query("DELETE FROM Apiario WHERE api_id = $apiarioId")) {
        http_response_code(204);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

$conn->close();
