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
        "pm1" => array("/^[0-9\.]+$/", "d"),
        "pm25" => array("/^[0-9\.]+$/", "d"),
        "pm4" => array("/^[0-9\.]+$/", "d"),
        "pm10" => array("/^[0-9\.]+$/", "d"),
        "temp" => array("/^[0-9\.]+$/", "d"),
        "humi" => array("/^[0-9\.]+$/", "d"),
        "voc" => array("/^[0-9]*$/", "i"),
        "co2" => array("/^[0-9]*$/", "i"),
        "lat" => array("/^[0-9\.]+$/", "d"),
        "lng" => array("/^[0-9\.]+$/", "d"),
        "time" => array("/^20[0-9]{2}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}$/", "s")
    );
    $fields = array();
    $values = array();
    $types = array();
    foreach ($expected_fields as $field => $arr) {
        if (!preg_match($arr[0], $data[$field])) die("Invalid data in field: $field");
        if (isset($data[$field]) and strlen($data[$field]) > 0) {
            $fields[] = $field;
            $values[] = $data[$field];
            $types[] = $arr[1];
        }
    }
    if (count($fields) == 0) die("No data to store");

    // Connect to DB
    $db = new mysqli($_db_hostname, $_db_username, $_db_password, $_db_database);
    if ($db->connect_errno) die("DB error 1");

    // Prepare query
    $sql = "INSERT INTO airfleet_log " .
        "(" . implode(",", $fields) . ") " .
        "VALUES " .
        "(" . implode(",", array_fill(0, count($fields), "?")) . ")";
    if (!$stmt = $db->prepare($sql)) die("DB error 2");

    // Bind parameters
    if (!$stmt->bind_param(implode("", $types), ...$values)) die("DB error 3");

    // Insert into DB
    if (!$stmt->execute()) die("DB error 4");

    // All OK
    die("OK");