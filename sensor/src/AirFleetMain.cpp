/*
	@author:  Thomas Stadel
	@date:    2024-12-15
	@brief:   AirFleet mobile air quality sensor
*/

#include "Particle.h"

#include "BleLcd.h"
#include "Sen50.h"
#include "Htu31.h"
#include "Mics.h"
#include "L86.h"

#include "Settings.h"

// Run manual mode so we can control WiFi etc.
SYSTEM_MODE(MANUAL);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Log handler (can be disabled in Settings.h)
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

// Past averages [CO2, PM1, PM2.5, PM4, PM10]
float_t past_average[5] = {0., 0., 0., 0., 0.};

// States
enum State {
  INIT,
  IDLE,
  SAMPLE,
  PUBLISH,
  SLEEP,
  LEVELS
};
State state = INIT;

// Prototypes
String generate_scale(float_t val, float_t minval, float_t maxval, float_t prev, uint8_t width);
void airfleet_levels(const char *event, const char *data);
void disconnectCloud();
bool connectCloud();
void triggerSample();
bool isIgnitionOn();
float_t getBatteryV();

// Timer that triggers sample every X sec.
Timer sampleTimer(SAMPLE_INTERVAL_MS, triggerSample);

void setup() {
#ifdef AIRFLEET_DEBUG
  // Wait for USB serial
  waitFor(Serial.isConnected, 15000);
  delay(1000);
#endif

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
  Particle.subscribe("hook-response/air-quality-request", airfleet_levels, MY_DEVICES);

  // Start sample timer
  sampleTimer.start();
}

