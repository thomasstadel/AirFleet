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

// Settings
#define SAMPLE_INTERVAL_MS            2000
#define SAMPLE_INTERVAL_METERS        500
#define I2C_SPEED                     CLOCK_SPEED_100KHZ

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(MANUAL);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// BLE LCD
BleLcd lcd = BleLcd();

// Sensirion SEN50: particle sensor - has absolutely nothing to do with particle.io :-P
Sen50 sen50 = Sen50();

// HTU31 sensor: temperature + humidity
Htu31 htu31 = Htu31();

// MiCS sensor: VOC+CO2
Mics mics = Mics();

// L86 GPS module
L86 l86 = L86(SAMPLE_INTERVAL_MS);

// States
enum class State {
  IDLE,
  SAMPLE,
  TRANSMIT,
  SLEEP
};
State state = State::IDLE;

// Timer for triggering sampling
void triggerSample() {
  if (state == State::IDLE) state = State::SAMPLE;
}
Timer sampleTimer(SAMPLE_INTERVAL_MS, triggerSample);

void setup() {
  // Setup I2C
  Wire.setSpeed(I2C_SPEED);
  Wire.begin();

  // Turn on sensors / actuators
  sen50.on();
  lcd.on();
  htu31.on();
  mics.on();
  l86.on();

  // Start sample timer
  sampleTimer.start();
}

void loop() {

  String str;
  String datetime;

  switch (state) {
    case State::IDLE:
      // Housekeeping for different modules
      lcd.loop();
      sen50.loop();
      htu31.loop();
      mics.loop();
      l86.loop();

      break;

    case State::SAMPLE:

      // Particles
      float_t pm[4];
      if (sen50.getSample(pm) == 0) {
        str = String::format("PM1: %.1f, PM2.5: %.1f, PM4: %.1f, PM10: %.1f", pm[0], pm[1], pm[2], pm[3]);
        Log.info(str);
      }
      else {
        Log.error("Could not get particles in air");
      }

      // Temperature
      float_t th[2];
      if (htu31.getSample(th) == 0) {
        str = String::format("Temperature: %.1f, humidity: %.1f", th[0], th[1]);
        Log.info(str);
      }
      else {
        Log.error("Could not get temperature and humidity");
      }

      // CO2 / VOC
      uint16_t cv[2];
      if (mics.getSample(cv) == 0) {
        str = String::format("VOC: %d, CO2: %d", cv[0], cv[1]);
        Log.info(str);
      }
      else {
        Log.error("Could not get VOC/CO2");
      }

      // GPS
      float data[3];
      l86.getSample(data, &datetime);
      Log.info(datetime);

      Log.info("---------------");

      state = State::IDLE;

      break;

    case State::TRANSMIT:
      break;

    case State::SLEEP:
      break;
  }
}
