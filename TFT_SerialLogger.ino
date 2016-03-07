#include <Arduino.h>

#ifndef _DEBUG
#define _DEBUG 0
#endif

uint8_t TFT_ROW = 0;
uint32_t BAUD_IDX = 5;
// DEFAULT BAUD: 9600                        ****
uint32_t BAUD[] {300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200};

uint8_t m = 1, n = 0;
#define NUM_ROWS 20 // 20 = 240 pixels in height / 12 per row for SmallFont
#define NUM_COLUMNS 39 // 40 = 320 pixels in width / 8 (approx) per column for SmallFont
char TFT_RowText[] = "                                       ";
char TFT_RowNull[] = "                                       ";

// SD uses the following pin mapping
//[CS,MOSI,MISO,CLK] = [D10,D11,D12,D13]
#include <SPI.h>
#include <SD.h>

File dataFile;
uint16_t num_files = 0;
char LOG_FILENAME[13] = {"LOG_0000.TXT"};
bool isLogging = false, SD_INITIALISED = false;

// Touchscreen digitiser
// Note : There are not enough pins on the Nano to get the position (and it didnt work for me anyway)
//        So just process the interrupt IRQ on pin D8
#define TOUCH_IRQ_PIN 8 // Attached to the IRQ pin for your touchscreen digitiser

// TFT
// Example memorysaver.h
// NOTE: You need to modify memorysaver.h in the library folder:
// <sketchbook location>/libraries/UTFT/memorysaver.h
#include "memorysaver.h"
#include <UTFT.h>

// Declare which fonts we will be using
extern uint8_t SmallFont[];

#define SMALL_ROW 12

// DB0 to DB7 -> D0 to D7
// RD -> 3.3V (Still works if i disconnect this)
// LEDA -> 5V (Still works if i disconnect this)
// VCCIN -> 5V
#define RS A5  // 19 : Register Select -> A5
#define WR A4  // 18 : Write           -> A4
#define CS A3  // 17 : Chip Select     -> A3
#define RST A2 // 16 : Reset           -> A2

UTFT myGLCD(ILI9325D_8,RS,WR,CS,RST);

uint32_t tPressed = 0, tReleased = 0;
int32_t touch_time = 0;

#define SHORT_TOUCH 100
#define LONG_TOUCH 1000

// Serial input
#include <SoftwareSerialWithHalfDuplex.h>
SoftwareSerialWithHalfDuplex ss(9,9);

// Function declarations
void print_header();
void print_log_status(const bool standalone = true);
void print_baud(const bool standalone = true);
void logFilename(const uint16_t &_num, char (&_filename)[13]); // Function declaration
uint16_t get_log_count(File dir);

void setup()
{
	// Initialise the SD card
	if (SD.begin(SS))
	{
		SD_INITIALISED = true;
	}
	dataFile = SD.open("/");
	num_files = get_log_count(dataFile);
	dataFile.close();
	logFilename(++num_files, LOG_FILENAME); // Pre-generate a log filename
	
	// Display serial information on TFT
	myGLCD.InitLCD();
	myGLCD.clrScr();
	myGLCD.setFont(SmallFont);
	myGLCD.setBackColor(VGA_BLACK);

	// Print the header to the TFT
	print_header();
	
	// setup the touchscreen IRQ pin
	pinMode(TOUCH_IRQ_PIN, INPUT_PULLUP);
	
	// Start the Software Serial port (pin 8)
	ss.begin( BAUD[BAUD_IDX] );
}

void loop()
{
	// Process serial data
	while (ss.available())
	{
		char c = ss.read();
		
		// Log the (raw) serial data to SD
		if (isLogging)
		{
			dataFile.print(c);
		}
		
		if (c > 31 )// only process valid characters (not null spaces etc)
		{
			TFT_RowText[n++] = c;
		}
		
		// With the TFT we need to take consideration of the line length
		// Newline if we have reached the end of the screen OR the last character was a \n or \r
		if ((n == NUM_COLUMNS) | (c == 10))
		{
			if (m < NUM_ROWS)
			{
				myGLCD.print(TFT_RowNull,LEFT,(m+1)*SMALL_ROW); // Clear the next line
			}
			// Display the serial data on the TFT
			myGLCD.print(TFT_RowText,LEFT,m*SMALL_ROW);
			
			// clear the line data
			memset(&TFT_RowText[0], 0, sizeof(TFT_RowText));
			n = 0; m = m+1;
		}
		if (m == NUM_ROWS)
		{
			m = 1;
		}
	}

	// Process touchscreen interrupts which can spawn the following events:
	// 1. Log data
	// 2. Change SoftwareSerialWithHalfDuplex BAUD
	if (!touch_isPressed())
	{
		// calculate the touch time
		if (tReleased < tPressed)
		{
			tReleased = millis();
			touch_time = tReleased - tPressed;
		}
		
		if (touch_time > SHORT_TOUCH)
		{
			// Short single touch -> Increment the BAUD rate
			if (touch_time < LONG_TOUCH)
			{
				if(++BAUD_IDX == 12) { BAUD_IDX = 0; }
				ss.begin( BAUD[BAUD_IDX] );
				print_baud();
				
				// Increment the row
				if (++m > NUM_ROWS)
				{
					m = 1;
				}
			}
			
			// Clear variables
			tPressed = 0;
			tReleased = 0;
			touch_time = 0;
		}
		else
		{
			// Clear variables
			touch_time = 0;
			tReleased = 0;
		}
	}
	else
	{
		if (tReleased < tPressed)
		{// calculate the touch time
			touch_time = millis() - tPressed;
		}
		
		if (tPressed == 0)
		{
			tPressed = millis();
		}
		else if (touch_time >= LONG_TOUCH)
		{
			isLogging = !isLogging;
			
			print_log_status();
			
			if (isLogging)
			{
				dataFile = SD.open(LOG_FILENAME, FILE_WRITE);
			}
			else
			{
				dataFile.close();
				logFilename(++num_files, LOG_FILENAME);
			}
			
			// Increment the row
			if (++m > NUM_ROWS)
			{
				m = 1;
			}
			
			// Ensure multiple long touch events dont get triggered
			tReleased = millis();
			touch_time = 0;
		}
	}
}

