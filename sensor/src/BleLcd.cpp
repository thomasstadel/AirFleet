/*
	@author:  Thomas Stadel
	@date:    2024-11-22
	@brief:   Driver for connecting to BLE enabled LCD display
*/

#include "BleLcd.h"

BleLcd::BleLcd() {
	lcdBleServiceUuid = BleUuid(BLE_LCD_SERVICE_UUID);
	lcdBleClearCharacteristicUuid = BleUuid(BLE_LCD_CLEAR_UUID);
	lcdBleSetCursorCharacteristicUuid = BleUuid(BLE_LCD_SET_CURSOR_UUID);
	lcdBlePrintCharacteristicUuid = BleUuid(BLE_LCD_PRINT_UUID);

    // Initial state
    state = IDLE;
}

void BleLcd::loop() {
	switch(state) {
		case SCAN:
			state = WAIT;
			BLE.scan(scanResultCallback, this);
			break;
			
		case CONNECT:
			stateConnect();
			break;

		case WAIT:
            // Restart scan every 5 sec.
			if (millis() - stateTime >= 5000) state = SCAN;
			break;

        case PAIR:
            statePair();
            break;

        case READY:
            stateReady();
            break;
	}
}

void BleLcd::on() {
    // Turn on BLE
    BLE.on();
	BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_YESNO);
	BLE.setPairingAlgorithm(BlePairingAlgorithm::LESC_ONLY);

    // Start scanning
    state = SCAN;
}

void BleLcd::off() {
    // Turn off BLE
    state = IDLE;
    BLE.off();
}

char BleLcd::clear() {
    if (state != READY) return -1;

	// Prepare buffer
	const uint8_t buf[] = {0};

    // Send
	lcdClearCharacteristic.setValue(buf, 1);

    return 0;
}

// Move cursor to x, y
char BleLcd::setCursor(const char x, const char y) {
    if (state != READY) return -1;

    // Prepare buffer
	const uint8_t buf[] = {x, y};

    // Send
	lcdSetCursorCharacteristic.setValue(buf, 2);

    return 0;
}

// Print message
char BleLcd::print(const uint8_t *buf, size_t len) {
    if (state != READY) return -1;

    // Send
	lcdPrintCharacteristic.setValue(buf, len);
    
    return len;
}
char BleLcd::print(const String str) {
    if (state != READY) return -1;

    // Send
    lcdPrintCharacteristic.setValue(str);

    return strlen(str);
}

// Callback when device is discovered
void BleLcd::scanResultCallback(const BleScanResult *scanResult, void *context) {
	BleLcd* ctx = static_cast<BleLcd*>(context);
	ctx->scanResult(scanResult);
}
void BleLcd::scanResult(const BleScanResult *scanResult) {
	BleUuid foundServiceUuid;
	size_t svcCount = scanResult->advertisingData().serviceUUID(&foundServiceUuid, 1);

    // Check if this is the correct device
	if (svcCount == 0 || !(foundServiceUuid == BLE_LCD_SERVICE_UUID)) return;

    // Found our device
	serverAddr = scanResult->address();

    // Go to connect state
	state = CONNECT;

	// Stop scanning
	BLE.stopScanning();
}

// Connects to device
void BleLcd::stateConnect() {
    // Connect
	peer = BLE.connect(serverAddr);
	if (!peer.connected()) {
        // Go to wait state before restarting a scan
		state = WAIT;
		stateTime = millis();
		return;
	}

    // Connected - getting info about services
	peer.getCharacteristicByUUID(lcdClearCharacteristic, lcdBleClearCharacteristicUuid);
	peer.getCharacteristicByUUID(lcdSetCursorCharacteristic, lcdBleSetCursorCharacteristicUuid);
	peer.getCharacteristicByUUID(lcdPrintCharacteristic, lcdBlePrintCharacteristicUuid);

    // Start pairing
	BLE.startPairing(peer);
	state = PAIR;

	stateTime = millis();
}

// Check pairing state
void BleLcd::statePair() {
	if (!peer.connected()) {
		// Lost connection - go to wait before scanning
		state = WAIT;
		stateTime = millis();
		return;
	}
    
	if (BLE.isPairing(peer)) {
		// Still pairing
		return;
	}

    // Ready to rock
    state = READY;
}

// Check connection
void BleLcd::stateReady() {
	if (!peer.connected()) {
		// Lost connection - go to wait before scanning
		state = WAIT;
		stateTime = millis();
		return;
	}    
}

