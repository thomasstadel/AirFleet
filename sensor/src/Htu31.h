/*
	@author:  Thomas Stadel
	@date:    2024-11-21
	@brief:   Driver for TE HTU31D temperature and humidity sensor
*/

#ifndef HTU31_H
#define HTU31_H

#include "Particle.h"

#define HTU31_ADR 0x40

class Htu31 {
	public:
		Htu31();

		void on();
		void off();
		void loop();
		int8_t getSample(float_t *pm);

	private:
		void writeToDevice(uint8_t *buf, size_t len);
		void readFromDevice(uint8_t *buf, size_t len);
		uint16_t parseSample(uint8_t *buf, size_t offset);
		uint8_t calcCRC(uint16_t pm);
};

#endif