void logFilename(const uint16_t &_num, char (&_filename)[13])
{
	uint16_t num_ = _num;
	const uint8_t th = num_ / 1000; num_ -= th*1000;
	const uint8_t hu = num_ / 100;  num_ -= hu*100;
	const uint8_t te = num_ / 10;  num_ -= te*10;

	_filename[0] = 'L';
	_filename[1] = 'O';
	_filename[2] = 'G';
	_filename[3] = '_';
	_filename[4] = '0'+th;
	_filename[5] = '0'+hu;
	_filename[6] = '0'+te;
	_filename[7] = '0'+num_;
	_filename[8] = '.';
	_filename[9] = 'T';
	_filename[10] = 'X';
	_filename[11] = 'T';
	_filename[12] = '\0';
}

uint16_t get_log_count(File dir)
{
	uint16_t count = 0, t_count;
	#if (_DEBUG == 1)
	Serial.println("+--------------");
	Serial.println("| SD files");
	Serial.println("+--------------");
	#endif
	while(true)
	{
		File entry =  dir.openNextFile();
		if (! entry)
		{
			// no more files
			#if (_DEBUG == 1)
			Serial.println("+--------------");
			Serial.print("| ");
			Serial.print(count); Serial.println(" log files");
			Serial.println("+--------------");
			#endif
			return count;
		}
		
		// Check if its a log file
		char * filename = entry.name();
		
		#if (_DEBUG == 1)
		Serial.print("| ");
		#endif
		if (strncmp(filename, "LOG_", 4) == 0)
		{
			t_count = filename[4] - '0'; // thousands
			t_count =10*t_count + (filename[5] - '0'); // hundreds
			t_count =10*t_count + (filename[6] - '0'); // tens
			t_count =10*t_count + (filename[7] - '0'); // units
			
			count = max(count, t_count);
			#if (_DEBUG == 1)
			Serial.print("* ");
			#endif
		}
		else
		{
			#if (_DEBUG == 1)
			Serial.print("  ");
			#endif
		}
		#if (_DEBUG == 1)
		Serial.println(entry.name());
		#endif
		entry.close();
	}
}


inline bool touch_isPressed()
{
	return (digitalRead(TOUCH_IRQ_PIN) == LOW);
}

void print_header()
{
	myGLCD.setColor(VGA_RED);
	print_log_status(false);
	myGLCD.setColor(VGA_FUCHSIA);
	myGLCD.print(" | TTL Display |", CENTER, 0);
	print_baud(false);
	myGLCD.setColor(VGA_YELLOW);
	
	m = 1;
}

void print_baud(const bool standalone)
{
	if (standalone)
	{
		m = 1;
		myGLCD.setColor(VGA_FUCHSIA);
	}
	
	String _BAUD = String( BAUD[BAUD_IDX] );
	_BAUD += " BAUD";
	myGLCD.print("    00 BAUD", RIGHT, 0); // clear the text
	myGLCD.print(_BAUD, RIGHT, 0); // display the BAUD
	
	if (standalone)
	{
		myGLCD.setColor(VGA_YELLOW);
	}
}

void print_log_status(const bool standalone)
{
	if (standalone)
	{
		m = 1;
		if (isLogging)
		{
			myGLCD.setColor(VGA_LIME);
		}
		else
		{
			myGLCD.setColor(VGA_RED);
		}
	}
	
	myGLCD.print("LOG_0000.TXT", LEFT, 0); // clear the text
	myGLCD.print(LOG_FILENAME, LEFT, 0);
	
	if (standalone)
	{
		myGLCD.setColor(VGA_YELLOW);
	}
}