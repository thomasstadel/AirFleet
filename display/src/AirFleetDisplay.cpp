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

// BLE connected callback
void connectedCallback(const BlePeerDevice& peer, void* context) {
	lcd.setCursor(0, 3);
	lcd.print("Forbundet           ");
}

// BLE disconnected callback
void disconnectedCallback(const BlePeerDevice& peer, void* context) {
	lcd.setCursor(0, 3);
	lcd.print("Afbrudt             ");
}

// LCD clear
void onLcdClear(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
	lcd.clear();
}

// LCD move cursor
void onLcdSetCursor(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
	if (len != 2) return;
	lcd.setCursor(data[0], data[1]);
}

// LCD print
void onLcdPrint(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
	for (size_t i = 0; i < len; i++) lcd.print((char)data[i]);
}

// BLE services and characteristics
BleUuid serviceUuid("46dec950-753c-44e3-abc3-bdfd08d63cfe");
BleUuid lcdClearCharacteristicUuid("520be753-2aa6-455e-b449-558a4555687e");
BleUuid lcdSetCursorCharacteristicUuid("520be753-2bb6-455e-b449-558a4555687e");
BleUuid lcdPrintCharacteristicUuid("520be753-2cc6-455e-b449-558a4555687e");
BleCharacteristic lcdClearCharacteristic("clear", BleCharacteristicProperty::WRITE, lcdClearCharacteristicUuid, serviceUuid, onLcdClear, NULL);
BleCharacteristic lcdSetCursorCharacteristic("setcursor", BleCharacteristicProperty::WRITE, lcdSetCursorCharacteristicUuid, serviceUuid, onLcdSetCursor, NULL);
BleCharacteristic lcdPrintCharacteristic("print", BleCharacteristicProperty::WRITE, lcdPrintCharacteristicUuid, serviceUuid, onLcdPrint, NULL);

void setup() {
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
	BLE.addCharacteristic(lcdSetCursorCharacteristic);
	BLE.addCharacteristic(lcdPrintCharacteristic);

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
	// Do nothing here
}
