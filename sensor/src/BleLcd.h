/*
	@author:  Thomas Stadel
	@date:    2024-11-21
	@brief:   Driver for connecting to BLE enabled LCD display
*/

#ifndef BLE_LCD_H
#define BLE_LCD_H

#include "Particle.h"
#include "Settings.h"

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
		enum State {
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