void loop() {
  // Last millis, when data was published to cloud
  static system_tick_t publishTime = 0;

  // Last millis, when we got average PMs
  static system_tick_t levelsTime = 0;

  // Last known good samples
  static float_t log_pm[4] = { 0., 0., 0., 0. };
  static float_t log_th[2] = { 0., 0. };
  static uint16_t log_cv[2] = { 0, 0 };
  static String log_datetime = "0000-00-00 00:00:00";
  static float_t log_gps[4] = { 0., 0., 0., 0. };

  String datetime;
  float_t pm[4];
  uint8_t pm_result;
  float_t th[2];
  uint8_t th_result;
  uint16_t cv[2];
  uint8_t cv_result;
  float_t gps[4];
  uint8_t gps_result;

  // State machine
  switch (state) {
    case INIT:
      // Wake up everything
      sen50.on();
      lcd.on();
      htu31.on();
      mics.on();
      l86.on();

      // Take first sample after init
      state = SAMPLE;

      break;

    case IDLE:
      // Housekeeping for different modules
      lcd.loop();
      sen50.loop();
      htu31.loop();
      mics.loop();
      l86.loop();

      // Check if ignition is off
      if (!isIgnitionOn()) state = SLEEP;

      // Check if we need to publish to cloud
      else if (publishTime + PUBLISH_INTERVAL_MS <= millis()) state = PUBLISH;

      break;

    case SAMPLE:
      // Get sample
      state = IDLE;

      // Get sample for all sensors
      gps_result = l86.getSample(gps, &datetime);
      pm_result = sen50.getSample(pm);
      th_result = htu31.getSample(th);
      cv_result = mics.getSample(cv);

      // Particles
      if (pm_result == 0 && memcmp(pm, log_pm, sizeof(pm)) != 0) {
        // Find max PM part
        float_t maxpm = 0;
        for (size_t i = 0; i < sizeof(pm) / sizeof(pm[0]); i++) {
          if (pm[i] > maxpm) maxpm = pm[i];
        }

        // Update LCD
        lcd.print(0, 2, "PM  " + generate_scale(maxpm, PM_MIN, PM_MAX,
          (past_average[1] + past_average[2] + past_average[3] + past_average[4]) / 4., // Average PM
          16)); // + " " + String::format("%.1f", maxpm));

        memcpy(log_pm, pm, sizeof(log_pm));
      }

      // Temperature / humidity
      if (th_result == 0 && memcmp(th, log_th, sizeof(th)) != 0) {
        // Show temp at top right in format: XXXC
        lcd.print(16, 0, String::format("%3.0fC", th[0]));

        // Show humidity right on second line: XX%RH
        lcd.print(8, 0, String::format("%2.0f%%RH", th[1]));

        memcpy(log_th, th, sizeof(log_th));
      }

      // CO2 / VOC
      if (cv_result == 0 && memcmp(cv, log_cv, sizeof(cv)) != 0) {
        lcd.print(0, 1, "CO2 " + generate_scale((float_t)cv[1], CO2_MIN, CO2_MAX,
          past_average[0], 16)); // + " " + String(cv[1]));

        memcpy(log_cv, cv, sizeof(log_cv));
      }

      // GPS
      if (gps_result == 0) {
        // Show clock at top left 2024-11-11 11:11:11
        if (log_datetime.substring(11, 16) != datetime.substring(11, 16)) {
          lcd.print(0, 0, datetime.substring(11, 16));
        }
        log_datetime = datetime;

        if (memcmp(gps, log_gps, sizeof(gps)) != 0) {
          memcpy(log_gps, gps, sizeof(log_gps));
        }
      }
      else {
        // Show there's no GPS
        lcd.print(0, 0, "?GPS?");
      }

      // Show alerts
      if (log_pm[0] >= PM_MAX || log_pm[1] >= PM_MAX || log_pm[2] >= PM_MAX || log_pm[3] >= PM_MAX) {
        lcd.enableFlash(0, 3, "   PM NIVEAU H0J    ", 750);
      }
      else if (log_cv[1] >= CO2_MAX) {
        lcd.enableFlash(0, 3, "   CO2 NIVEAU H0J   ", 750);
      }
      else {
        lcd.disableFlash();
        lcd.print(9, 3, "OK");
      }

      // Check if we need to publish data
      if (log_gps[3] >= PUBLISH_INTERVAL_KM) state = PUBLISH;

#ifdef AIRFLEET_DEBUG
      Log.info("---------------");

      // Car battery
      Log.info("Battery voltage: %.1fV", getBatteryV());

      // Particles
      if (pm_result == 0) {
        Log.info("PM1: %.1f, PM2.5: %.1f, PM4: %.1f, PM10: %.1f", pm[0], pm[1], pm[2], pm[3]);
      }
      else {
        Log.error("Could not get particles in air");
      }

      // Temperature
      if (th_result == 0) {
        Log.info("Temperature: %.1f, humidity: %.1f", th[0], th[1]);
      }
      else {
        Log.error("Could not get temperature and humidity");
      }

      // CO2 / VOC
      if (cv_result == 0) {
        Log.info("VOC: %d, CO2: %d", cv[0], cv[1]);
      }
      else {
        Log.error("Could not get VOC/CO2");
      }

      // GPS
      if (gps_result == 0) {
        Log.info("GPS UTC time: %s", (const char*)datetime);
        Log.info("GPS Latitude: %f, Longitude: %f, Speed: %f, Distance: %f", gps[0], gps[1], gps[2], gps[3]);
      }
      else if (gps_result == 1) {
        Log.error("GPS looking for satelittes");
      }
      else {
        Log.error("No data from GPS");
      }

      Log.info("---------------");
#endif

      break;

    case PUBLISH:
      // Publish last sample to cloud

      state = IDLE;
      if (!connectCloud()) break;

#ifdef AIRFLEET_DEBUG
      Log.info("Publishing data to cloud");
#endif

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

      // Check if we need to update the past PM levels from cloud
      if (levelsTime == 0 || levelsTime + LEVELS_INTERVAL_MS <= millis()) {
        state = LEVELS;
      }
      else {
        // Disconnect WiFi
        disconnectCloud();
      }

      break;

    case LEVELS:
      // Update average particle levels
      state = IDLE;
      if (!connectCloud()) break;

#ifdef AIRFLEET_DEBUG
        Log.info("AIRFLEET_LEVELS Requesting...");
#endif

      // Request particle levels
      if (Particle.publish("air-quality-request", "", PRIVATE)) levelsTime = millis();

      break;

    case SLEEP:
      // Go to sleep mode

#ifdef AIRFLEET_DEBUG
      Log.info("=== GOING TO SLEEP  ===");
#endif

      // Update LCD
      lcd.disableFlash();
      lcd.clear();
      lcd.print(0, 1, "    TAENDING FRA");

      // Put everything asleep
      sen50.off();
      lcd.off();
      htu31.off();
      mics.off();
      l86.off();

      // Put Photon 2 to sleep, and wakeup every X sec to check for ignition
      SystemSleepConfiguration config;
      config.mode(SystemSleepMode::STOP).duration(IGNITION_ON_INTERVAL_MS);
      while (!isIgnitionOn()) System.sleep(config);

#ifdef AIRFLEET_DEBUG
      Log.info("=== WOKE UP ===");
#endif

      state = INIT;

      break;
  }
}

