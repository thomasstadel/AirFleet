/*
    @brief  Example settings for AirFleet
*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include "Particle.h"

// Debug mode
#define AIRFLEET_DEBUG

// Sample interval
#define SAMPLE_INTERVAL_MS         2000

// Log interval
#define LOG_INTERVAL_MS            60000
#define LOG_INTERVAL_METERS        500

// I2C speed
#define I2C_SPEED                  CLOCK_SPEED_100KHZ

// BLE LCD
#define BLE_LCD_SERVICE_UUID	   "46dec950-753c-44e3-abc3-bdfd08d63cfe"
#define BLE_LCD_CLEAR_UUID		   "520be753-2aa6-455e-b449-558a4555687e"
#define BLE_LCD_SET_CURSOR_UUID    "520be753-2bb6-455e-b449-558a4555687e"
#define BLE_LCD_PRINT_UUID		   "520be753-2cc6-455e-b449-558a4555687e"

// HTU31 temperature and humidity sensor
#define HTU31_ADR 0x40

// L86 GPS module
#define L86_RESET_PIN               D3
#define L86_FORCEON_PIN             D2
#define L86_SERIAL                  Serial1
#define L86_DISTANCE_BY_SPEED
//#define L86_DISTANCE_BY_HAVERSINE

// MiCS CO2/VOC sensor
#define MICS_ADR                    0x70

// Sensirion SEN50 particle sensor
#define SEN50_ADR                   0x69

#endif