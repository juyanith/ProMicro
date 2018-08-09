/*
 Name:		OLED_Test.ino
 Created:	8/8/2018 6:56:35 AM
 Author:	stonecypherv
*/
#include <tiny_buffer.h>
#include <ssd1331_api.h>
#include <ssd1306_fonts.h>
#include <ssd1306_console.h>
#include <sprite_pool.h>
#include <nano_gfx_types.h>
#include <nano_gfx.h>
#include <nano_engine.h>
#include <font6x8.h>

#include <SparkFunMPL3115A2.h>
MPL3115A2 sensor;

const PROGMEM uint8_t dotImage[] =
{
	0b00000110,
	0b00001111,
	0b00001111,
	0b00000110,
};
const int dotHeight = 8;
const int dotWidth = 4;

const PROGMEM uint8_t degreeImage[] =
{
	0b00011100,
	0b00110110,
	0b01100011,
	0b00110110,
	0b00011100,
};
const int degreeHeight = 8;
const int degreeWidth = 5;

void displayTemp(float tempC)
{
	static char buf[4];
	static int int_val, frac_val;

	//dtostrf(tempC, 5, 2, buf);
	//ssd1306_printFixed(0, 0, buf, STYLE_NORMAL);

	int i = (int)tempC;
	if (i != int_val)
	{
		snprintf(buf, sizeof(buf), "%2d", i);
		ssd1306_printFixed(0, 0, buf, STYLE_NORMAL);

		int_val = i;
	}

	i = (tempC - i) * 100;
	if (i != frac_val)
	{
		snprintf(buf, sizeof(buf), "%02d", i);
		ssd1306_printFixed(54, 0, buf, STYLE_NORMAL);

		frac_val = i;
	}

	ssd1306_drawBitmap(48, 3, dotWidth, dotHeight, dotImage);
	ssd1306_drawBitmap(102, 0, degreeWidth, degreeHeight, degreeImage);
}

void setup() {
	Serial.begin(9600);
	while (!Serial);

	ssd1306_128x32_i2c_init();
	ssd1306_fillScreen(0x00);

	ssd1306_setFixedFont(comic_sans_font24x32_123);

	// Configure the sensor
	//sensor.setModeAltimeter();	// Measure altitude above sea level in meters
	sensor.setModeBarometer();		// Measure pressure in Pascals from 20 to 110 kPa

	sensor.setOversampleRate(7);	// Set Oversample to the recommended 128
	sensor.enableEventFlags();		// Enable all three pressure and temp event flags
}

void loop() {
	Serial.println("Reading temperature...");

	float tempC = sensor.readTemp();
	Serial.print(tempC);
	Serial.println(" C");

	displayTemp(tempC);

	delay(1000);
}
