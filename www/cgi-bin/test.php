<?php

header("Content-Type: text/plain");

echo "Hello from PHP CGI!\n";
echo "===================\n\n";

echo "METHOD: " . $_SERVER['REQUEST_METHOD'] . "\n";
echo "QUERY_STRING: " . $_SERVER['QUERY_STRING'] . "\n";
echo "CONTENT_TYPE: " . (isset($_SERVER['CONTENT_TYPE']) ? $_SERVER['CONTENT_TYPE'] : "None") . "\n\n";

echo "--- GET DATA ---\n";
print_r($_GET);

echo "\n--- POST DATA ---\n";
print_r($_POST);

echo "\n--- RAW BODY (stdin) ---\n";
echo file_get_contents('php://input') . "\n";
?>