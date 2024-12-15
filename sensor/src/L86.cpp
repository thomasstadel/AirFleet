/*
	@author:  Thomas Stadel
	@date:    2024-11-25
	@brief:   Driver for Queltec L86 GPS module
*/

#include "L86.h"

L86::L86() {
	// Setup GPIOs
	pinMode(L86_RESET_PIN, OUTPUT);
	pinMode(L86_FORCEON_PIN, OUTPUT);

	digitalWrite(L86_RESET_PIN, LOW);
	digitalWrite(L86_FORCEON_PIN, HIGH);
	delay(50);
	
	// Reset module to make sure we're in correct state
	reset();

	// Open UART
	L86_SERIAL.begin(9600);
	delay(1000);

	String str;
	if (L86_BAUDRATE != 9600) {
		// Empty read buffer
		while (L86_SERIAL.available()) L86_SERIAL.read();

		// Set baudrate of GPS module
		str = "$PMTK251,";
		str.concat(L86_BAUDRATE);
		str.concat("*");
		str.concat(calcCRC(str));
		str.concat("\r\n");
		L86_SERIAL.print(str);

		// Change baudrate of serial
		L86_SERIAL.flush();
		L86_SERIAL.end();

		L86_SERIAL.begin(L86_BAUDRATE);
		waitFor(L86_SERIAL.isEnabled, 5000);
		delay(1000);
	}

	// Empty read buffer
	while (L86_SERIAL.available()) L86_SERIAL.read();

	// Set fixpoint interval
	str = "$PMTK220,";
	str.concat(SAMPLE_INTERVAL_MS);
	str.concat("*");
	str.concat(calcCRC(str));
	str.concat("\r\n");
	L86_SERIAL.print(str);
#ifdef AIRFLEET_DEBUG
	Log.info("GPS setting interval: %s", (const char*)str);
#endif

	// Set defaults
	gps_valid = -1;
	gps_latitude = 0.;
	gps_longitude = 0.;
	gps_prev_latitude = 0.;
	gps_prev_longitude = 0.;
	gps_speed = 0.;
	gps_distance = 0.;
	gps_millis = 0;
	gps_datetime = "0000-00-00 00:00:00";	
}

void L86::reset() {
	/*
		Reset module according to datasheet.
		Ref: https://docs.rs-online.com/1fe5/0900766b8147dbfb.pdf

		Note, that the logic is inverted, because a transistor is
		used for pulling down the reset pin, according to schematics.
		Ref: https://github.com/thomasstadel/AirFleet/blob/main/schematics/sensor%20board/AirFleet.kicad_sch
	*/
	digitalWrite(L86_FORCEON_PIN, LOW);
	digitalWrite(L86_RESET_PIN, LOW);
	delay(5); // > 2ms
	digitalWrite(L86_RESET_PIN, HIGH);
	delay(15); // > 10ms
	digitalWrite(L86_RESET_PIN, LOW);
	delay(15); // Make sure module has restarted
}

