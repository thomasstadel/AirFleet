/*
	@author:  Thomas Stadel
	@date:    2024-11-23
	@brief:   Driver for MiCS-VZ-89TE CO2 and VOC
			  I2C refs:
			  https://www.sgxsensortech.com/content/uploads/2016/07/VZ869-E-Version-rev-B-I2C-communication-quick-manual.pdf
			  https://www.sgxsensortech.com/content/uploads/2017/03/I2C-Datasheet-MiCS-VZ-89TE-rev-H-ed170214-Read-Only.pdf
*/

#ifndef MICS_H
#define MICS_H

#include "Particle.h"

#define MICS_ADR 0x70

class Mics {
	public:
		Mics();

		void on();
		void off();
		void loop();
		int8_t getSample(uint16_t *pm);

	private:
		void writeToDevice(uint8_t *buf, size_t len);
		void readFromDevice(uint8_t *buf, size_t len);
		uint16_t parseSample(uint8_t *buf, size_t offset);
		uint8_t calcCRC(uint8_t *buffer, size_t size);
};

#endif
