<?php
// =============================================================
// Endpoint: /sensoriarnia
// GET /sensoriarnia         → Lista tutti i sensori installati sulle arnie
// GET /sensoriarnia/{id}    → Dettaglio singolo sensore arnia
// POST /sensoriarnia        → Installa un sensore su un'arnia
// PUT /sensoriarnia/{id}    → Aggiorna configurazione sensore arnia
// DELETE /sensoriarnia/{id} → Rimuovi sensore da arnia
// =============================================================

require_once __DIR__ . '/../config.php';

$method = $_SERVER['REQUEST_METHOD'];

function sensoreArniaColumnExists($conn, $column) {
    static $cache = [];
    if (isset($cache[$column])) {
        return $cache[$column];
    }

    $colEsc = $conn->real_escape_string($column);
    $res = $conn->query("SHOW COLUMNS FROM `SensoreArnia` LIKE '$colEsc'");
    $exists = ($res && $res->num_rows > 0);
    $cache[$column] = $exists;
    return $exists;
}

if ($method === 'GET') {

    // GET singolo sensore arnia
    if (isset($_GET['sensoreArniaId'])) {
        $seaId = (int)$_GET['sensoreArniaId'];
        $sql = "SELECT * FROM SensoreArnia WHERE sea_id = $seaId";
        $result = $conn->query($sql);

        if ($result && $result->num_rows > 0) {
            $row = $result->fetch_assoc();

            // Arnia (partial)
            $arnia = null;
            $resArn = $conn->query("SELECT * FROM Arnia WHERE arn_id = {$row["sea_arn_id"]}");
            if ($resArn->num_rows > 0) {
                $a = $resArn->fetch_assoc();
                $arnia = [
                    "arn_id" => (int)$a["arn_id"],
                    "arn_piena" => (bool)$a["arn_piena"],
                    "arn_MacAddress" => $a["arn_MacAddress"],
                    "isPartial" => true,
                    "link" => "$baseUrl/arnie/{$a["arn_id"]}"
                ];
            }

            // Tipo rilevazione (partial)
            $tipo = null;
            $resTip = $conn->query("SELECT * FROM TipoRilevazione WHERE tip_id = {$row["sea_tip_id"]}");
            if ($resTip->num_rows > 0) {
                $t = $resTip->fetch_assoc();
                $tipo = [
                    "tip_tipologia" => $t["tip_tipologia"],
                    "tip_unita" => $t["tip_unita"],
                    "isPartial" => true,
                    "link" => "$baseUrl/tipirilevazione/{$t["tip_id"]}"
                ];
            }

            // Rilevazioni recenti (partial, ultime 10)
            $rilevazioni = [];
            $resRil = $conn->query("SELECT * FROM Rilevazione WHERE ril_sea_id = $seaId ORDER BY ril_dataOra DESC LIMIT 10");
            while ($r = $resRil->fetch_assoc()) {
                $rilevazioni[] = [
                    "ril_id" => (int)$r["ril_id"],
                    "ril_dato" => (float)$r["ril_dato"],
                    "ril_dataOra" => $r["ril_dataOra"],
                    "isPartial" => true,
                    "link" => "$baseUrl/rilevazioni/{$r["ril_id"]}"
                ];
            }

            $sensoreArnia = [
                "sea_id" => (int)$row["sea_id"],
                "sea_arn_id" => (int)$row["sea_arn_id"],
                "sea_tip_id" => (int)$row["sea_tip_id"],
                "sea_stato" => (bool)$row["sea_stato"],
                "sea_attivo" => (bool)$row["sea_attivo"],
                "sea_on" => (bool)$row["sea_on"],
                "sea_min" => (float)$row["sea_min"],
                "sea_max" => (float)$row["sea_max"],
                "arnia" => $arnia,
                "tipoRilevazione" => $tipo,
                "rilevazioni" => $rilevazioni,
                "link" => "$baseUrl/sensoriarnia/{$row["sea_id"]}"
            ];
            if (array_key_exists("sea_intervallo_ms", $row)) {
                $sensoreArnia["sea_intervallo_ms"] = (int)$row["sea_intervallo_ms"];
            }

            header('Content-Type: application/json');
            echo json_encode($sensoreArnia, JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            header('Content-Type: application/json');
            echo json_encode(["errore" => "Sensore arnia non trovato"]);
        }
    }
    // GET tutti i sensori arnia
    else {
        $sensoriArnia = [];
        $sql = "SELECT * FROM SensoreArnia";
        $result = $conn->query($sql);

        while ($row = $result->fetch_assoc()) {
            // Arnia (partial)
            $arnia = null;
            $resArn = $conn->query("SELECT * FROM Arnia WHERE arn_id = {$row["sea_arn_id"]}");
            if ($resArn->num_rows > 0) {
                $a = $resArn->fetch_assoc();
                $arnia = [
                    "arn_id" => (int)$a["arn_id"],
                    "arn_piena" => (bool)$a["arn_piena"],
                    "isPartial" => true,
                    "link" => "$baseUrl/arnie/{$a["arn_id"]}"
                ];
            }

            // Tipo rilevazione (partial)
            $tipo = null;
            $resTip = $conn->query("SELECT * FROM TipoRilevazione WHERE tip_id = {$row["sea_tip_id"]}");
            if ($resTip->num_rows > 0) {
                $t = $resTip->fetch_assoc();
                $tipo = [
                    "tip_tipologia" => $t["tip_tipologia"],
                    "tip_unita" => $t["tip_unita"],
                    "isPartial" => true,
                    "link" => "$baseUrl/tipirilevazione/{$t["tip_id"]}"
                ];
            }

            $sensoriArnia[] = [
                "sea_id" => (int)$row["sea_id"],
                "sea_arn_id" => (int)$row["sea_arn_id"],
                "sea_tip_id" => (int)$row["sea_tip_id"],
                "sea_stato" => (bool)$row["sea_stato"],
                "sea_attivo" => (bool)$row["sea_attivo"],
                "sea_on" => (bool)$row["sea_on"],
                "sea_min" => (float)$row["sea_min"],
                "sea_max" => (float)$row["sea_max"],
                "arnia" => $arnia,
                "tipoRilevazione" => $tipo,
                "link" => "$baseUrl/sensoriarnia/{$row["sea_id"]}"
            ];
            if (array_key_exists("sea_intervallo_ms", $row)) {
                $sensoriArnia[count($sensoriArnia) - 1]["sea_intervallo_ms"] = (int)$row["sea_intervallo_ms"];
            }
        }

        header('Content-Type: application/json');
        echo json_encode($sensoriArnia, JSON_PRETTY_PRINT);
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

    $arnId = (int)$data['sea_arn_id'];
    $tipId = (int)$data['sea_tip_id'];
    $stato = isset($data['sea_stato']) ? ($data['sea_stato'] ? 1 : 0) : 1;
    $attivo = isset($data['sea_attivo']) ? ($data['sea_attivo'] ? 1 : 0) : 1;
    $on = isset($data['sea_on']) ? ($data['sea_on'] ? 1 : 0) : 1;
    $min = isset($data['sea_min']) ? (float)$data['sea_min'] : 'NULL';
    $max = isset($data['sea_max']) ? (float)$data['sea_max'] : 'NULL';
    $intervallo = isset($data['sea_intervallo_ms']) ? (int)$data['sea_intervallo_ms'] : (isset($data['intervallo']) ? (int)$data['intervallo'] : 360000);

    if (sensoreArniaColumnExists($conn, 'sea_intervallo_ms')) {
        $sql = "INSERT INTO SensoreArnia (sea_arn_id, sea_tip_id, sea_stato, sea_attivo, sea_on, sea_min, sea_max, sea_intervallo_ms)
                VALUES ($arnId, $tipId, $stato, $attivo, $on, $min, $max, $intervallo)";
    } else {
        $sql = "INSERT INTO SensoreArnia (sea_arn_id, sea_tip_id, sea_stato, sea_attivo, sea_on, sea_min, sea_max)
                VALUES ($arnId, $tipId, $stato, $attivo, $on, $min, $max)";
    }

    if ($conn->query($sql)) {
        $newId = $conn->insert_id;
        http_response_code(201);
        header('Content-Type: application/json');
        echo json_encode(["sea_id" => $newId, "link" => "$baseUrl/sensoriarnia/$newId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'PUT') {
    if (!isset($_GET['sensoreArniaId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro sensoreArniaId mancante"]);
        exit;
    }

    $seaId = (int)$_GET['sensoreArniaId'];
    $data = getRequestJsonBody();
    if (!is_array($data)) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Body JSON non valido"]);
        $conn->close();
        exit;
    }

    $arnId = (int)$data['sea_arn_id'];
    $tipId = (int)$data['sea_tip_id'];
    $stato = $data['sea_stato'] ? 1 : 0;
    $attivo = $data['sea_attivo'] ? 1 : 0;
    $on = $data['sea_on'] ? 1 : 0;
    $min = isset($data['sea_min']) ? (float)$data['sea_min'] : 'NULL';
    $max = isset($data['sea_max']) ? (float)$data['sea_max'] : 'NULL';
    $intervallo = isset($data['sea_intervallo_ms']) ? (int)$data['sea_intervallo_ms'] : (isset($data['intervallo']) ? (int)$data['intervallo'] : 360000);

    if (sensoreArniaColumnExists($conn, 'sea_intervallo_ms')) {
        $sql = "UPDATE SensoreArnia SET sea_arn_id = $arnId, sea_tip_id = $tipId,
                sea_stato = $stato, sea_attivo = $attivo, sea_on = $on,
                sea_min = $min, sea_max = $max, sea_intervallo_ms = $intervallo
                WHERE sea_id = $seaId";
    } else {
        $sql = "UPDATE SensoreArnia SET sea_arn_id = $arnId, sea_tip_id = $tipId,
                sea_stato = $stato, sea_attivo = $attivo, sea_on = $on,
                sea_min = $min, sea_max = $max WHERE sea_id = $seaId";
    }

    if ($conn->query($sql)) {
        header('Content-Type: application/json');
        echo json_encode(["messaggio" => "Sensore arnia aggiornato", "link" => "$baseUrl/sensoriarnia/$seaId"]);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

elseif ($method === 'DELETE') {
    if (!isset($_GET['sensoreArniaId'])) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Parametro sensoreArniaId mancante"]);
        exit;
    }

    $seaId = (int)$_GET['sensoreArniaId'];

    $check = $conn->query("SELECT sea_id FROM SensoreArnia WHERE sea_id = $seaId");
    if ($check->num_rows === 0) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(["errore" => "Sensore arnia non trovato"]);
        exit;
    }

    if ($conn->query("DELETE FROM SensoreArnia WHERE sea_id = $seaId")) {
        http_response_code(204);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(["errore" => $conn->error]);
    }
}

$conn->close();
