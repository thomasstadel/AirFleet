/*
	@author:  Thomas Stadel
	@date:    2024-11-21
	@brief:   AirFleet mobile air quality sensor
*/

#include "Particle.h"

#include "BleLcd.h"
#include "Sen50.h"
#include "Htu31.h"
#include "Mics.h"
#include "L86.h"

#include "Settings.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
#ifdef AIRFLEET_DEBUG
SerialLogHandler logHandler(LOG_LEVEL_INFO);
#endif

// BLE LCD
BleLcd lcd;

// Sensirion SEN50: particle sensor - has absolutely nothing to do with particle.io :-P
Sen50 sen50;

// HTU31 sensor: temperature + humidity
Htu31 htu31;

// MiCS sensor: VOC+CO2
Mics mics;

// L86 GPS module
L86 l86;

// States
enum State {
  INIT,
  IDLE,
  SAMPLE,
  PUBLISH,
  ALERT,
  SLEEP
};
State state = INIT;

// Timer for triggering sampling
void triggerSample() {
  if (state != SLEEP) state = SAMPLE;
}
Timer sampleTimer(SAMPLE_INTERVAL_MS, triggerSample);

void airfleet_levels(const char *event, const char *data) {
  Log.info("airfleet_levels event: %s, data: %s", event, data);
}

void setup() {
#ifdef AIRFLEET_DEBUG
  // Wait for USB serial
  waitFor(Serial.isEnabled, 15000);
#endif

  // Configure pins
  pinMode(IGNITION_PIN, INPUT);

  // Setup I2C
  Wire.setSpeed(I2C_SPEED);
  Wire.begin();

  // Setup sensors
  lcd = BleLcd();
  sen50 = Sen50();
  htu31 = Htu31();
  mics = Mics();
  l86 = L86();

  // Subscribe to air quality levels
  Particle.subscribe("airfleet_levels", airfleet_levels, MY_DEVICES);

  // Start sample timer
  sampleTimer.start();
}

void loop() {
  // Last millis data was published to cloud
  static unsigned long publishTime = 0;

  // Last known good samples
  static float_t log_pm[4] = { 0., 0., 0., 0. };
  static float_t log_th[2] = { 0., 0. };
  static uint16_t log_cv[2] = { 0, 0 };
  static String log_datetime = "0000-00-00 00:00:00";
  static float_t log_gps[4] = { 0., 0., 0., 0. };

  String datetime;

  switch (state) {
    case INIT:

      // TODO: Request current sensor levels from webservice

      // Wake up everything
      sen50.on();
      lcd.on();
      htu31.on();
      mics.on();
      l86.on();

      state = IDLE;

      break;

    case IDLE:
      // Housekeeping for different modules
      lcd.loop();
      sen50.loop();
      htu31.loop();
      mics.loop();
      l86.loop();

      // Check if we need to publish to cloud
      if (publishTime + PUBLISH_INTERVAL_MS <= millis()) state = PUBLISH;

      // Check if ignition is off
      if (digitalRead(IGNITION_PIN) == LOW) state = SLEEP;

      break;

    case SAMPLE:

      Log.info("---------------");

      // Check if ignition is off
      Log.info("Battery voltage: %.1fV", analogRead(VIN_REF_PIN) * VIN_REF_FACTOR);

      state = IDLE;

      // Particles
      float_t pm[4];
      if (sen50.getSample(pm) == 0) {
        Log.info("PM1: %.1f, PM2.5: %.1f, PM4: %.1f, PM10: %.1f", pm[0], pm[1], pm[2], pm[3]);
        memcpy(log_pm, pm, sizeof(log_pm));
      }
      else {
        Log.error("Could not get particles in air");
      }

      // Temperature
      float_t th[2];
      if (htu31.getSample(th) == 0) {
        Log.info("Temperature: %.1f, humidity: %.1f", th[0], th[1]);
        memcpy(log_th, th, sizeof(log_th));
      }
      else {
        Log.error("Could not get temperature and humidity");
      }

      // CO2 / VOC
      uint16_t cv[2];
      if (mics.getSample(cv) == 0) {
        Log.info("VOC: %d, CO2: %d", cv[0], cv[1]);
        memcpy(log_cv, cv, sizeof(log_cv));
      }
      else {
        Log.error("Could not get VOC/CO2");
      }

      // GPS
      float gps[4];
      int8_t gps_valid;
      gps_valid = l86.getSample(gps, &datetime);
      Log.info("GPS valid: %d", gps_valid);
      Log.info("UTC time: %s", (const char*)datetime);
      Log.info("Latitude: %f, Longitude: %f, Speed: %f, Distance: %f", gps[0], gps[1], gps[2], gps[3]);
      if (gps_valid == 0) {
        memcpy(log_gps, gps, sizeof(log_gps));
        log_datetime = datetime;

        if (gps[3] >= PUBLISH_INTERVAL_KM) state = PUBLISH;
      }

      Log.info("---------------");

      // TODO: Update LCD

      break;

    case PUBLISH:

#ifdef AIRFLEET_DEBUG
      Log.info("Publishing data to cloud");
#endif

      state = IDLE;

      // Build JSON string
      char buf[255];
      snprintf(buf, sizeof(buf),
        "{"
        "\"pm1\":%.1f,\"pm25\":%.1f,\"pm4\":%.1f,\"pm10\":%.1f,"   // Particles
        "\"temp\":%.1f,\"humi\":%.1f,"                             // Temperature + humidity
        "\"voc\":%d,\"co2\":%d,"                                   // VOC + CO2
        "\"lat\":%.6f,\"lng\":%.6f,"                               // GPS lat, lng
        "\"time\":\"%s\""                                          // UTC time
        "}",
        log_pm[0], log_pm[1], log_pm[2], log_pm[3],
        log_th[0], log_th[1],
        log_cv[0], log_cv[1],
        log_gps[0], log_gps[1],
        (const char*)log_datetime
        );

      // Publish to cloud
      Particle.publish("airfleet_push", buf, PRIVATE);

      // Reset distance + publishtime, so publish will be triggered in X millisec./km
      l86.reset_distance();
      publishTime = millis();

      break;

    case ALERT:

      // TODO: Display alert on LCD

      break;

    case SLEEP:

#ifdef AIRFLEET_DEBUG
      Log.info("=== GOING TO SLEEP  ===");
#endif

      // Put everything asleep
      sen50.off();
      lcd.off();
      htu31.off();
      mics.off();
      l86.off();

      // Put Photon 2 to sleep, and wakeup on ignition pin
      SystemSleepConfiguration config;
      config.mode(SystemSleepMode::STOP).gpio(IGNITION_PIN, RISING);
      System.sleep(config);

#ifdef AIRFLEET_DEBUG
      Log.info("=== WOKE UP ===");
#endif

      state = INIT;

      break;
  }
}
