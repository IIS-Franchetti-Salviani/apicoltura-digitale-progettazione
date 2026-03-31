<?php
// =============================================================
// Endpoint: /tipirilevazione
// GET /tipirilevazione         -> Lista tutti i tipi di rilevazione con sensore (partial)
// GET /tipirilevazione/{id}    -> Dettaglio singolo tipo
// POST /tipirilevazione        -> Crea un nuovo tipo di rilevazione
// PUT /tipirilevazione/{id}    -> Aggiorna un tipo esistente
// DELETE /tipirilevazione/{id} -> Elimina un tipo
// =============================================================

require_once __DIR__ . '/../config.php';

$method = $_SERVER['REQUEST_METHOD'];

function dbColumnExists($conn, $table, $column) {
    static $cache = [];
    $key = $table . '.' . $column;
    if (isset($cache[$key])) {
        return $cache[$key];
    }

    $tableEsc = $conn->real_escape_string($table);
    $colEsc = $conn->real_escape_string($column);
    $res = $conn->query("SHOW COLUMNS FROM `$tableEsc` LIKE '$colEsc'");
    $exists = ($res && $res->num_rows > 0);
    $cache[$key] = $exists;
    return $exists;
}

function defaultTipCodice($tipologia) {
    $tmp = strtolower(trim($tipologia));
    $tmp = preg_replace('/[^a-z0-9]+/', '_', $tmp);
    $tmp = trim($tmp, '_');
    if ($tmp === '') {
        $tmp = 'tipo';
    }
    return $tmp;
}

