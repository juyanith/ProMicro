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

//Enables the pressure and temp measurement event flags so that we can
//test against them. This is recommended in datasheet during setup.
void enableEventFlags()
{
	IIC_Write(PT_DATA_CFG, 0x07); // Enable all three pressure and temp event flags 
}

//Clears then sets the OST bit which causes the sensor to immediately take another reading
//Needed to sample faster than 1Hz
void toggleOneShot(void)
{
	byte tempSetting = IIC_Read(CTRL_REG1); //Read current settings
	tempSetting &= ~(1 << 1); //Clear OST bit
	IIC_Write(CTRL_REG1, tempSetting);

	tempSetting = IIC_Read(CTRL_REG1); //Read current settings to be safe
	tempSetting |= (1 << 1); //Set OST bit
	IIC_Write(CTRL_REG1, tempSetting);
}

// These are the two I2C functions in this sketch.
byte IIC_Read(byte regAddr)
{
	// This function reads one byte over IIC
	Wire.beginTransmission(MPL3115A2_ADDRESS);
	Wire.write(regAddr);  // Address of CTRL_REG1
	Wire.endTransmission(false); // Send data to I2C dev with option for a repeated start. THIS IS NECESSARY and not supported before Arduino V1.0.1!
	Wire.requestFrom(MPL3115A2_ADDRESS, 1); // Request the data...
	return Wire.read();
}

void IIC_Write(byte regAddr, byte value)
{
	// This function writes one byto over IIC
	Wire.beginTransmission(MPL3115A2_ADDRESS);
	Wire.write(regAddr);
	Wire.write(value);
	Wire.endTransmission(true);
}

float readTemp()
{
	toggleOneShot(); //Toggle the OST bit causing the sensor to immediately take another reading

					 //Wait for TDR bit, indicates we have new temp data
	int counter = 0;
	while ((IIC_Read(STATUS) & (1 << 1)) == 0)
	{
		if (++counter > 100) return(-999); //Error out
		delay(1);
	}

	// Read temperature registers
	Wire.beginTransmission(MPL3115A2_ADDRESS);
	Wire.write(OUT_T_MSB);  // Address of data to get
	Wire.endTransmission(false); // Send data to I2C dev with option for a repeated start. THIS IS NECESSARY and not supported before Arduino V1.0.1!
	Wire.requestFrom(MPL3115A2_ADDRESS, 2); // Request two bytes

											//Wait for data to become available
	counter = 0;
	while (Wire.available() < 2)
	{
		if (++counter > 100) return(-999); //Error out
		delay(1);
	}

	byte msb, lsb;
	msb = Wire.read();
	lsb = Wire.read();

	// The least significant bytes l_altitude and l_temp are 4-bit,
	// fractional values, so you must cast the calulation in (float),
	// shift the value over 4 spots to the right and divide by 16 (since 
	// there are 16 values in 4-bits). 
	float templsb = (lsb >> 4) / 16.0; //temp, fraction of a degree

	float temperature = (float)(msb + templsb);

	return(temperature);
}

//Sets the mode to Altimeter
//CTRL_REG1, ALT bit
void setModeAltimeter()
{
	byte tempSetting = IIC_Read(CTRL_REG1); //Read current settings
	tempSetting |= (1 << 7); //Set ALT bit
	IIC_Write(CTRL_REG1, tempSetting);
}

//Sets the mode to Barometer
//CTRL_REG1, ALT bit
void setModeBarometer()
{
	byte tempSetting = IIC_Read(CTRL_REG1); //Read current settings
	tempSetting &= ~(1 << 7); //Clear ALT bit
	IIC_Write(CTRL_REG1, tempSetting);
}

//Call with a rate from 0 to 7. See page 33 for table of ratios.
//Sets the over sample rate. Datasheet calls for 128 but you can set it 
//from 1 to 128 samples. The higher the oversample rate the greater
//the time between data samples.
void setOversampleRate(byte sampleRate)
{
	if (sampleRate > 7) sampleRate = 7; //OS cannot be larger than 0b.0111
	sampleRate <<= 3; //Align it for the CTRL_REG1 register

	byte tempSetting = IIC_Read(CTRL_REG1); //Read current settings
	tempSetting &= 0b11000111; //Clear out old OS bits
	tempSetting |= sampleRate; //Mask in new OS bits
	IIC_Write(CTRL_REG1, tempSetting);
}

void displayTemp(float tempC)
{
	static char int_buf[4], frac_buf[3];
	static int int_val, frac_val;

	//dtostrf(tempC, 5, 2, buf);
	//ssd1306_printFixed(0, 0, buf, STYLE_NORMAL);

	int i = (int)tempC;
	int f = (tempC - i) * 100;

	// if the temperature has changed
	if (i != int_val || f != frac_val)
	{
		snprintf(int_buf, sizeof(int_buf), "%2d", i);
		snprintf(frac_buf, sizeof(frac_buf), "%02d", f);

		//ssd1306_clearScreen();
		ssd1306_printFixed(0, 0, int_buf, STYLE_NORMAL);
		ssd1306_drawBitmap(48, 3, dotWidth, dotHeight, dotImage);
		ssd1306_printFixed(54, 0, frac_buf, STYLE_NORMAL);
		ssd1306_drawBitmap(102, 0, degreeWidth, degreeHeight, degreeImage);
	}
}

void setup() {
	Serial.begin(9600);
	while (!Serial);

	ssd1306_128x32_i2c_init();
	ssd1306_fillScreen(0x00);

	ssd1306_setFixedFont(comic_sans_font24x32_123);

	if (IIC_Read(WHO_AM_I) == 196)
		Serial.println("MPL3115A2 online!");
	else
		Serial.println("No response - check connections");

	// Configure the sensor
	//setModeAltimeter();	// Measure altitude above sea level in meters
	setModeBarometer();		// Measure pressure in Pascals from 20 to 110 kPa

	setOversampleRate(7);	// Set Oversample to the recommended 128
	enableEventFlags();		// Enable all three pressure and temp event flags
}

void loop() {
	Serial.println("Reading temperature...");

	float tempC = readTemp();
	Serial.print(tempC);
	Serial.println(" C");

	displayTemp(tempC);

	delay(1000);
}
