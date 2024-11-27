/*
	@author:  Thomas Stadel
	@date:    2024-11-21
	@brief:   Driver for Sensirion SEN50 particle sensor
*/

#ifndef SEN50_H
#define SEN50_H

#include "Particle.h"
#include "Settings.h"

class Sen50 {
	public:
		Sen50();

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
