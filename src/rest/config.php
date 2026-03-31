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

// =============================================================
// Query layer riusabile (restdb-like) per GET collection
// Parametri supportati:
// - q={...}
// - h={"$fields":{...},"$max":...,"$skip":...,"$orderby":{...}}
// - filter=...
// - sort=campo&dir=1|-1
// - skip, max
// - totals=true, count=true
// =============================================================

// Punto unico per controllare esposizione campi/filtri/sort per risorsa.
$restResourceExposure = [
    'apiari' => [
        'table' => 'Apiario',
        'pk' => 'api_id',
        'fields' => ['api_id', 'api_nome', 'api_luogo', 'api_lon', 'api_lat'],
        'required_fields' => ['api_id'],
        'search_fields' => ['api_nome', 'api_luogo'],
        'default_sort' => 'api_id',
        'default_dir' => 1,
        'default_max' => 100,
        'max_limit' => 1000
    ],
    'arnie' => [
        'table' => 'Arnia',
        'pk' => 'arn_id',
        'fields' => ['arn_id', 'arn_api_id', 'arn_nome', 'arn_dataInst', 'arn_piena', 'arn_MacAddress', 'arn_attiva'],
        'required_fields' => ['arn_id', 'arn_api_id'],
        'search_fields' => ['arn_nome', 'arn_MacAddress'],
        'default_sort' => 'arn_id',
        'default_dir' => 1,
        'default_max' => 100,
        'max_limit' => 1000
    ],
    'sensori' => [
        'table' => 'Sensore',
        'pk' => 'sen_id',
        'fields' => ['sen_id', 'sen_modello', 'sen_codice', 'sen_produttore'],
        'required_fields' => ['sen_id'],
        'search_fields' => ['sen_modello', 'sen_codice', 'sen_produttore'],
        'default_sort' => 'sen_id',
        'default_dir' => 1,
        'default_max' => 100,
        'max_limit' => 1000
    ],
    'tipirilevazione' => [
        'table' => 'TipoRilevazione',
        'pk' => 'tip_id',
        'fields' => ['tip_id', 'tip_tipologia', 'tip_codice', 'tip_sen_id', 'tip_unita', 'tip_futuro'],
        'required_fields' => ['tip_id', 'tip_sen_id'],
        'search_fields' => ['tip_tipologia', 'tip_codice', 'tip_unita'],
        'default_sort' => 'tip_id',
        'default_dir' => 1,
        'default_max' => 100,
        'max_limit' => 1000
    ],
    'sensoriarnia' => [
        'table' => 'SensoreArnia',
        'pk' => 'sea_id',
        'fields' => ['sea_id', 'sea_arn_id', 'sea_tip_id', 'sea_stato', 'sea_attivo', 'sea_on', 'sea_min', 'sea_max', 'sea_intervallo_ms'],
        'required_fields' => ['sea_id', 'sea_arn_id', 'sea_tip_id'],
        'search_fields' => [],
        'default_sort' => 'sea_id',
        'default_dir' => 1,
        'default_max' => 100,
        'max_limit' => 1000
    ],
    'rilevazioni' => [
        'table' => 'Rilevazione',
        'pk' => 'ril_id',
        'fields' => ['ril_id', 'ril_sea_id', 'ril_dato', 'ril_dataOra', 'ril_codice_stato'],
        'required_fields' => ['ril_id', 'ril_sea_id'],
        'search_fields' => [],
        'default_sort' => 'ril_dataOra',
        'default_dir' => -1,
        'default_max' => 100,
        'max_limit' => 1000
    ],
    'notifiche' => [
        'table' => 'Notifica',
        'pk' => 'not_id',
        'fields' => ['not_id', 'not_ril_id', 'not_titolo', 'not_dex', 'not_macAddress', 'not_tipoSensore', 'not_valoreRiferimento', 'not_timestamp', 'not_livello', 'not_livelloStr', 'not_letto'],
        'required_fields' => ['not_id', 'not_ril_id'],
        'search_fields' => ['not_titolo', 'not_dex', 'not_macAddress', 'not_tipoSensore'],
        'default_sort' => 'not_id',
        'default_dir' => -1,
        'default_max' => 100,
        'max_limit' => 1000
    ],
    'utenti' => [
        'table' => 'Utente',
        'pk' => 'ute_id',
        'fields' => ['ute_id', 'ute_username', 'ute_admin', 'ute_token', 'ute_scadenzaToken', 'ute_creato_at'],
        'required_fields' => ['ute_id'],
        'search_fields' => ['ute_username'],
        'default_sort' => 'ute_id',
        'default_dir' => 1,
        'default_max' => 100,
        'max_limit' => 500
    ],
    'configurazioni' => [
        'table' => 'ConfigurazioneScheda',
        'pk' => 'cfs_id',
        'fields' => ['cfs_id', 'cfs_arn_id', 'cfs_macAddress', 'calibrationFactor', 'calibrationOffset', 'rest_timeout_ms', 'wdt_timeout_sec', 'wifi_check_ms', 'ota_abilitato'],
        'required_fields' => ['cfs_id', 'cfs_arn_id'],
        'search_fields' => ['cfs_macAddress'],
        'default_sort' => 'cfs_id',
        'default_dir' => 1,
        'default_max' => 100,
        'max_limit' => 1000
    ]
];

