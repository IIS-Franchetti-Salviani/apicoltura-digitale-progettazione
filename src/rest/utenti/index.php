<?php
// =============================================================
// Endpoint: /utenti
// GET /utenti         → Lista tutti gli utenti (senza password)
// GET /utenti/{id}    → Dettaglio singolo utente (senza password)
// POST /utenti        → Crea un nuovo utente
// PUT /utenti/{id}    → Aggiorna un utente
// DELETE /utenti/{id} → Elimina un utente
// =============================================================

require_once __DIR__ . '/../config.php';

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'GET') {

    // GET singolo utente
    if (isset($_GET['utenteId'])) {
        $uteId = (int)$_GET['utenteId'];
        $sql = "SELECT ute_id, ute_username, ute_admin FROM Utente WHERE ute_id = $uteId";
        $result = $conn->query($sql);

        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();

            $utente = [
                "ute_id" => (int)$row["ute_id"],
                "ute_username" => $row["ute_username"],
                "ute_admin" => (bool)$row["ute_admin"],
                "link" => "$baseUrl/utenti/{$row["ute_id"]}"
            ];

            header('Content-Type: application/json');
            echo json_encode($utente, JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Utente non trovato"]);
        }
    }
    // GET tutti gli utenti
    else {
        $queryMeta = restGetCollectionQuery($conn, 'utenti');
        if (!$queryMeta['ok']) {
            http_response_code($queryMeta['status']);
            header('Content-Type: application/json');
            echo json_encode(["errore" => $queryMeta['errore']]);
            $conn->close();
            exit;
        }

        $utenti = [];
        $rows = $queryMeta['rows'];

        foreach ($rows as $row) {
            $utenti[] = [
                "ute_id" => (int)$row["ute_id"],
                "ute_username" => $row["ute_username"],
                "ute_admin" => (bool)$row["ute_admin"],
                "link" => "$baseUrl/utenti/{$row["ute_id"]}"
            ];
        }

        restSendCollectionResponse($utenti, $queryMeta);
    }
}

elseif ($method === 'POST') {
    $data = getRequestJsonBody();
    if (!is_array($data) || !isset($data['ute_username'], $data['ute_password'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido o campi mancanti: ute_username, ute_password"]);
        $conn->close();
        exit;
    }

    $username = $conn->real_escape_string($data['ute_username']);
    $password = $conn->real_escape_string($data['ute_password']);
    $admin = isset($data['ute_admin']) ? ($data['ute_admin'] ? 1 : 0) : 0;

    // Verifica che lo username non esista gia'
    $check = $conn->query("SELECT ute_id FROM Utente WHERE ute_username = '$username'");
    if ($check->num_rows > 0) {
        http_response_code(409);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Username gia' esistente"]);
        $conn->close();
        exit;
    }

    $sql = "INSERT INTO Utente (ute_username, ute_password, ute_admin)
            VALUES ('$username', '$password', $admin)";

    if ($conn->query($sql)) {
        $newId = $conn->insert_id;
        http_response_code(201);
        header('Content-Type: application/json');
        echo json_encode(["ute_id" => $newId, "link" => "$baseUrl/utenti/$newId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'PUT') {
    if (!isset($_GET['utenteId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro utenteId mancante"]);
        exit;
    }

    $uteId = (int)$_GET['utenteId'];
    $data = getRequestJsonBody();
    if (!is_array($data) || !isset($data['ute_username'], $data['ute_password'], $data['ute_admin'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido o campi mancanti: ute_username, ute_password, ute_admin"]);
        $conn->close();
        exit;
    }

    $username = $conn->real_escape_string($data['ute_username']);
    $password = $conn->real_escape_string($data['ute_password']);
    $admin = $data['ute_admin'] ? 1 : 0;

    $sql = "UPDATE Utente SET ute_username = '$username', ute_password = '$password',
            ute_admin = $admin WHERE ute_id = $uteId";

    if ($conn->query($sql)) {
        header('Content-Type: application/json');
        echo json_encode(["messaggio" => "Utente aggiornato", "link" => "$baseUrl/utenti/$uteId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'DELETE') {
    if (!isset($_GET['utenteId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro utenteId mancante"]);
        exit;
    }

    $uteId = (int)$_GET['utenteId'];

    $check = $conn->query("SELECT ute_id FROM Utente WHERE ute_id = $uteId");
    if ($check->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Utente non trovato"]);
        exit;
    }

    if ($conn->query("DELETE FROM Utente WHERE ute_id = $uteId")) {
        http_response_code(204);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

$conn->close();
