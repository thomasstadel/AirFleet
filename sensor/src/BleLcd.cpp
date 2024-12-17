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
	lcdBleFlashCharacteristicUuid = BleUuid(BLE_LCD_FLASH_UUID);

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

	// Clear buffers
	memset(curLCD, 32, sizeof(curLCD));
	curLCD[0] = 0;
	memset(curFlash, 0, sizeof(curFlash));
}

void BleLcd::off() {
    // Turn off BLE
    state = IDLE;
    BLE.off();
}

void BleLcd::clearCurrent() {
	memset(curLCD, 32, sizeof(curLCD));
	curLCD[0] = 0;
	memset(curFlash, 0, sizeof(curFlash));
}

char BleLcd::clear() {
	// Prepare buffer
	const uint8_t buf[] = {0};

	// Check for changes
	if (state != INIT && curLCD[0] == 0) return 0;
	curLCD[0] = 0;

    if (state != INIT && state != READY) return -1;

    // Send
	lcdClearCharacteristic.setValue(buf, 1);

    return 0;
}

// Print message
char BleLcd::print(const char x, const char y, const uint8_t *buf, size_t len) {
	uint8_t buf2[255];
	buf2[0] = x;
	buf2[1] = y;
	size_t len2 = 0;
	uint8_t changes = 0;
	for (len2 = 0; len2 < len && len2 < 250; len2++) {
		buf2[len2+2] = buf[len2];

		// Check for changes
		if (curLCD[x + 20*y + len2] != buf[len2]) {
			changes++;
			curLCD[x + 20*y + len2] = buf[len2];
		}
	}

	if (curLCD[0] == 0) curLCD[0] = 32;

    if (state != INIT && state != READY) return -1;
	if (state != INIT && changes == 0) return 0;

    // Send
	lcdPrintCharacteristic.setValue(buf2, len2+2);
    
    return len2+2;
}
char BleLcd::print(const char x, const char y, const String str) {
	uint8_t buf[255];
	size_t len;
	for (len = 0; len < str.length() && len < 255; len++) buf[len] = (char)str.charAt(len);

	return print(x, y, buf, len);
}

// Flash text on screen, by show and hiding text
char BleLcd::enableFlash(const char x, const char y, const String str, const uint16_t interval) {
	uint8_t buf[255];
	memset(buf, 0, sizeof(buf));

	// data[0] = x
	// data[1] = y
	// data[2] = interval MSB, interval = 0 => disable
	// data[3] = interval LSB
	buf[0] = x;
	buf[1] = y;
	buf[2] = (interval >> 8) & 0xFF;
	buf[3] = interval & 0xFF;

	size_t i;
	for (i = 0; i < str.length() && i < 250; i++) {
		buf[i+4] = (uint8_t)str.charAt(i);
	}

	// Check for changes
	if (memcmp(curFlash, buf, i) == 0) return 0;
	memcpy(curFlash, buf, sizeof(buf));

    if (state != READY) return -1;

    // Send
    lcdFlashCharacteristic.setValue(buf, str.length() + 4);

    return str.length() + 4;
}
char BleLcd::disableFlash() {
	uint8_t buf[4];
	memset(buf, 0, sizeof(buf));

	// Check for changes
	if (memcmp(curFlash, buf, sizeof(buf)) == 0) return 0;
	memcpy(curFlash, buf, sizeof(buf));

	if (state != READY) return -1;

	lcdFlashCharacteristic.setValue(buf, sizeof(buf));

	return sizeof(buf);
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
	peer.getCharacteristicByUUID(lcdFlashCharacteristic, lcdBleFlashCharacteristicUuid);

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

    // Init state
    state = INIT;

	// Update LCD with recent data
	if (curLCD[0] == 0) {
		// Clear
		Log.info("READY clear");
		clear();
	}
	else {
		// Print
		Log.info("READY print");
		for (size_t y = 0; y <= 3; y++) {
			uint8_t buf[255];
			for (size_t len = 0; len < 20; len++) {
				buf[len] = curLCD[20*y + len];
			}
			print(0, y, buf, 20);
		}
	}
	if (curFlash[2] > 0 || curFlash[3] > 0) {
		// Flash
		Log.info("READY flash");
		size_t len = 4;
		while (len < 255 && curFlash[len] != 0) len++;
		lcdFlashCharacteristic.setValue(curFlash, len);
	}

	// Ready state
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