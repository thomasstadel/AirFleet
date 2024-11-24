/*
	@author:  Thomas Stadel
	@date:    2024-11-23
	@brief:   Driver for Queltec L86 GPS module
			  Ref: https://docs.rs-online.com/3824/0900766b8147dbf6.pdf
*/

#ifndef L86_H
#define L86_H

#include "Particle.h"

#define L86_SERIAL Serial1

class L86 {
	public:
		L86(unsigned long sample_interval_ms);

		void on();
		void off();
		void loop();
		int8_t getSample(float_t *data, String *datetime);

	private:
		void writeToDevice(uint8_t *buf, size_t len);
		void readFromDevice(uint8_t *buf, size_t len);
		uint16_t parseSample(uint8_t *buf, size_t offset);
		String appendCRC(String str);
		String calcCRC(String str);
		String trim(String str);
		String getPart(String str, int part);
		String readBuffer;
		float_t gps_latitude;
		float_t gps_longitude;
		float_t gps_speed;
		uint8_t gps_valid;
		String gps_datetime;
};

#endif