if (!function_exists('restGetResourceExposure')) {
    function restGetResourceExposure($resourceName) {
        global $restResourceExposure;
        return $restResourceExposure[$resourceName] ?? null;
    }
}

if (!function_exists('restGetTableColumns')) {
    function restGetTableColumns($conn, $tableName) {
        static $cache = [];
        if (isset($cache[$tableName])) {
            return $cache[$tableName];
        }

        $tableEsc = $conn->real_escape_string($tableName);
        $res = $conn->query("SHOW COLUMNS FROM `$tableEsc`");
        $cols = [];
        while ($res && ($row = $res->fetch_assoc())) {
            $cols[] = $row['Field'];
        }
        $cache[$tableName] = $cols;
        return $cols;
    }
}

if (!function_exists('restParseJsonParam')) {
    function restParseJsonParam($paramName, &$error = null) {
        if (!isset($_GET[$paramName])) {
            return null;
        }
        $decoded = json_decode($_GET[$paramName], true);
        if (!is_array($decoded)) {
            $error = "Parametro $paramName non valido: atteso JSON object";
            return null;
        }
        return $decoded;
    }
}

if (!function_exists('restParseBool')) {
    function restParseBool($value) {
        if (is_bool($value)) {
            return $value;
        }
        $v = strtolower(trim((string)$value));
        return in_array($v, ['1', 'true', 'yes', 'on'], true);
    }
}

if (!function_exists('restSqlLiteral')) {
    function restSqlLiteral($conn, $value) {
        if ($value === null) return 'NULL';
        if (is_bool($value)) return $value ? '1' : '0';
        if (is_int($value) || is_float($value)) return (string)$value;
        if (is_numeric($value) && !is_string($value)) return (string)$value;
        return "'" . $conn->real_escape_string((string)$value) . "'";
    }
}

if (!function_exists('restCompileFieldCondition')) {
    function restCompileFieldCondition($conn, $field, $value, &$error = null) {
        $fieldSql = "`$field`";

        if (!is_array($value)) {
            if ($value === null) return "$fieldSql IS NULL";
            return "$fieldSql = " . restSqlLiteral($conn, $value);
        }

        $parts = [];
        foreach ($value as $op => $opValue) {
            switch ($op) {
                case '$gt':  $parts[] = "$fieldSql > "  . restSqlLiteral($conn, $opValue); break;
                case '$gte': $parts[] = "$fieldSql >= " . restSqlLiteral($conn, $opValue); break;
                case '$lt':  $parts[] = "$fieldSql < "  . restSqlLiteral($conn, $opValue); break;
                case '$lte': $parts[] = "$fieldSql <= " . restSqlLiteral($conn, $opValue); break;
                case '$not':
                    if ($opValue === null) $parts[] = "$fieldSql IS NOT NULL";
                    else $parts[] = "$fieldSql <> " . restSqlLiteral($conn, $opValue);
                    break;
                case '$exists':
                    $parts[] = restParseBool($opValue) ? "$fieldSql IS NOT NULL" : "$fieldSql IS NULL";
                    break;
                case '$regex':
                    $parts[] = "$fieldSql REGEXP " . restSqlLiteral($conn, $opValue);
                    break;
                case '$in':
                case '$nin':
                    if (!is_array($opValue) || count($opValue) === 0) {
                        $error = "Operatore $op richiede array non vuoto";
                        return null;
                    }
                    $vals = [];
                    foreach ($opValue as $v) $vals[] = restSqlLiteral($conn, $v);
                    $parts[] = $fieldSql . ($op === '$in' ? " IN (" : " NOT IN (") . implode(',', $vals) . ")";
                    break;
                case '$bt':
                    if (!is_array($opValue) || count($opValue) !== 2) {
                        $error = "Operatore $op richiede array di 2 valori";
                        return null;
                    }
                    $parts[] = "$fieldSql BETWEEN " . restSqlLiteral($conn, $opValue[0]) . " AND " . restSqlLiteral($conn, $opValue[1]);
                    break;
                default:
                    $error = "Operatore non supportato: $op";
                    return null;
            }
        }

        return count($parts) ? '(' . implode(' AND ', $parts) . ')' : '1=1';
    }
}

