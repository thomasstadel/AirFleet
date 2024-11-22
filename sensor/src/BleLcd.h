/*
	@author:  Thomas Stadel
	@date:    2024-11-21
	@brief:   Driver for connecting to BLE enabled LCD display
*/

#ifndef BLE_LCD_H
#define BLE_LCD_H

#include "Particle.h"

#define BLE_LCD_SERVICE_UUID    "46dec950-753c-44e3-abc3-bdfd08d63cfe"
#define BLE_LCD_CLEAR_UUID      "520be753-2aa6-455e-b449-558a4555687e"
#define BLE_LCD_SET_CURSOR_UUID "520be753-2bb6-455e-b449-558a4555687e"
#define BLE_LCD_PRINT_UUID      "520be753-2cc6-455e-b449-558a4555687e"

class BleLcd {
	public:
		BleLcd();

		void on();
		void off();
		void loop();
		char clear();
		char setCursor(const char x, const char y);
		char print(const uint8_t *buf, size_t len);
		char print(const String str);

	private:
		enum class State {
			IDLE,
			SCAN,
			WAIT,
			CONNECT,
			PAIR,
			READY
		};
		State state;

		BleUuid lcdBleServiceUuid;
		BleUuid lcdBleClearCharacteristicUuid;
		BleUuid lcdBleSetCursorCharacteristicUuid;
		BleUuid lcdBlePrintCharacteristicUuid;

		BlePeerDevice peer;
		BleCharacteristic lcdClearCharacteristic;
		BleCharacteristic lcdSetCursorCharacteristic;
		BleCharacteristic lcdPrintCharacteristic;

		unsigned long stateTime;
		BleAddress serverAddr;

		static void scanResultCallback(const BleScanResult *scanResult, void *context);
		void scanResult(const BleScanResult *scanResult);
		void stateConnect();
		void statePair();
		void stateReady();
};

#endif