void L86::loop() {
	// Empty serial buffer
	while (L86_SERIAL.available()) {
		// Read single character
		char c = (char)L86_SERIAL.read();

		// Start new command
		if (c == '$') {
			readBuffer = c;

			if (gps_valid == -1) gps_valid = 1;
		}
		
		// Check for newlines
		else if (c == '\r') {
			String str = trim(readBuffer);
			readBuffer = "";

			// Check if this is GPS position data
			// Expected format: $GNRMC,105117.000,A,5626.2207,N,00922.2751,E,0.00,2.02,251124,,,A,V*00
			if (str.startsWith("$GNRMC,") && str.indexOf("*") > -1) {

	#ifdef AIRFLEET_DEBUG
				Log.info("GPS data: %s", (const char*)str);
	#endif

				// CRC check
				if (str.substring(str.length() - 2) == calcCRC(str.substring(0, str.length() - 2))) {
					// CRC OK

	#ifdef AIRFLEET_DEBUG
					Log.info("GPS data CRC OK");
	#endif

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
						// Valid data

						// GPS position
						gps_latitude = calcDecimalDegrees(getPart(str, 3));
						if (getPart(str, 4) == "S") gps_latitude *= -1;
						gps_longitude = calcDecimalDegrees(getPart(str, 5));
						if (getPart(str, 6) == "W") gps_longitude *= -1;

						// Speed - knots to km/t. 1 knot = 1.852 km/t
						// Ref: https://en.wikipedia.org/wiki/Knot_(unit)
						gps_speed = getPart(str, 7).toFloat() / 1.852;

						// Calculate distance since last sample
	#ifdef L86_DISTANCE_BY_SPEED
						// Calculate distance using time delta and speed
						if (gps_millis > 0) {
							unsigned long time_delta = millis() - gps_millis;
							gps_distance += gps_speed / 3600000. * time_delta;
						}
	#else
		#ifdef L86_DISTANCE_BY_HAVERSINE
						// Calculate distance using Haversine formula
						if (gps_prev_latitude != 0. && gps_prev_longitude != 0.) {

							/*
								BEGIN Haversine implementation
								This code originates from:
								https://www.geeksforgeeks.org/haversine-formula-to-find-distance-between-two-points-on-a-sphere/
							*/

							// distance between latitudes and longitudes
							double_t dLat = (gps_prev_latitude - gps_latitude) * M_PI / 180.0;
							double_t dLon = (gps_prev_longitude - gps_longitude) * M_PI / 180.0;
					
							// convert to radians
							double_t lat1 = (gps_prev_latitude) * M_PI / 180.0;
							double_t lat2 = (gps_latitude) * M_PI / 180.0;
					
							// apply formulae
							double_t a = pow(sin(dLat / 2), 2) + 
									pow(sin(dLon / 2), 2) * 
									cos(lat1) * cos(lat2);
							double_t rad = 6371;
							double_t c = 2 * asin(sqrt(a));
							gps_distance += (float_t)(rad * c);

							/*
								END Haversine implementation
							*/
						}
		#endif
	#endif

						gps_prev_latitude = gps_latitude;
						gps_prev_longitude = gps_longitude;
						gps_millis = millis();
						gps_valid = 0;
					}
					else {
						// No valid GPS position
						gps_latitude = 0.;
						gps_longitude = 0.;
						gps_speed = 0.;
						gps_valid = 1;
					}
				}
				else {
					// CRC error
	#ifdef AIRFLEET_DEBUG				
					Log.error("CRC error reading from GPS-module. Got CRC: %s, calculated CRC: %s",
						(const char*)str.substring(str.length() - 2),
						(const char*)calcCRC(str.substring(0, str.length() - 2)));
	#endif
				}
			}
		}

		// Append to buffer
		else  {
			readBuffer.concat(c);
		}
	}
}

void L86::on() {
	// Empty readbuffer
	readBuffer = "";

	// Make sure serial is enabled
	if (!L86_SERIAL.isEnabled()) {
		L86_SERIAL.begin(L86_BAUDRATE);
		waitFor(L86_SERIAL.isEnabled, 5000);
		delay(1000);
	}

	// Force the GPS module on
	digitalWrite(L86_FORCEON_PIN, HIGH);
	delay(50);
}

void L86::off() {
	// Send GPS module to backup mode
	digitalWrite(L86_FORCEON_PIN, LOW);
	delay(10);
	L86_SERIAL.println("$PMTK225,4*2F");
	L86_SERIAL.flush();
	delay(10);
	L86_SERIAL.end();
}

void L86::reset_distance() {
	gps_distance = 0.;
}

// Returns:
//   0 on valid position
//   1 on no valid position
//  -1 if no data from module
// Data contains: [latitude, longitude, speed km/t, traveled distance in km]
// Datetime format: yyyy-mm-dd hh:mm:ss
int8_t L86::getSample(float_t *data, String *datetime) {
	data[0] = gps_latitude;
	data[1] = gps_longitude;
	data[2] = gps_speed;
	data[3] = gps_distance;

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

float_t L86::calcDecimalDegrees(String str) {
    // Expected format: DDDMM.MMMM or DDMM.MMMM

    // Find .
    size_t pos = str.indexOf(".");
    if (pos == std::string::npos) return 0.;

    // Get degrees
    String degStr = str.substring(0, pos - 2);
    float_t deg = degStr.toFloat();
    
    // Get minutes
    String minStr = str.substring(pos - 2);
    float_t min = minStr.toFloat();

    // Return degrees
    return deg + min / 60.;
}

String L86::trim(String str) {
	// Whitespaces to remove
	String whitespaces = "\r\n\t";
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

// $GNRMC,135030.000,A,5626.2161,N,00922.2802,E,0.00,294.78,261124,,,A35030.000,5626.2161,N,00922.2802,E,1,14,0.69,7.4,M,43.1,M,,*65

String L86::calcCRC(String str) {
	if (!str.startsWith("$") || !str.endsWith("*")) return "";

	String substr = str.substring(1, str.length() - 1);
	if (substr.length() == 0) return "";

#ifdef AIRFLEET_DEBUG
	Log.info("Calculating CRC value of: %s", (const char*)substr);
#endif

	uint8_t crc = substr.charAt(0);
	for (size_t i = 1; i < substr.length(); i++) {
		crc = crc ^ substr.charAt(i);
	}
	return String::format("%02X", crc);
}