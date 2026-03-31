<?php
// =============================================================
// API Manager - Router per il REST server Apicoltura Digitale
// Smista le richieste alle risorse appropriate
// =============================================================

$restLogDir = __DIR__ . '/logs';
$restLogFile = $restLogDir . '/rest_calls.log';
$restLogMaxBytes = 10 * 1024 * 1024; // 10 MB
$restLogKeepBytes = 8 * 1024 * 1024; // porzione mantenuta alla rotazione

function rest_log_append_circular($filePath, $entry, $maxBytes, $keepBytes) {
    $dir = dirname($filePath);
    if (!is_dir($dir)) {
        @mkdir($dir, 0775, true);
    }

    $line = json_encode($entry, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    if ($line === false) {
        $line = '{"errore":"encoding log fallito"}';
    }
    $line .= PHP_EOL;

    $fp = @fopen($filePath, 'c+');
    if (!$fp) {
        return;
    }

    if (!flock($fp, LOCK_EX)) {
        fclose($fp);
        return;
    }

    clearstatcache(true, $filePath);
    $size = @filesize($filePath);
    if ($size === false) {
        $size = 0;
    }

    if ($size > $maxBytes) {
        $keep = ($size > $keepBytes) ? $keepBytes : $size;
        $tail = '';

        if ($keep > 0) {
            fseek($fp, -$keep, SEEK_END);
            $tail = stream_get_contents($fp);
            if ($tail === false) {
                $tail = '';
            }
        }

        ftruncate($fp, 0);
        rewind($fp);
        if ($tail !== '') {
            fwrite($fp, $tail);
        }
    }

    fseek($fp, 0, SEEK_END);
    fwrite($fp, $line);
    fflush($fp);
    flock($fp, LOCK_UN);
    fclose($fp);
}

$restRequestStart = microtime(true);
$restMethod = $_SERVER['REQUEST_METHOD'] ?? 'UNKNOWN';
$restUri = $_SERVER['REQUEST_URI'] ?? '';
$restIp = $_SERVER['REMOTE_ADDR'] ?? '';
$restUserAgent = $_SERVER['HTTP_USER_AGENT'] ?? '';
$restContentType = $_SERVER['CONTENT_TYPE'] ?? '';
$restRawBody = file_get_contents('php://input');
if ($restRawBody === false) {
    $restRawBody = '';
}
$GLOBALS['REST_RAW_INPUT'] = $restRawBody;

register_shutdown_function(function () use (
    $restLogFile,
    $restLogMaxBytes,
    $restLogKeepBytes,
    $restRequestStart,
    $restMethod,
    $restUri,
    $restIp,
    $restUserAgent,
    $restContentType,
    $restRawBody
) {
    $status = http_response_code();
    if (!$status) {
        $status = 200;
    }

    $durationMs = (int)round((microtime(true) - $restRequestStart) * 1000);
    $lastError = error_get_last();
    $fatalTypes = [E_ERROR, E_PARSE, E_CORE_ERROR, E_COMPILE_ERROR, E_USER_ERROR];
    $fatal = null;

    if ($lastError && in_array($lastError['type'], $fatalTypes, true)) {
        $fatal = [
            "type" => $lastError['type'],
            "message" => $lastError['message'],
            "file" => $lastError['file'],
            "line" => $lastError['line']
        ];
        if ($status < 500) {
            $status = 500;
        }
    }

    $entry = [
        "timestamp" => date('c'),
        "method" => $restMethod,
        "url" => $restUri,
        "status" => $status,
        "duration_ms" => $durationMs,
        "ip" => $restIp,
        "content_type" => $restContentType,
        "user_agent" => $restUserAgent,
        "payload" => $restRawBody
    ];

    if ($fatal !== null) {
        $entry["fatal_error"] = $fatal;
    }

    rest_log_append_circular($restLogFile, $entry, $restLogMaxBytes, $restLogKeepBytes);
});

// CORS headers per consentire richieste dal front-end
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS");
header("Access-Control-Allow-Headers: Content-Type, Authorization, x-apikey, X-Requested-With, Accept");

// Gestione preflight CORS
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

// Carica configurazione condivisa:
// - valida x-apikey (se abilitato in config.php)
// - inizializza connessione DB e variabili comuni
require_once __DIR__ . '/config.php';

// URI richiesta
$requestURI = $_SERVER['REQUEST_URI'];
$scriptName = $_SERVER['SCRIPT_NAME'];

// Estrai il path, escludendo query string
$path = parse_url($requestURI, PHP_URL_PATH);

// Rimuovi eventuale path base del server (es. /rest/)
$basePath = dirname($scriptName);
if (strpos($path, $basePath) === 0) {
    $path = substr($path, strlen($basePath));
}

// Normalizza: rimuovi slash iniziali/finali
$path = trim($path, '/');

// Split della URI nei segmenti
$segments = explode('/', $path);

// Se vuota, mostra le risorse disponibili
if (empty($segments[0])) {
    header('Content-Type: application/json');
    echo json_encode([
        "messaggio" => "API Apicoltura Digitale",
        "versione" => "1.0",
        "risorse" => [
            "_ping" => "http://" . $_SERVER['HTTP_HOST'] . dirname($scriptName) . "/_ping/",
            "apiari" => "http://" . $_SERVER['HTTP_HOST'] . dirname($scriptName) . "/apiari/",
            "arnie" => "http://" . $_SERVER['HTTP_HOST'] . dirname($scriptName) . "/arnie/",
            "sensori" => "http://" . $_SERVER['HTTP_HOST'] . dirname($scriptName) . "/sensori/",
            "tipirilevazione" => "http://" . $_SERVER['HTTP_HOST'] . dirname($scriptName) . "/tipirilevazione/",
            "sensoriarnia" => "http://" . $_SERVER['HTTP_HOST'] . dirname($scriptName) . "/sensoriarnia/",
            "configurazioni" => "http://" . $_SERVER['HTTP_HOST'] . dirname($scriptName) . "/configurazioni/",
            "rilevazioni" => "http://" . $_SERVER['HTTP_HOST'] . dirname($scriptName) . "/rilevazioni/",
            "notifiche" => "http://" . $_SERVER['HTTP_HOST'] . dirname($scriptName) . "/notifiche/",
            "utenti" => "http://" . $_SERVER['HTTP_HOST'] . dirname($scriptName) . "/utenti/"
        ]
    ], JSON_PRETTY_PRINT);
    exit;
}

// Primo segmento = risorsa
$risorsa = $segments[0];
$parametri = [];

// Se presente un secondo segmento, interpretalo come ID della risorsa
if (isset($segments[1]) && $segments[1] !== '') {
    switch ($risorsa) {
        case '_ping':
            $parametri['pingId'] = $segments[1];
            break;
        case 'apiari':
            $parametri['apiarioId'] = $segments[1];
            break;
        case 'arnie':
            $parametri['arniaId'] = $segments[1];
            break;
        case 'sensori':
            $parametri['sensoreId'] = $segments[1];
            break;
        case 'tipirilevazione':
            $parametri['tipoId'] = $segments[1];
            break;
        case 'sensoriarnia':
            $parametri['sensoreArniaId'] = $segments[1];
            break;
        case 'configurazioni':
            $parametri['configurazioneId'] = $segments[1];
            break;
        case 'rilevazioni':
            $parametri['rilevazioneId'] = $segments[1];
            break;
        case 'notifiche':
            $parametri['notificaId'] = $segments[1];
            break;
        case 'utenti':
            $parametri['utenteId'] = $segments[1];
            break;
        default:
            break;
    }
}

// Costruzione del file da includere
$folderPath = __DIR__ . '/' . $risorsa;
$filePath = $folderPath . '/index.php';

// Controllo esistenza file
if (file_exists($filePath)) {
    // Passa i parametri estratti dalla URI
    $_GET = array_merge($_GET, $parametri);
    include $filePath;
} else {
    http_response_code(404);
    header('Content-Type: application/json');
    echo json_encode(["errore" => "Risorsa non trovata: $risorsa"]);
}