if ($method === 'GET') {

    $hasTipCodice = dbColumnExists($conn, 'TipoRilevazione', 'tip_codice');
    $hasTipFuturo = dbColumnExists($conn, 'TipoRilevazione', 'tip_futuro');

    // GET singolo tipo
    if (isset($_GET['tipoId'])) {
        $tipoId = (int)$_GET['tipoId'];
        $sql = "SELECT * FROM TipoRilevazione WHERE tip_id = $tipoId";
        $result = $conn->query($sql);

        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();

            // Sensore associato (partial)
            $sensore = null;
            $resSen = $conn->query("SELECT * FROM Sensore WHERE sen_id = {$row["tip_sen_id"]}");
            if ($resSen && $resSen->num_rows > 0) {
                $s = $resSen->fetch_assoc();
                $sensore = [
                    "sen_modello" => $s["sen_modello"],
                    "isPartial" => true,
                    "link" => "$baseUrl/sensori/{$s["sen_id"]}"
                ];
            }

            // Sensori arnia che usano questo tipo (partial)
            $sensoriArnia = [];
            $resSea = $conn->query("SELECT * FROM SensoreArnia WHERE sea_tip_id = $tipoId");
            while ($resSea && ($sa = $resSea->fetch_assoc())) {
                $sensoriArnia[] = [
                    "sea_id" => (int)$sa["sea_id"],
                    "sea_arn_id" => (int)$sa["sea_arn_id"],
                    "isPartial" => true,
                    "link" => "$baseUrl/sensoriarnia/{$sa["sea_id"]}"
                ];
            }

            $tipo = [
                "tip_id" => (int)$row["tip_id"],
                "tip_tipologia" => $row["tip_tipologia"],
                "tip_sen_id" => (int)$row["tip_sen_id"],
                "tip_unita" => $row["tip_unita"],
                "sensore" => $sensore,
                "sensoriArnia" => $sensoriArnia,
                "link" => "$baseUrl/tipirilevazione/{$row["tip_id"]}"
            ];

            if ($hasTipCodice) {
                $tipo["tip_codice"] = $row["tip_codice"];
            }
            if ($hasTipFuturo) {
                $tipo["tip_futuro"] = (bool)$row["tip_futuro"];
            }

            header('Content-Type: application/json');
            echo json_encode($tipo, JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Tipo rilevazione non trovato"]);
        }
    }
    // GET tutti i tipi
    else {
        $tipi = [];
        $sql = "SELECT * FROM TipoRilevazione";
        $result = $conn->query($sql);

        while ($result && ($row = $result->fetch_assoc())) {
            $sensore = null;
            $resSen = $conn->query("SELECT * FROM Sensore WHERE sen_id = {$row["tip_sen_id"]}");
            if ($resSen && $resSen->num_rows > 0) {
                $s = $resSen->fetch_assoc();
                $sensore = [
                    "sen_modello" => $s["sen_modello"],
                    "isPartial" => true,
                    "link" => "$baseUrl/sensori/{$s["sen_id"]}"
                ];
            }

            $item = [
                "tip_id" => (int)$row["tip_id"],
                "tip_tipologia" => $row["tip_tipologia"],
                "tip_sen_id" => (int)$row["tip_sen_id"],
                "tip_unita" => $row["tip_unita"],
                "sensore" => $sensore,
                "link" => "$baseUrl/tipirilevazione/{$row["tip_id"]}"
            ];

            if ($hasTipCodice) {
                $item["tip_codice"] = $row["tip_codice"];
            }
            if ($hasTipFuturo) {
                $item["tip_futuro"] = (bool)$row["tip_futuro"];
            }

            $tipi[] = $item;
        }

        header('Content-Type: application/json');
        echo json_encode($tipi, JSON_PRETTY_PRINT);
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

    if (!isset($data['tip_tipologia'], $data['tip_sen_id'], $data['tip_unita'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Campi obbligatori: tip_tipologia, tip_sen_id, tip_unita"]);
        $conn->close();
        exit;
    }

    $tipologia = $conn->real_escape_string($data['tip_tipologia']);
    $senId = (int)$data['tip_sen_id'];
    $unita = $conn->real_escape_string($data['tip_unita']);

    $hasTipCodice = dbColumnExists($conn, 'TipoRilevazione', 'tip_codice');
    $hasTipFuturo = dbColumnExists($conn, 'TipoRilevazione', 'tip_futuro');

    $fields = ["tip_tipologia", "tip_sen_id", "tip_unita"];
    $values = ["'$tipologia'", "$senId", "'$unita'"];

    if ($hasTipCodice) {
        $tipCodiceRaw = isset($data['tip_codice']) ? $data['tip_codice'] : defaultTipCodice($data['tip_tipologia']);
        $tipCodice = $conn->real_escape_string($tipCodiceRaw);
        $fields[] = "tip_codice";
        $values[] = "'$tipCodice'";
    }

    if ($hasTipFuturo) {
        $tipFuturo = isset($data['tip_futuro']) ? ((bool)$data['tip_futuro'] ? 1 : 0) : 0;
        $fields[] = "tip_futuro";
        $values[] = "$tipFuturo";
    }

    $sql = "INSERT INTO TipoRilevazione (" . implode(", ", $fields) . ")
            VALUES (" . implode(", ", $values) . ")";

    if ($conn->query($sql)) {
        $newId = $conn->insert_id;
        http_response_code(201);
        header('Content-Type: application/json');
        echo json_encode(["tip_id" => $newId, "link" => "$baseUrl/tipirilevazione/$newId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'PUT') {
    if (!isset($_GET['tipoId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro tipoId mancante"]);
        exit;
    }

    $tipoId = (int)$_GET['tipoId'];
    $data = getRequestJsonBody();
    if (!is_array($data)) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido"]);
        $conn->close();
        exit;
    }

    if (!isset($data['tip_tipologia'], $data['tip_sen_id'], $data['tip_unita'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Campi obbligatori: tip_tipologia, tip_sen_id, tip_unita"]);
        $conn->close();
        exit;
    }

    $tipologia = $conn->real_escape_string($data['tip_tipologia']);
    $senId = (int)$data['tip_sen_id'];
    $unita = $conn->real_escape_string($data['tip_unita']);

    $hasTipCodice = dbColumnExists($conn, 'TipoRilevazione', 'tip_codice');
    $hasTipFuturo = dbColumnExists($conn, 'TipoRilevazione', 'tip_futuro');

    $set = [
        "tip_tipologia = '$tipologia'",
        "tip_sen_id = $senId",
        "tip_unita = '$unita'"
    ];

    if ($hasTipCodice) {
        $tipCodiceRaw = isset($data['tip_codice']) ? $data['tip_codice'] : defaultTipCodice($data['tip_tipologia']);
        $tipCodice = $conn->real_escape_string($tipCodiceRaw);
        $set[] = "tip_codice = '$tipCodice'";
    }

    if ($hasTipFuturo) {
        $tipFuturo = isset($data['tip_futuro']) ? ((bool)$data['tip_futuro'] ? 1 : 0) : 0;
        $set[] = "tip_futuro = $tipFuturo";
    }

    $sql = "UPDATE TipoRilevazione SET " . implode(", ", $set) . " WHERE tip_id = $tipoId";

    if ($conn->query($sql)) {
        header('Content-Type: application/json');
        echo json_encode(["messaggio" => "Tipo rilevazione aggiornato", "link" => "$baseUrl/tipirilevazione/$tipoId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'DELETE') {
    if (!isset($_GET['tipoId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro tipoId mancante"]);
        exit;
    }

    $tipoId = (int)$_GET['tipoId'];

    $check = $conn->query("SELECT tip_id FROM TipoRilevazione WHERE tip_id = $tipoId");
    if (!$check || $check->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Tipo rilevazione non trovato"]);
        exit;
    }

    if ($conn->query("DELETE FROM TipoRilevazione WHERE tip_id = $tipoId")) {
        http_response_code(204);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

$conn->close();

