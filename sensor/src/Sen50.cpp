/*
	@author:  Thomas Stadel
	@date:    2024-11-22
	@brief:   Driver for Sensirion SEN50 particle sensor
*/

#include "Sen50.h"

Sen50::Sen50() {
	return;
}

void Sen50::loop() {
	return;
}

void Sen50::on() {
    // Start measurement
	uint8_t buf[] = { 0x00, 0x21 };
	writeToDevice(buf, 2);
	delay(50);
}

void Sen50::off() {
    // Stop measurement
	uint8_t buf[] = { 0x01, 0x04 };
	writeToDevice(buf, 2);
}

// Returns samples in factor x10
int8_t Sen50::getSample(float_t *pm) {
	// Read measured values
	uint8_t buf[] = { 0x03, 0xC4 };
	writeToDevice(buf, 2);
	delay(20);

	// Read result
	uint8_t result[24];
	readFromDevice(result, 24);

	// Parse results
	pm[0] = parseSample(result, 0) / 10.; // PM1
	pm[1] = parseSample(result, 3) / 10.; // PM2.5
	pm[2] = parseSample(result, 6) / 10.; // PM4
	pm[3] = parseSample(result, 9) / 10.; // PM10

	// Check for data error
	if (pm[0] == 0 || pm[1] == 0 || pm[2] == 0 || pm[3] == 0) {
		return -1;
	}
	else {
		return 0;
	}
}

void Sen50::writeToDevice(uint8_t *buf, size_t len) {
	Wire.beginTransmission(SEN50_ADR);
	Wire.write(buf, len);
	Wire.endTransmission();
}

void Sen50::readFromDevice(uint8_t *buf, size_t len) {
	memset(buf, 0, len);
	Wire.requestFrom(SEN50_ADR, len);
	size_t i = 0;
	while (Wire.available() && i < len) {
		buf[i] = Wire.read();
		i++;
	}
}

uint16_t Sen50::parseSample(uint8_t *buf, size_t offset) {
	uint16_t result = (buf[offset] << 8) | buf[offset + 1];
	
	// Check CRC - return 0 on error
	if (buf[offset+2] != calcCRC(result)) return 0;

	return result;
}

// CRC for Sensirion SEN50
uint8_t Sen50::calcCRC(uint16_t pm) {
	// uint16_t to uint8_t[2] conversion
	uint8_t data[2];
	data[0] = (pm >> 8) & 0xFF;
	data[1] = pm & 0xFF;

	// Calc CRC
    unsigned char crc = 0xFF;
    for(int i = 0; i < 2; i++) {
        crc ^= data[i];
        for (unsigned char bit = 8; bit > 0; --bit) {
            if(crc & 0x80) {
                crc = (crc << 1) ^ 0x31u;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}