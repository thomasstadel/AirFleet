/*
	@author:  Thomas Stadel
	@date:    2024-11-21
	@brief:   AirFleet mobile air quality sensor
*/

#include "Particle.h"
#include "BleLcd.h"
#include "Sen50.h"

// Settings
#define SAMPLE_INTERVAL_MILLISECONDS  5000
#define SAMPLE_INTERVAL_METERS        500
#define I2C_SPEED                     CLOCK_SPEED_100KHZ

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// BLE LCD
BleLcd lcd = BleLcd();

// Sensirion SEN50
Sen50 sen50 = Sen50();

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
Timer sampleTimer(SAMPLE_INTERVAL_MILLISECONDS, triggerSample);

void setup() {
  // Setup I2C
  Wire.setSpeed(I2C_SPEED);
  Wire.begin();

  // Setup Sensirion SEN50
  sen50.on();

  // Setup BLE to LCD display
  lcd.on();

  // Start sample timer
  sampleTimer.start();
}

void loop() {

  String str;

  switch (state) {
    case State::IDLE:
      lcd.loop();

      break;

    case State::SAMPLE:

      // Get samples from Sensirion
      uint16_t pm[4];
      sen50.getSample(pm);

      lcd.clear();

      str = String::format("PM1: %d", pm[0]/10);
      lcd.setCursor(0, 0);
      lcd.print(str);

      str = String::format("PM2.5: %d", pm[1]/10);
      lcd.setCursor(10, 0);
      lcd.print(str);

      str = String::format("PM4: %d", pm[2]/10);
      lcd.setCursor(0, 1);
      lcd.print(str);

      str = String::format("PM10: %d", pm[3]/10);
      lcd.setCursor(10, 1);
      lcd.print(str);

      state = State::IDLE;

      break;

    case State::TRANSMIT:
      break;

    case State::SLEEP:
      break;
  }
}