// Timer for triggering sampling
void triggerSample() {
  if (state != SLEEP) state = SAMPLE;
}

// Start and check connection to Particle cloud
bool connectCloud() {
    // Check WiFi
    if (WiFi.isOff()) {

#ifdef AIRFLEET_DEBUG
      Log.info("CLOUD connecting");
#endif

      WiFi.on();
      WiFi.connect();
      Particle.connect();
      return false;
    }
    if (!WiFi.ready()) return false;

    // Check Particle cloud
    if (!Particle.connected()) return false;

    return true;
}

// End connection to Particle cloud
void disconnectCloud() {
  if (WiFi.isOff()) return;

#ifdef AIRFLEET_DEBUG
  Log.info("CLOUD disconnecting");
#endif

  Particle.disconnect();
  WiFi.disconnect();
  WiFi.off();
}

// Callback when we get past average levels
void airfleet_levels(const char *event, const char *data) {
#ifdef AIRFLEET_DEBUG
  Log.info("AIRFLEET_LEVELS CALLBACK event: %s, data: %s", event, data);
#endif

  // Parse JSON
  JSONValue jsondata = JSONValue::parseCopy(data);

  // Iterate through elements in JSON
  JSONObjectIterator iter(jsondata);
  while (iter.next()) {

#ifdef AIRFLEET_DEBUG    
    Log.info("JSON %s: %s", (const char *)iter.name(), (const char *)iter.value().toString());
#endif

    if (iter.name() == "co2") {
      past_average[0] = (float_t)iter.value().toDouble();
    }
    else if (iter.name() == "pm1") {
      past_average[1] = (float_t)iter.value().toDouble();
    }
    else if (iter.name() == "pm25") {
      past_average[2] = (float_t)iter.value().toDouble();
    }
    else if (iter.name() == "pm4") {
      past_average[3] = (float_t)iter.value().toDouble();
    }
    else if (iter.name() == "pm10") {
      past_average[4] = (float_t)iter.value().toDouble();
    }
  }

  disconnectCloud();
}

// Function generating a scale, e.g.:
// [---    |    ]
String generate_scale(float_t val, float_t minval, float_t maxval, float_t prev, uint8_t width) {
  String scale = "[";
  int8_t val_i = (int8_t)((val - minval) / (maxval - minval) * (float_t)(width - 2));
  if (val_i < 0) val_i = 0;
  if (val_i > width - 3) val_i = width - 3;
  int8_t prev_i = (int8_t)((prev - minval) / (maxval - minval) * (float_t)(width - 2));
  if (prev_i < 0) prev_i = 0;
  if (prev_i > width - 3) prev_i = width - 3;
  for (uint8_t i = 0; i < width - 2; i++) {
    if (val_i >= i && prev_i == i) {
      scale.concat("+");
    }
    else if (val_i >= i) {
      scale.concat("-");
    }
    else if (prev_i == i) {
      scale.concat("|");
    }
    else {
      scale.concat(" ");
    }
  }
  scale.concat("]");
  return scale;
}

bool isIgnitionOn() {
  return getBatteryV() >= IGNITION_ON_V;
}

float_t getBatteryV() {
  return analogRead(VIN_REF_PIN) * VIN_REF_FACTOR;
}