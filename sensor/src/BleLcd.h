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
		char print(const char x, const char y, const uint8_t *buf, size_t len);
		char print(const char x, const char y, const String str);
		char enableFlash(const char x, const char y, const String str, const uint16_t interval);
		char disableFlash();
		void clearCurrent();

	private:
		enum State {
			IDLE,
			SCAN,
			WAIT,
			CONNECT,
			PAIR,
			INIT,
			READY
		};
		State state;

		BleUuid lcdBleServiceUuid;
		BleUuid lcdBleClearCharacteristicUuid;
		BleUuid lcdBleSetCursorCharacteristicUuid;
		BleUuid lcdBlePrintCharacteristicUuid;
		BleUuid lcdBleFlashCharacteristicUuid;

		BlePeerDevice peer;
		BleCharacteristic lcdClearCharacteristic;
		BleCharacteristic lcdSetCursorCharacteristic;
		BleCharacteristic lcdPrintCharacteristic;
		BleCharacteristic lcdFlashCharacteristic;

		unsigned long stateTime;
		BleAddress serverAddr;

		uint8_t curLCD[255];
		uint8_t curFlash[255];

		static void scanResultCallback(const BleScanResult *scanResult, void *context);
		void scanResult(const BleScanResult *scanResult);
		void stateConnect();
		void statePair();
		void stateReady();
};

#endif
