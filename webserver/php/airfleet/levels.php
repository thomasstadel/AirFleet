<?php
    /*
        @brief      Returns average levels from last 24 hours
        @author     Thomas Stadel
        @date       2024-12-16
    */

    // Include config
    require_once("config.php");

    // Validate client
    if (empty($_SERVER["HTTP_API_KEY"]) or $_SERVER["HTTP_API_KEY"] != $_api_token) {
        http_response_code(403);
        die("Forbidden");
    }

    // Connect to DB
    $db = new mysqli($_db_hostname, $_db_username, $_db_password, $_db_database);
    if ($db->connect_errno) die("DB error 1");

    // Get average
    $res = $db->query("SELECT AVG(co2) AS co2, AVG(pm1) AS pm1, AVG(pm25) AS pm25, AVG(pm4) AS pm4, AVG(pm10) AS pm10 " . 
        "FROM airfleet_log WHERE time >= '" . date("Y-m-d H:i:s", strtotime("-1 day")) . "'");
    if (!$res) die("DB error 2");

    // Get result row
    $row = $res->fetch_assoc();
    if (!$row) die("DB error 3");
    foreach ($row as $key => $val) $row[$key] = number_format($val, 1, ".", "");

    // Headers
    header("Content-type: application/json");

    echo(json_encode($row));