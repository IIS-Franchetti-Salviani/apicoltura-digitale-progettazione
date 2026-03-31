<?php
// =============================================================
// Configurazione centralizzata del database
// Apicoltura Digitale - REST Server
// =============================================================

// =============================================================
// Sicurezza API Key (x-apikey)
// =============================================================
// 1) Attiva/disattiva il controllo accesso
//    true  = richiesto header x-apikey valido
//    false = accesso libero (comportamento legacy)
$apiKeyValidationEnabled = true;

// 2) Lista chiavi valide (usane almeno una robusta in produzione)
$allowedApiKeys = [
    'M7QYVwcR8Njwt2JgfprFw3rgkdLdYuYg'
];

// 3) Nome header da validare
$apiKeyHeaderName = 'x-apikey';

// 4) Fallback opzionale da query string (consigliato false)
$apiKeyAllowQueryParam = false;
$apiKeyQueryParamName = 'x-apikey';

if (!function_exists('restGetHeaderValue')) {
    function restGetHeaderValue($headerName) {
        $target = strtolower($headerName);

        if (function_exists('getallheaders')) {
            $headers = getallheaders();
            if (is_array($headers)) {
                foreach ($headers as $k => $v) {
                    if (strtolower($k) === $target) {
                        return trim((string)$v);
                    }
                }
            }
        }

        $serverKey = 'HTTP_' . strtoupper(str_replace('-', '_', $headerName));
        if (isset($_SERVER[$serverKey])) {
            return trim((string)$_SERVER[$serverKey]);
        }

        return '';
    }
}

if (!function_exists('restIsApiKeyAllowed')) {
    function restIsApiKeyAllowed($candidate, $allowedKeys) {
        if (!is_string($candidate) || $candidate === '') {
            return false;
        }

        foreach ($allowedKeys as $validKey) {
            if (!is_string($validKey) || $validKey === '') {
                continue;
            }
            if (hash_equals($validKey, $candidate)) {
                return true;
            }
        }

        return false;
    }
}

if (!function_exists('restEnforceApiKey')) {
    function restEnforceApiKey() {
        global $apiKeyValidationEnabled, $allowedApiKeys, $apiKeyHeaderName, $apiKeyAllowQueryParam, $apiKeyQueryParamName;

        if (!$apiKeyValidationEnabled) {
            return true;
        }

        $providedKey = restGetHeaderValue($apiKeyHeaderName);

        if ($providedKey === '' && $apiKeyAllowQueryParam && isset($_GET[$apiKeyQueryParamName])) {
            $providedKey = trim((string)$_GET[$apiKeyQueryParamName]);
        }

        if (!restIsApiKeyAllowed($providedKey, $allowedApiKeys)) {
            http_response_code(401);
            header('Content-Type: application/json');
            echo json_encode([
                "errore" => "Accesso non autorizzato",
                "dettaglio" => "Header x-apikey mancante o non valido"
            ]);
            return false;
        }

        return true;
    }
}

// Esegue il controllo per tutte le richieste HTTP (tranne preflight CORS)
if (php_sapi_name() !== 'cli') {
    $method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
    if ($method !== 'OPTIONS') {
        if (!restEnforceApiKey()) {
            exit;
        }
    }
}

$dbHost = '89.46.111.79';
$dbPort = 3306;
$dbUser = 'Sql1287228';
$dbPassword = '0w23o56486';
$dbName = 'Sql1287228_4';

// Connessione al database
$conn = new mysqli($dbHost, $dbUser, $dbPassword, $dbName, $dbPort);

// Verifica connessione
if ($conn->connect_error) {
    http_response_code(500);
    header('Content-Type: application/json');
    echo json_encode(["errore" => "Connessione al database fallita: " . $conn->connect_error]);
    exit;
}

// Imposta charset UTF-8
$conn->set_charset("utf8mb4");

// URL base per la costruzione dei link HATEOAS
$baseUrl = "https://" . $_SERVER['HTTP_HOST'] . dirname($_SERVER['SCRIPT_NAME']);

// Corpo richiesta raw e parser JSON condivisi
if (!function_exists('getRawRequestBody')) {
    function getRawRequestBody() {
        static $cachedRaw = null;

        if ($cachedRaw !== null) {
            return $cachedRaw;
        }

        if (isset($GLOBALS['REST_RAW_INPUT'])) {
            $cachedRaw = (string)$GLOBALS['REST_RAW_INPUT'];
            return $cachedRaw;
        }

        $raw = file_get_contents("php://input");
        $cachedRaw = ($raw === false) ? '' : $raw;
        return $cachedRaw;
    }
}

if (!function_exists('getRequestJsonBody')) {
    function getRequestJsonBody() {
        $raw = getRawRequestBody();
        if ($raw === '') {
            return [];
        }

        $decoded = json_decode($raw, true);
        if (!is_array($decoded)) {
            return null;
        }

        return $decoded;
    }
}
