-- @brief   SQL structure for AirFleet database
-- @author  Thomas Stadel
-- @date    2024-11-27

CREATE TABLE `airfleet_log` (
  `time` datetime NOT NULL,
  `pm1` decimal(10,1) UNSIGNED DEFAULT NULL,
  `pm25` decimal(10,1) UNSIGNED DEFAULT NULL,
  `pm4` decimal(10,1) UNSIGNED DEFAULT NULL,
  `pm10` decimal(10,1) UNSIGNED DEFAULT NULL,
  `temp` decimal(10,1) DEFAULT NULL,
  `humi` decimal(10,1) UNSIGNED DEFAULT NULL,
  `voc` int UNSIGNED DEFAULT NULL,
  `co2` int UNSIGNED DEFAULT NULL,
  `lat` decimal(10,6) DEFAULT NULL,
  `lng` decimal(10,6) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

ALTER TABLE `airfleet_log`
  ADD PRIMARY KEY (`time`);
COMMIT;
