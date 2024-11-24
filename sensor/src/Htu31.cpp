/*
	@author:  Thomas Stadel
	@date:    2024-11-22
	@brief:   Driver for TE HTU31D temperature and humidity sensor
*/

#include "Htu31.h"

Htu31::Htu31() {
	return;
}

void Htu31::loop() {
	return;
}

void Htu31::on() {
	// Soft reset = 15 ms
	uint8_t buf[] = { 0x1E };
	writeToDevice(buf, 1);
	delay(15);
}

void Htu31::off() {
    // Stop measurement
}

// Returns samples
int8_t Htu31::getSample(float_t *th) {
	uint8_t buf[6];

	// Start conversion
	buf[0] = 0x40;
	writeToDevice(buf, 1);
	delay(3); // Convertion time 1,0ms (RH) + 1,6ms (T) = 2,6ms at 0,02%RH and 0,04C resolution

	// Request reading temp. and humidity
	buf[0] = 0x00;
	writeToDevice(buf, 1);

	// Read from device
	memset(buf, 0, 6);
	readFromDevice(buf, 6);

	// Parse temperature
	uint16_t t = parseSample(buf, 0);

	// Parse humidity
	uint16_t h = parseSample(buf, 3);

	// Check for data error
	if (t == 0 || h == 0) {
		th[0] = 0;
		th[1] = 0;
		return -1;
	}

	// Conversion
	th[0] = -40. + 165. * (float_t)t / 65535.; 	// Temperature: -40 + 165 * t / (2^16 - 1)
	th[1] = 100. * (float_t)h / 65535;		    // Humidity: 100 * h / (2^16 - 1)
	return 0;
}

void Htu31::writeToDevice(uint8_t *buf, size_t len) {
	Wire.beginTransmission(HTU31_ADR);
	Wire.write(buf, len);
	Wire.endTransmission();
}

void Htu31::readFromDevice(uint8_t *buf, size_t len) {
	memset(buf, 0, len);
	Wire.requestFrom(HTU31_ADR, len);
	size_t i = 0;
	while (Wire.available() && i < len) {
		buf[i] = Wire.read();
		i++;
	}
}

uint16_t Htu31::parseSample(uint8_t *buf, size_t offset) {
	uint16_t result = (buf[offset] << 8) | buf[offset + 1];
	
	// Check CRC - return 0 on error
	if (buf[offset+2] != calcCRC(result)) return 0;

	return result;
}

// CRC for Sensirion SEN50
uint8_t Htu31::calcCRC(uint16_t pm) {
	// uint16_t to uint8_t[2] conversion
	uint8_t data[2];
	data[0] = (pm >> 8) & 0xFF;
	data[1] = pm & 0xFF;

	// Calc CRC TODO: Check CRC calculation
    unsigned char crc = 0x00;
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