<?php
// =============================================================
// Endpoint: /_ping
// Risposta costante per compatibilita' client: []
// =============================================================

require_once __DIR__ . '/../config.php';

header('Content-Type: application/json');
echo '[]';

$conn->close();

