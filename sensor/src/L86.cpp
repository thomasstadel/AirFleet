/*
	@author:  Thomas Stadel
	@date:    2024-11-23
	@brief:   Driver for Queltec L86 GPS module
*/

#include "L86.h"

L86::L86(unsigned long sample_interval_ms) {
	// Open UART
	L86_SERIAL.begin(9600);

	// Set baudrate of GPS module to 115200
	L86_SERIAL.println(appendCRC("$PMTK251,115200*"));

	// Change baudrate of serial
	L86_SERIAL.flush();
	L86_SERIAL.end();
	delay(10);
	L86_SERIAL.begin(115200);

	// Set fixpoint interval
	String str = "$PMTK220,";
	str.concat(sample_interval_ms);
	str.concat("*");
	str.concat(calcCRC(str));
	L86_SERIAL.println(str);

	// Set defaults
	gps_valid = -1;
	gps_latitude = 0.;
	gps_longitude = 0.;
	gps_speed = 0.;
	gps_datetime = "0000-00-00 00:00:00";
}

void L86::loop() {
	// Empty serial buffer
	while (L86_SERIAL.available()) readBuffer.concat((char)L86_SERIAL.read());

	// Check for newlines
	int idx = readBuffer.indexOf("\r\n");
	while (idx > -1) {
		String str = readBuffer.substring(0, idx);
		readBuffer = readBuffer.substring(idx + 2);
		
		// Check if this is GPS position data
		if (str.startsWith("$GNRMC,") && str.indexOf("*") > -1) {
			// Build datetime string
			String date_part = getPart(str, 9);
			String time_part = getPart(str, 1);
			if (date_part.length() == 6 && time_part.length() == 10) {
				// Seems ok
				gps_datetime = "20"; // TODO: In year 2100, please change this to 21,
									 // and increment this todo. I wont be there to
									 // thank you, so thank you in advance!

				// Date part
				gps_datetime.concat(date_part.substring(4, 6)); // Year
				gps_datetime.concat("-");
				gps_datetime.concat(date_part.substring(2, 4)); // Month
				gps_datetime.concat("-");
				gps_datetime.concat(date_part.substring(0, 2)); // Date
				gps_datetime.concat(" ");

				// Time part
				gps_datetime.concat(time_part.substring(0, 2)); // Hour
				gps_datetime.concat(":");
				gps_datetime.concat(time_part.substring(2, 4)); // Minute
				gps_datetime.concat(":");
				gps_datetime.concat(time_part.substring(4, 6)); // Second
			}

			// Check if data is valid
			if (getPart(str, 2) == "A") {
				// Valid GPS position
				gps_latitude = 0.;
				gps_longitude = 0.;

				gps_valid = 0;
			}
			else {
				// No valid GPS position
				gps_latitude = 0.;
				gps_longitude = 0.;

				gps_valid = 1;
			}
		}

		idx = readBuffer.indexOf("\r\n");
	}
}

void L86::on() {
	// TODO: turn GPS on

	// You're a dirty little GPS module - come to daddy!
}

void L86::off() {
	// TODO: Put GPS in sleepmode

	// Where's my chloroform!?
}

// Returns:
//   0 on valid position
//   1 on no valid position
//  -1 if no data from module
// Data contains: [latitude, longitude, speed km/t]
// Datetime format: yyyy-mm-dd hh:mm:ss
int8_t L86::getSample(float_t *data, String *datetime) {
	data[0] = gps_latitude;
	data[1] = gps_longitude;
	data[2] = gps_speed;

	*datetime = gps_datetime;

	return gps_valid;
}

// Returns part of string separated by comma
String L86::getPart(String str, int part) {
	int curpart = 0;
	int offset = 0;
	int idx = 0;
	while (idx > -1 && (unsigned int)offset < str.length()) {
		idx = str.indexOf(',', offset);
		if (curpart == part) {
			if (idx == -1) return str.substring(offset);
			return str.substring(offset, idx);
		}
		offset = idx + 1;
		curpart++;
	}
	return "";
}

String L86::trim(String str) {
	// Whitespaces to remove
	String whitespaces = "\r\n";
	// Remove at beginning
	while (str.length() > 0 && whitespaces.indexOf(str.charAt(0)) > -1) {
		str = str.substring(1);
	}
	// Remove at end
	while (str.length() > 0 && whitespaces.indexOf(str.charAt(str.length() - 1)) > -1) {
		str = str.substring(0, str.length() - 1);
	}
	return str;
}

String L86::appendCRC(String str) {
	str.concat(calcCRC(str));
	return str;
}

String L86::calcCRC(String str) {
	if (!str.startsWith("$") || !str.endsWith("*")) return "";

	String substr = str.substring(1, str.length() - 1);
	if (substr.length() == 0) return "";

	uint8_t crc = (char)substr.charAt(0);
	for (size_t i = 1; i < substr.length(); i++) {
		crc = crc ^ substr.charAt(i);
	}
	return String::format("%02x", crc);
}