if (!function_exists('restCompileQueryObject')) {
    function restCompileQueryObject($conn, $queryObj, $allowedFields, &$error = null) {
        if (!is_array($queryObj)) return '1=1';
        $parts = [];

        foreach ($queryObj as $key => $value) {
            if ($key === '$or' || $key === '$and') {
                if (!is_array($value) || count($value) === 0) {
                    $error = "$key richiede array non vuoto";
                    return null;
                }
                $subParts = [];
                foreach ($value as $node) {
                    $sub = restCompileQueryObject($conn, $node, $allowedFields, $error);
                    if ($sub === null) return null;
                    $subParts[] = "($sub)";
                }
                $parts[] = '(' . implode($key === '$or' ? ' OR ' : ' AND ', $subParts) . ')';
                continue;
            }

            if (!in_array($key, $allowedFields, true)) {
                $error = "Filtro sul campo non consentito: $key";
                return null;
            }

            $cond = restCompileFieldCondition($conn, $key, $value, $error);
            if ($cond === null) return null;
            $parts[] = $cond;
        }

        return count($parts) ? implode(' AND ', $parts) : '1=1';
    }
}

if (!function_exists('restBuildOrderByClause')) {
    function restBuildOrderByClause($allowedFields, $defaultSort, $defaultDir, $hint) {
        $order = [];

        if (is_array($hint) && isset($hint['$orderby']) && is_array($hint['$orderby'])) {
            foreach ($hint['$orderby'] as $field => $dir) {
                if (in_array($field, $allowedFields, true)) {
                    $order[] = "`$field` " . ((int)$dir < 0 ? 'DESC' : 'ASC');
                }
            }
        }

        if (isset($_GET['sort'])) {
            $sortField = (string)$_GET['sort'];
            if (in_array($sortField, $allowedFields, true)) {
                $dir = isset($_GET['dir']) && (int)$_GET['dir'] < 0 ? 'DESC' : 'ASC';
                $order = ["`$sortField` $dir"];
            }
        }

        if (count($order) === 0 && in_array($defaultSort, $allowedFields, true)) {
            $order[] = "`$defaultSort` " . ((int)$defaultDir < 0 ? 'DESC' : 'ASC');
        }

        return count($order) ? implode(', ', $order) : '';
    }
}

