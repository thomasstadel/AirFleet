/*
	@author:  Thomas Stadel
	@date:    2024-11-23
	@brief:   Driver for MiCS-VZ-89TE CO2 and VOC
*/

#include "Mics.h"

Mics::Mics() {
}

void Mics::loop() {
}

void Mics::on() {
}

void Mics::off() {
}

// Returns samples
int8_t Mics::getSample(uint16_t *cv) {
	uint8_t buf[7];

	// getStatus command
	buf[0] = 0x0C;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	buf[5] = calcCRC(buf, 5);
	writeToDevice(buf, 6);
	delay(100);

	// Read sample
	readFromDevice(buf, 7);

	// Check CRC
	if (buf[6] != calcCRC(buf, 6)) return -1;

	// Conversion
	cv[0] = ((uint16_t)buf[0] - 13) * 1000/229; 	  // VOC
	cv[1] = ((uint16_t)buf[1] - 13) * 1600/400 + 400; // CO2
	return 0;
}

void Mics::writeToDevice(uint8_t *buf, size_t len) {
	Wire.beginTransmission(MICS_ADR);
	Wire.write(buf, len);
	Wire.endTransmission();
}

void Mics::readFromDevice(uint8_t *buf, size_t len) {
	memset(buf, 0, len);
	Wire.requestFrom(MICS_ADR, len);
	size_t i = 0;
	while (Wire.available() && i < len) {
		buf[i] = Wire.read();
		i++;
	}
}

/*
    CRC-function for MiCS sensor
    https://www.sgxsensortech.com/content/uploads/2017/03/I2C-Datasheet-MiCS-VZ-89TE-rev-H-ed170214-Read-Only.pdf
*/
uint8_t Mics::calcCRC(uint8_t *buffer, size_t size) {
    /* Local variable */
    uint8_t crc = 0x00;
    uint8_t i = 0x00;
    uint16_t sum = 0x0000;

    /* Summation with carry */
    for(i = 0; i < size; i++) {
        sum += (uint16_t)buffer[i];
    }

    crc = (uint8_t)sum;
    crc += (sum / 0x0100); // Add with carry
    crc = 0xFF-crc; // Complement
    
    /* Returning results*/
    return crc;
}