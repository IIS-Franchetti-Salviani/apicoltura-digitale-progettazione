<?php
// =============================================================
// Endpoint: /sensori
// GET /sensori         → Lista tutti i modelli di sensore con tipi rilevazione (partial)
// GET /sensori/{id}    → Dettaglio singolo sensore
// POST /sensori        → Crea un nuovo modello di sensore
// PUT /sensori/{id}    → Aggiorna un sensore esistente
// DELETE /sensori/{id} → Elimina un sensore
// =============================================================

require_once __DIR__ . '/../config.php';

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'GET') {

    // GET singolo sensore
    if (isset($_GET['sensoreId'])) {
        $sensoreId = (int)$_GET['sensoreId'];
        $sql = "SELECT * FROM Sensore WHERE sen_id = $sensoreId";
        $result = $conn->query($sql);

        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();

            // Tipi rilevazione associati
            $tipi = [];
            $resTip = $conn->query("SELECT * FROM TipoRilevazione WHERE tip_sen_id = $sensoreId");
            while ($t = $resTip->fetch_assoc()) {
                $tipi[] = [
                    "tip_id" => (int)$t["tip_id"],
                    "tip_tipologia" => $t["tip_tipologia"],
                    "tip_unita" => $t["tip_unita"],
                    "isPartial" => true,
                    "link" => "$baseUrl/tipirilevazione/{$t["tip_id"]}"
                ];
            }

            $sensore = [
                "sen_id" => (int)$row["sen_id"],
                "sen_modello" => $row["sen_modello"],
                "tipiRilevazione" => $tipi,
                "link" => "$baseUrl/sensori/{$row["sen_id"]}"
            ];

            header('Content-Type: application/json');
            echo json_encode($sensore, JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Sensore non trovato"]);
        }
    }
    // GET tutti i sensori
    else {
        $sensori = [];
        $sql = "SELECT * FROM Sensore";
        $result = $conn->query($sql);

        while ($row = $result->fetch_assoc()) {
            $tipi = [];
            $resTip = $conn->query("SELECT * FROM TipoRilevazione WHERE tip_sen_id = {$row["sen_id"]}");
            while ($t = $resTip->fetch_assoc()) {
                $tipi[] = [
                    "tip_id" => (int)$t["tip_id"],
                    "tip_tipologia" => $t["tip_tipologia"],
                    "isPartial" => true,
                    "link" => "$baseUrl/tipirilevazione/{$t["tip_id"]}"
                ];
            }

            $sensori[] = [
                "sen_id" => (int)$row["sen_id"],
                "sen_modello" => $row["sen_modello"],
                "tipiRilevazione" => $tipi,
                "link" => "$baseUrl/sensori/{$row["sen_id"]}"
            ];
        }

        header('Content-Type: application/json');
        echo json_encode($sensori, JSON_PRETTY_PRINT);
    }
}

elseif ($method === 'POST') {
    $data = getRequestJsonBody();
    if (!is_array($data) || !isset($data['sen_modello'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido o campo mancante: sen_modello"]);
        $conn->close();
        exit;
    }

    $modello = $conn->real_escape_string($data['sen_modello']);

    $sql = "INSERT INTO Sensore (sen_modello) VALUES ('$modello')";

    if ($conn->query($sql)) {
        $newId = $conn->insert_id;
        http_response_code(201);
        header('Content-Type: application/json');
        echo json_encode(["sen_id" => $newId, "link" => "$baseUrl/sensori/$newId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'PUT') {
    if (!isset($_GET['sensoreId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro sensoreId mancante"]);
        exit;
    }

    $sensoreId = (int)$_GET['sensoreId'];
    $data = getRequestJsonBody();
    if (!is_array($data) || !isset($data['sen_modello'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido o campo mancante: sen_modello"]);
        $conn->close();
        exit;
    }

    $modello = $conn->real_escape_string($data['sen_modello']);

    $sql = "UPDATE Sensore SET sen_modello = '$modello' WHERE sen_id = $sensoreId";

    if ($conn->query($sql)) {
        header('Content-Type: application/json');
        echo json_encode(["messaggio" => "Sensore aggiornato", "link" => "$baseUrl/sensori/$sensoreId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'DELETE') {
    if (!isset($_GET['sensoreId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro sensoreId mancante"]);
        exit;
    }

    $sensoreId = (int)$_GET['sensoreId'];

    $check = $conn->query("SELECT sen_id FROM Sensore WHERE sen_id = $sensoreId");
    if ($check->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Sensore non trovato"]);
        exit;
    }

    if ($conn->query("DELETE FROM Sensore WHERE sen_id = $sensoreId")) {
        http_response_code(204);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

$conn->close();
