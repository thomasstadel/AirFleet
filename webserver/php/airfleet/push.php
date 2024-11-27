<?php
    /*
        @brief      Endpoint for airfleet pushing data to cloud.
                    Stores entries in MySQL db.
        @author     Thomas Stadel
        @date       2024-11-27
    */

    // Include config
    require_once("config.php");

    // Read JSON
    if (!$json = json_decode(file_get_contents("php://input"), true)) die("Unable to parse JSON");

    // Data
    if (!$data = json_decode($json["data"], true)) die("Unable to parse data");

    // TODO: Add some authenticity

    // Validate data
    $expected_fields = array(
        "pm1" => "/^[0-9\.]+$/",
        "pm25" => "/^[0-9\.]+$/",
        "pm4" => "/^[0-9\.]+$/",
        "pm10" => "/^[0-9\.]+$/",
        "temp" => "/^[0-9\.]+$/",
        "humi" => "/^[0-9\.]+$/",
        "voc" => "/^[0-9]+$/",
        "co2" => "/^[0-9]+$/",
        "lat" => "/^[0-9\.]+$/",
        "lng" => "/^[0-9\.]+$/",
        "time" => "/^20[0-9]{2}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}$/"
    );
    $fields = array();
    $values = array();
    foreach ($expected_fields as $field => $regex) {
        if (empty($data[$field])) die("Missing field: $field");
        if (!preg_match($regex, $data[$field])) die("Invalid data in field: $field");
        $values[] = $data[$field];
    }

    // Connect to DB
    $db = new mysqli($_db_hostname, $_db_username, $_db_password, $_db_database);
    if ($db->connect_errno) die("DB error 1");

    // Prepare query
    $sql = "INSERT INTO airfleet_log " .
        "(" . implode(",", array_keys($expected_fields)) . ") " .
        "VALUES " .
        "(" . implode(",", array_fill(0, count($expected_fields), "?")) . ")";
    if (!$stmt = $db->prepare($sql)) die("DB error 2");

    // Bind parameters
    if (!$stmt->bind_param("ddddddiidds", ...$values)) die("DB error 3");

    // Insert into DB
    if (!$stmt->execute()) die("DB error 4");

    // All OK
    die("OK");