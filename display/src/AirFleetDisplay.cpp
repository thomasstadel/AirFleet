/*
	@author: Thomas Stadel
	@date: 2024-11-21
	@brief: BLE peripheral controlling LCD display
			Based on example script from docs.particle.io
*/

#include "Particle.h"
#include "LiquidCrystal.h"

// Manual mode, since no WiFi needed for this device
SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

// Initiate lcd driver
LiquidCrystal lcd(D7, D6, D5, D4, D3, D2);

// Mutex lock to avoid async functions to communicate with LCD simultaneously
os_mutex_t lcdLock;

// Flash state
enum FlashState {
	DISABLE,
	PRINT,
	WAIT_CLEAR,
	CLEAR,
	WAIT_PRINT
};
FlashState flashState = DISABLE;
String flashString = "";
String flashClearString = "";
unsigned char flashX = 0;
unsigned char flashY = 0;
unsigned long flashTime = 0;
unsigned long flashInterval = 0;

// BLE connected callback
void connectedCallback(const BlePeerDevice& peer, void* context) {
	os_mutex_lock(lcdLock);
	lcd.setCursor(0, 3);
	lcd.print("Forbundet           ");
	os_mutex_unlock(lcdLock);
}

// BLE disconnected callback
void disconnectedCallback(const BlePeerDevice& peer, void* context) {
	os_mutex_lock(lcdLock);
	flashState = DISABLE;
	lcd.setCursor(0, 3);
	lcd.print("Afbrudt             ");
	os_mutex_unlock(lcdLock);
}

// LCD clear
void onLcdClear(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
	os_mutex_lock(lcdLock);
	lcd.clear();
	os_mutex_unlock(lcdLock);
}

// LCD print at x, y
void onLcdPrint(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
	// data[0] = x
	// data[1] = y

	String str = "";
	for (size_t i = 2; i < len; i++) str.concat((char)data[i]);
	
	os_mutex_lock(lcdLock);
	lcd.setCursor(data[0], data[1]);
	lcd.print(str);
	os_mutex_unlock(lcdLock);
}

// LCD flash
void onLcdFlash(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {

	// If current state == WAIT_CLEAR then CLEAR display now
	if (flashState == WAIT_CLEAR) {
		os_mutex_lock(lcdLock);
		lcd.setCursor(flashX, flashY);
		lcd.print(flashClearString);
		os_mutex_unlock(lcdLock);
	}

	if (((data[2] << 8) | data[3]) == 0) {
		flashState = DISABLE;
		return;
	}

	// data[0] = x
	// data[1] = y
	// data[2] = interval MSB, interval = 0 => disable
	// data[3] = interval LSB
	if (len < 4) return;
	flashX = data[0];
	flashY = data[1];
	flashInterval = (data[2] << 8) | data[3];
	flashString = "";
	flashClearString = "";
	for (size_t i = 4; i < len; i++) {
		flashString.concat((char)data[i]);
		flashClearString.concat(" ");
	}
	flashState = PRINT;
}

// BLE services and characteristics
BleUuid serviceUuid("46dec950-753c-44e3-abc3-bdfd08d63cfe");
BleUuid lcdClearCharacteristicUuid("520be753-2aa6-455e-b449-558a4555687e");
BleUuid lcdPrintCharacteristicUuid("520be753-2cc6-455e-b449-558a4555687e");
BleUuid lcdFlashCharacteristicUuid("520be753-2dd6-455e-b449-558a4555687e");
BleCharacteristic lcdClearCharacteristic("clear", BleCharacteristicProperty::WRITE, lcdClearCharacteristicUuid, serviceUuid, onLcdClear, NULL);
BleCharacteristic lcdPrintCharacteristic("print", BleCharacteristicProperty::WRITE, lcdPrintCharacteristicUuid, serviceUuid, onLcdPrint, NULL);
BleCharacteristic lcdFlashCharacteristic("flash", BleCharacteristicProperty::WRITE, lcdFlashCharacteristicUuid, serviceUuid, onLcdFlash, NULL);

void setup() {
	// Create mutex
	os_mutex_create(&lcdLock);

	// Turn on BLE
	BLE.on();

	// Setup callbacks
	BLE.onConnected(connectedCallback, NULL);
	BLE.onDisconnected(disconnectedCallback, NULL);

	// Setting paringmode
	BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_ONLY);
	BLE.setPairingAlgorithm(BlePairingAlgorithm::LESC_ONLY);

	// Add BLE characteristics
    BLE.addCharacteristic(lcdClearCharacteristic);
	BLE.addCharacteristic(lcdPrintCharacteristic);
	BLE.addCharacteristic(lcdFlashCharacteristic);

	// Setup advertisement data
    BleAdvertisingData advData;
    advData.appendServiceUUID(serviceUuid);

	// Start advertising device
    BLE.advertise(&advData);

	// Setup LCD and show current state
	lcd.begin(20, 4);
	lcd.clear();
	lcd.setCursor(0, 3);
	lcd.print("Afventer BLE        ");
}

void loop() {
	switch (flashState) {
		case DISABLE:
			break;
		case PRINT:
			os_mutex_lock(lcdLock);
			lcd.setCursor(flashX, flashY);
			lcd.print(flashString);
			os_mutex_unlock(lcdLock);
			flashState = WAIT_CLEAR;
			flashTime = millis();
			break;
		case WAIT_CLEAR:
			if (millis() - flashTime >= flashInterval) flashState = CLEAR;
			break;
		case CLEAR:
			os_mutex_lock(lcdLock);
			lcd.setCursor(flashX, flashY);
			lcd.print(flashClearString);
			os_mutex_unlock(lcdLock);
			flashState = WAIT_PRINT;
			flashTime = millis();
			break;
		case WAIT_PRINT:
			if (millis() - flashTime >= flashInterval) flashState = PRINT;
			break;
	}
}