if (!function_exists('restGetCollectionQuery')) {
    function restGetCollectionQuery($conn, $resourceName) {
        $policy = restGetResourceExposure($resourceName);
        if (!$policy) {
            return ["ok" => false, "status" => 500, "errore" => "Policy risorsa non trovata: $resourceName"];
        }

        $table = $policy['table'];
        $pk = $policy['pk'];

        $dbColumns = restGetTableColumns($conn, $table);
        $allowedFields = array_values(array_intersect($policy['fields'], $dbColumns));
        if (count($allowedFields) === 0) {
            return ["ok" => false, "status" => 500, "errore" => "Nessun campo esposto configurato per $resourceName"];
        }
        if (in_array($pk, $dbColumns, true) && !in_array($pk, $allowedFields, true)) {
            $allowedFields[] = $pk;
        }

        $hintErr = null;
        $hint = restParseJsonParam('h', $hintErr);
        if ($hintErr !== null) {
            return ["ok" => false, "status" => 400, "errore" => $hintErr];
        }
        if ($hint === null) $hint = [];

        $queryErr = null;
        $queryObj = restParseJsonParam('q', $queryErr);
        if ($queryErr !== null) {
            return ["ok" => false, "status" => 400, "errore" => $queryErr];
        }
        if ($queryObj === null) $queryObj = [];

        $selectedFields = $allowedFields;
        if (isset($hint['$fields']) && is_array($hint['$fields'])) {
            $selectedFields = [];
            foreach ($hint['$fields'] as $f => $enabled) {
                if ($enabled && in_array($f, $allowedFields, true)) $selectedFields[] = $f;
            }
            if (count($selectedFields) === 0) $selectedFields = $allowedFields;
            if (!empty($policy['required_fields']) && is_array($policy['required_fields'])) {
                foreach ($policy['required_fields'] as $reqField) {
                    if (in_array($reqField, $allowedFields, true) && !in_array($reqField, $selectedFields, true)) {
                        $selectedFields[] = $reqField;
                    }
                }
            } elseif (in_array($pk, $allowedFields, true) && !in_array($pk, $selectedFields, true)) {
                $selectedFields[] = $pk;
            }
        }

        $whereParts = ['1=1'];

        $querySqlErr = null;
        $querySql = restCompileQueryObject($conn, $queryObj, $allowedFields, $querySqlErr);
        if ($querySql === null) {
            return ["ok" => false, "status" => 400, "errore" => $querySqlErr];
        }
        $whereParts[] = "($querySql)";

        $filterText = isset($_GET['filter']) ? trim((string)$_GET['filter']) : '';
        $searchFields = array_values(array_intersect($policy['search_fields'], $allowedFields));
        if ($filterText !== '' && count($searchFields) > 0) {
            $needle = "'%" . $conn->real_escape_string($filterText) . "%'";
            $filterParts = [];
            foreach ($searchFields as $f) $filterParts[] = "`$f` LIKE $needle";
            $whereParts[] = '(' . implode(' OR ', $filterParts) . ')';
        }

        $whereSql = implode(' AND ', $whereParts);
        $orderBySql = restBuildOrderByClause($allowedFields, $policy['default_sort'], $policy['default_dir'], $hint);

        $defaultMax = (int)$policy['default_max'];
        $maxLimit = (int)$policy['max_limit'];

        $skip = isset($hint['$skip']) ? (int)$hint['$skip'] : (isset($_GET['skip']) ? (int)$_GET['skip'] : 0);
        if ($skip < 0) $skip = 0;

        $max = isset($hint['$max']) ? (int)$hint['$max'] : (isset($_GET['max']) ? (int)$_GET['max'] : $defaultMax);
        if ($max < 0) $max = 0;
        if ($max > $maxLimit) $max = $maxLimit;

        $totals = isset($_GET['totals']) ? restParseBool($_GET['totals']) : false;
        $countOnly = isset($_GET['count']) ? restParseBool($_GET['count']) : false;

        $selectSql = implode(', ', array_map(function ($f) { return "`$f`"; }, $selectedFields));

        $sql = "SELECT $selectSql FROM `$table` WHERE $whereSql";
        if ($orderBySql !== '') $sql .= " ORDER BY $orderBySql";
        $sql .= " LIMIT $max OFFSET $skip";

        $result = $conn->query($sql);
        if (!$result) {
            return ["ok" => false, "status" => 500, "errore" => $conn->error];
        }

        $rows = [];
        while ($row = $result->fetch_assoc()) $rows[] = $row;

        $totalCount = null;
        if ($totals || $countOnly) {
            $countSql = "SELECT COUNT(*) AS c FROM `$table` WHERE $whereSql";
            $cres = $conn->query($countSql);
            if ($cres && $crow = $cres->fetch_assoc()) $totalCount = (int)$crow['c'];
            else $totalCount = 0;
        }

        return [
            "ok" => true,
            "rows" => $rows,
            "totalsEnabled" => $totals,
            "countOnly" => $countOnly,
            "totals" => [
                "count" => $totalCount,
                "skip" => $skip,
                "max" => $max
            ]
        ];
    }
}

if (!function_exists('restSendCollectionResponse')) {
    function restSendCollectionResponse($rows, $queryMeta) {
        header('Content-Type: application/json');

        if (!empty($queryMeta['countOnly'])) {
            echo json_encode([
                "data" => [],
                "totals" => [
                    "count" => (int)$queryMeta['totals']['count']
                ]
            ], JSON_PRETTY_PRINT);
            return;
        }

        if (!empty($queryMeta['totalsEnabled'])) {
            echo json_encode([
                "data" => $rows,
                "totals" => [
                    "count" => (int)$queryMeta['totals']['count'],
                    "skip" => (int)$queryMeta['totals']['skip'],
                    "max" => (int)$queryMeta['totals']['max']
                ]
            ], JSON_PRETTY_PRINT);
            return;
        }

        echo json_encode($rows, JSON_PRETTY_PRINT);
    }
}
