/*
    @brief  Settings for AirFleet

            This file is excluded from github, so please remember to update
            the example file accordingly when making any changes below.
*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include "Particle.h"

// Uncomment to enable debug mode
#define AIRFLEET_DEBUG

// Sample interval
#define SAMPLE_INTERVAL_MS        5000

// Publish interval
#define PUBLISH_INTERVAL_MS       60000
#define PUBLISH_INTERVAL_KM       0.1

// Request levels interval
#define LEVELS_INTERVAL_MS        3600000

// CO2 and PM scale for bar graph
#define CO2_MIN                   400.
#define CO2_MAX                   1000.
#define PM_MIN                    0.
#define PM_MAX                    5.

// I2C speed
#define I2C_SPEED                  CLOCK_SPEED_100KHZ

// Ignition / battery-voltage
#define VIN_REF_PIN                A5
#define VIN_REF_FACTOR             16.5/4095.
#define IGNITION_ON_V              0.
#define IGNITION_CHECK_INTERVAL    15000

// BLE LCD
#define BLE_LCD_SERVICE_UUID	   "46dec950-753c-44e3-abc3-bdfd08d63cfe"
#define BLE_LCD_CLEAR_UUID		   "520be753-2aa6-455e-b449-558a4555687e"
#define BLE_LCD_SET_CURSOR_UUID    "520be753-2bb6-455e-b449-558a4555687e"
#define BLE_LCD_PRINT_UUID		   "520be753-2cc6-455e-b449-558a4555687e"
#define BLE_LCD_FLASH_UUID		   "520be753-2dd6-455e-b449-558a4555687e"

// HTU31 temperature and humidity sensor
#define HTU31_ADR                   0x40

// L86 GPS module
#define L86_RESET_PIN               D3
#define L86_FORCEON_PIN             D2
#define L86_SERIAL                  Serial1
#define L86_BAUDRATE                115200
//#define L86_DEBUG_SPEED             36.
#define L86_DISTANCE_BY_SPEED
//#define L86_DISTANCE_BY_HAVERSINE

// MiCS CO2/VOC sensor
#define MICS_ADR                    0x70

// Sensirion SEN50 particle sensor
#define SEN50_ADR                   0x69

#endif