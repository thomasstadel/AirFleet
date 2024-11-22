/*
	@author:  Thomas Stadel
	@date:    2024-11-21
	@brief:   Driver for connecting to BLE enabled LCD display
*/

#ifndef SEN50_H
#define SEN50_H

#include "Particle.h"

#define SEN50_ADR 0x69

class Sen50 {
	public:
		Sen50();

		void on();
		void off();
		void loop();
		void getSample(uint16_t *pm);

	private:
		void writeToDevice(uint8_t *buf, size_t len);
		void readFromDevice(uint8_t *buf, size_t len);
		uint16_t parseSample(uint8_t *buf, size_t offset);
		uint8_t calcCRC(uint16_t pm);
};

#endif
