/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
 * 				2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
 *
 *  license:    This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU Library General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later version.
 *
 *              This program is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *              GNU General Public License for more details.
 *
 *              You should have received a copy of the GNU General Public License
 *              along with this program. If not, see http:*www.gnu.org/licenses
 */

/*!
 * \file       lcd.c
 * \brief      functions to control the HR20 LCD
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>, Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

// AVR LibC includes
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>

// HR20 Project includes
#include "main.h"
#include "lcd.h"
#include "task.h"
#include "rtc.h"
#include "eeprom.h"

// Local Defines
#define LCD_CONTRAST_MIN       0   //!< \brief contrast minimum
#define LCD_CONTRAST_MAX      15   //!< \brief contrast maxmum
#define LCD_MAX_POS            4   //!< \brief number of 7 segment chars
#define LCD_MAX_CHARS  (sizeof(LCD_CharTablePrgMem))   //!< \brief no. of chars in \ref LCD_CharTablePrgMem

#define LCD_REGISTER_COUNT     16   //!< \brief no. of registers each bitplane


// Vars
volatile uint8_t LCD_used_bitplanes = 1; //!< \brief number of used bitplanes / used for power save

//! segment data for the segment registers in each bitplane
volatile uint8_t LCD_Data[LCD_BITPLANES][LCD_REGISTER_COUNT];

#ifdef LCD_UPSIDE_DOWN
  #define LCD_upside_down 1
#else
  #define LCD_upside_down 0
#endif


// Look-up table to convert value to LCD display data (segment control)
// Get value with: bitmap = pgm_read_word(&LCD_CharTablePrgMem[position]);
const uint16_t LCD_CharTablePrgMem[] PROGMEM =
{
		//	mlkjihggfedcba
		0b0010010000111111,		//0	(offset 0, ASCII 48)
		0b0000010000000110,		//1
		0b0000000011011011,		//2
		0b0000000010001111,		//3
		0b0000000011100110,		//4
		0b0000000011101101,		//5
		0b0000000011111101,		//6
		0b0001010000000001,		//7
		0b0000000011111111,		//8
		0b0000000011101111,		//9
		0b0000000011000000,		//:	(10)
		0b0010001000000000,		//;
		0b0000110000000000,		//<
		0b0000000011001000,		//=
		0b0010000100000000,		//>
		0b0000000000000000,		//?
		0b0000000011100011,		//@
		0b0000000011110111,		//A
		0b0001001010001111,		//B
		0b0000000000111001,		//C
		0b0001001000001111,		//D	(20)
		0b0000000001111001,		//E
		0b0000000001110001,		//F
		0b0000000010111101,		//G
		0b0000000011110110,		//H
		0b0001001000001001,		//I
		0b0000000000011110,		//J
		0b0000110001110000,		//K
		0b0000000000111000,		//L
		0b0000010100110110,		//M
		0b0000100100110110,		//N	(30)
		0b0000000000111111,		//O
		0b0000000011110011,		//P
		0b0000100000111111,		//Q
		0b0000100011110011,		//R
		0b0000000011101101,		//S
		0b0001001000000001,		//T
		0b0000000000111110,		//U
		0b0010010000110000,		//V
		0b0010100000110110,		//W
		0b0010110100000000,		//X	(40)
		0b0001010100000000,		//Y
		0b0010010000001001,		//Z
		0b0000000011100011,		// degree symbol (43)
		0b0000000011000000,		// negative sign
		0b0000000000000000,		//" "			 (45)
		0b0001001000100100		// % (46)
};


// Look-up table to adress a segment inside a field (could be negative number)
const int8_t LCD_SegOffsetTablePrgMem[] [14] PROGMEM =
{
	{								// for field 0
		LCD_SEG_1A,		// Seg A			 AAAAAAAAAAAAA
		LCD_SEG_1B,		// Seg B			FH     I     JB
		LCD_SEG_1C,		// Seg C			F H    I    J B
		LCD_SEG_1D,		// Seg D			F  H   I   J  B
		LCD_SEG_1E,		// Seg E			F	H  I  J   B
		LCD_SEG_1F,		// Seg F			F	  HIJ     B
		LCD_SEG_1G1,	// Seg G1			 G1G1G1 G2G2G2
		LCD_SEG_1G2,	// Seg G2			E     MLK     C
		LCD_SEG_1H,		// Seg H			E    M L K    C
		LCD_SEG_1I,		// Seg I			E   M  L  K   C
		LCD_SEG_1J,		// Seg J			E  M   L   K  C
		LCD_SEG_1K,		// Seg M			E M    L    K C
		LCD_SEG_1L,		// Seg L			 DDDDDDDDDDDDD
		LCD_SEG_1M		// Seg K			 
	},

	{							// for field 1
		LCD_SEG_2A,
		LCD_SEG_2B,
		LCD_SEG_2C,
		LCD_SEG_2D,
		LCD_SEG_2E,
		LCD_SEG_2F,
		LCD_SEG_2G1,
		LCD_SEG_2G2,
		LCD_SEG_2H,
		LCD_SEG_2I,
		LCD_SEG_2J,
		LCD_SEG_2K,
		LCD_SEG_2L,
		LCD_SEG_2M
	},
	
	{							// for field 2
		LCD_SEG_3A,
		LCD_SEG_3B,
		LCD_SEG_3C,
		LCD_SEG_3D,
		LCD_SEG_3E,
		LCD_SEG_3F,
		LCD_SEG_3G1,
		LCD_SEG_3G2,
		LCD_SEG_3H,
		LCD_SEG_3I,
		LCD_SEG_3J,
		LCD_SEG_3K,
		LCD_SEG_3L,
		LCD_SEG_3M
	},
	
	{							// for field 3
		LCD_SEG_4A,
		LCD_SEG_4B,
		LCD_SEG_4C,
		LCD_SEG_4D,
		LCD_SEG_4E,
		LCD_SEG_4F,
		LCD_SEG_4G1,
		LCD_SEG_4G2,
		LCD_SEG_4H,
		LCD_SEG_4I,
		LCD_SEG_4J,
		LCD_SEG_4K,
		LCD_SEG_4L,
		LCD_SEG_4M
	},
};

//! Look-up table for adress hour-bar segments
const uint8_t LCD_SegHourBarOffsetTablePrgMem[] PROGMEM =
{
	LCD_SEG_B0,    LCD_SEG_B1,    LCD_SEG_B2,    LCD_SEG_B3,    LCD_SEG_B4,
	LCD_SEG_B5,    LCD_SEG_B6,    LCD_SEG_B7,    LCD_SEG_B8,    LCD_SEG_B9,
	LCD_SEG_B10,   LCD_SEG_B11,   LCD_SEG_B12,   LCD_SEG_B13,   LCD_SEG_B14,
	LCD_SEG_B15,   LCD_SEG_B16,   LCD_SEG_B17,   LCD_SEG_B18,   LCD_SEG_B19,
	LCD_SEG_B20,   LCD_SEG_B21,   LCD_SEG_B22,   LCD_SEG_B23
};

const uint8_t LCD_SegDayOfWeek[] PROGMEM = { LCD_SEG_MON, LCD_SEG_TUE, LCD_SEG_WED, LCD_SEG_THU, LCD_SEG_FRI, LCD_SEG_SAT, LCD_SEG_SUN };

const uint8_t LCD_DOWStringTable[][4] PROGMEM =
{
	"MON ", "TUE ", "WED ", "THU ", "FRI ", "SAT ", "SUN "
};


static void LCD_calc_used_bitplanes(uint8_t mode);

/*!
 *******************************************************************************
 *  Init LCD
 *
 *  \note
 *  - Initialize LCD Global Vars
 *  - Set up the LCD (timing, contrast, etc.)
 ******************************************************************************/
void LCD_Init(void)
{
	// Clear segment buffer.
	LCD_AllSegments(LCD_MODE_OFF);

	//Set the initial LCD contrast level
	LCDCCR = (config.lcd_contrast << LCDCC0);

	
	LCDCCR |= (1<<LCDDC2) | (1<<LCDDC1) | (1<<LCDDC0); //Display configuration (LCDDC=7): drivers 50% on

	// LCD Control and Status Register B
	//   - clock source is TOSC1 pin (LCDCS=1)
	//   - COM0:3 connected (LCDMUX=3)
	//   - SEG0:24 connected (LCDPM=7)
	LCDCRB = (1<<LCDCS) | (3<<LCDMUX0) | (7<<LCDPM0);
	
	// LCD Frame Rate Register
	//   - LCD Prescaler Select (LCDPS=16)		@32.768Hz	->	2048Hz
	//   - LCD Duty Cycle 1/4 (K=8)				2048Hz / 8	->	256Hz
	//   - LCD Clock Divider (LCDCD=4)			256Hz / 4	->	64Hz
	LCDFRR = (0<<LCDPS0) | (3<<LCDCD0);
 
	// LCD Control and Status Register A
	//  - Enable LCD
	//  - Set Low Power Waveform
	LCDCRA = (1<<LCDEN) | (1<<LCDAB);

	// Enable LCD start of frame interrupt
	LCDCRA |= (1<<LCDIE);

	LCD_used_bitplanes=1;
}


/*!
 *******************************************************************************
 *  Switch LCD on/off
 *
 *  \param mode
 *       -      0: clears all digits
 *       -  other: set all digits
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF
 ******************************************************************************/
void LCD_AllSegments(uint8_t mode)
{
	uint8_t i;
	uint8_t val = (mode==LCD_MODE_ON)?0xff:0x00;

	for (i=0; i<LCD_REGISTER_COUNT*LCD_BITPLANES; i++)
	{
		((uint8_t *)LCD_Data)[i] = val;
	}
	LCD_used_bitplanes = 1;
	LCD_Update();
}

/*!
 *******************************************************************************
 *  Print char in LCD field
 *
 *  \note  segments inside one 7 segment array are adressed using address of
 *         segment "F" \ref LCD_FieldOffsetTablePrgMem[] as base address adding
 *         \ref LCD_SegOffsetTablePrgMem[] *
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param value char to print, see \ref LCD_CharTablePrgMem[]
 *  \param pos   position in lcd 0=right to 3=left <tt> 32 : 10 </tt>
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 *  \param value
 *        - 0: clears all digits,
 *        - other: set all digits
 ******************************************************************************/
void LCD_PrintChar(uint8_t value, uint8_t pos, uint8_t mode)
{
	uint16_t bitmap;
	uint8_t seg;
	uint8_t i;
	uint16_t mask;

	value -= 48;		// Start of ASCII table
	// Boundary Check 
	if ((pos < LCD_MAX_POS) && (value < LCD_MAX_CHARS))
	{
		// Get Fieldbase for Position
		if (LCD_upside_down)
			pos = 3-pos;
		// Get Bitmap for Value
		bitmap = pgm_read_word(&LCD_CharTablePrgMem[value]);
		mask = 1;

		// Set 7 Bits
		for (i=0; i<14; i++)
		{
			seg = pgm_read_byte(&LCD_SegOffsetTablePrgMem[pos][i]);
			// Set or Clear?
			LCD_SetSeg(seg,((bitmap & mask)?mode:LCD_MODE_OFF));
			mask <<= 1;
		}
	}
}


void LCD_PrintDayOfWeek(uint8_t dow, uint8_t mode)
{
	uint8_t i;
	uint8_t tmp;
	// Put 4 chars
	for (i=0; i<4; i++)
	{
		tmp = pgm_read_byte(&LCD_DOWStringTable[dow][i]);
		LCD_PrintChar(tmp, 3-i, mode);
	}
}


/*!
 *******************************************************************************
 *  Print Hex value in LCD field
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param value value to be printed (0-0xff)
 *  \param pos   position in lcd 0:left, 1:right
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintHex(uint8_t value, uint8_t pos, uint8_t mode)
{
	uint8_t tmp;
	// Boundary Check
	if (pos <= 2)
	{
		// 1st Digit at 0 (2)
		tmp = (value / 16) + 48;
		LCD_PrintChar(tmp, pos, mode);
		pos++;
		// 2nd Digit at 1 (3)
		tmp = (value % 16) + 48;
		LCD_PrintChar(tmp, pos, mode);
	}
}


/*!
 *******************************************************************************
 *  Print decimal value in LCD field (only 2 digits)
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *        it is triggered automatically at change of bitframe
 *
 *  \param value value to be printed (0-99)
 *  \param pos   position in lcd 
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintDec(uint8_t value, uint8_t pos, uint8_t mode)
{
	uint8_t tmp;
	// Boundary Check
	if ((pos <= 2) && (value < 100))
	{

		// 1st Digit at 0 (2)
		tmp = (value / 10) + 48;
		if (pos==2)
			LCD_PrintChar(tmp, pos, mode);
		else
			if	(value>9)
				LCD_PrintChar(tmp, pos, mode);
		pos++;
		// 2nd Digit at 1 (3)
		tmp = (value % 10) + 48;
		LCD_PrintChar(tmp, pos, mode);
	}
}
/*!
 *******************************************************************************
 *  Print decimal value in LCD field (3 digits)
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *        it is triggered automatically at change of bitframe
 *
 *  \param value value to be printed (0-999)
 *  \param pos   position in lcd
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintDec3(uint16_t value, uint8_t pos, uint8_t mode)
{
	if (value>999) value=999;
	if (pos <= 1)
	{
		// 3nd Digit 
		LCD_PrintChar((value / 100) + 48, pos, mode);
		LCD_PrintDec(value%100, pos+1, mode);
	}
}


/*!
 *******************************************************************************
 *  Print decimal uint16 value in LCD field
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *        it is triggered automatically at change of bitframe
 *
 *  \param value value to be printed (0-9999)
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintDecW(uint16_t value, uint8_t mode)
{
	uint8_t tmp;
	// Boundary Check
	if (value > 9999)
		value = 9999;        
	// Print     
	tmp = (uint8_t) (value / 100);
	LCD_PrintDec(tmp, 2, mode);
	tmp = (uint8_t) (value % 100);
	LCD_PrintDec(tmp, 0, mode);
}


/*!
 *******************************************************************************
 *  Print hex uint16 value in LCD field
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *        it is triggered automatically at change of bitframe
 *
 *  \param value value to be printed (0-0xffff)
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintHexW(uint16_t value, uint8_t mode)
{
	uint8_t tmp;
	// Print     
	tmp = (uint8_t) (value >> 8);
	LCD_PrintHex(tmp, 0, mode);
	tmp = (uint8_t) (value & 0xff);
	LCD_PrintHex(tmp, 2, mode);
}


/*!
 *******************************************************************************
 *  Print BYTE as temperature on LCD (desired temperature)
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \note  range for desired temperature 5,0°C - 30°C, OFF and ON 
 *
 *  \param temp<BR>
 *     - TEMP_MIN-1          : \c OFF <BR>
 *     - TEMP_MIN to TEMP_MAX : temperature = temp/2  [5,0°C - 30°C]
 *     - TEMP_MAX+1          : \c ON  <BR>
 *     -    other: \c invalid <BR>
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintTemp(uint8_t temp, uint8_t mode)
{
	if (temp==TEMP_MIN-1)
	{
		// OFF
		LCD_PrintString(PSTR("OFF "),mode); 
	}
	else
	if (temp==TEMP_MAX+1)
	{
		// On
		LCD_PrintString(PSTR("ON "),mode); 
	}
	else
	if (temp>TEMP_MAX+1)
	{
		// Error -E rr
		LCD_PrintString(PSTR("ERR "),mode); 
	}
	else
	{
	#define START_POS 0
	LCD_PrintChar(LCD_CHAR_DEGREE, 3, mode);
	LCD_SetSeg(LCD_SEG_DOT, mode);    // decimal point
	LCD_PrintDec((temp>>1), START_POS, mode);
	LCD_PrintChar(((temp&1)?5:0)+48, START_POS + 2, mode);
//	if (temp < (100/5))
	//	LCD_PrintChar(LCD_CHAR_NULL, START_POS + 2, mode);
}
}


/*!
 *******************************************************************************
 *  Print INT as temperature on LCD (measured temperature)
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *
 *  \param temp temperature in 1/100 deg C<BR>
 *     min:  -999 => -9,9°C
 *     max:  9999 => 99,9°C
  *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintTempInt(int16_t temp, uint8_t mode)
{
	bool neg;

	// check min / max
	if (temp < -999)
		temp = -999;
 
	// negative ?    
	neg = (temp < 0); 
	if (neg)
		temp = -temp;    

	#define START_POS 0
	LCD_PrintChar(LCD_CHAR_DEGREE, 3, mode);
	LCD_SetSeg(LCD_SEG_DOT, mode);    // decimal point


	// 1/100°C not printed
	LCD_PrintDec3(temp/10, START_POS, mode);
	
	if (neg)
	{
		// negative Temp      
		LCD_PrintChar(LCD_CHAR_neg, START_POS, mode);
	}
	else
	if (temp < 1000)
	{
		// Temp < 10°C
		LCD_PrintChar(LCD_CHAR_NULL, START_POS, mode);
	}                             
}

/*!
 *******************************************************************************
 *  Print LCD string from table
 *
 *  \note  something weird due to 7 Segments
 *
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintString(char *string, uint8_t mode)
{
	uint8_t i;
	uint8_t tmp;
	// Put 4 chars
	for (i=0; i<4; i++)
	{
		tmp = pgm_read_byte(&string[i]);
		LCD_PrintChar(tmp, i, mode);
	}
}


void LCD_SetDOW(uint8_t dow, uint8_t mode)
{
	uint8_t segment;

	if (dow > 6)
		dow = 6;
	// Get segment number for dow
	segment = pgm_read_byte(&LCD_SegDayOfWeek[dow]);
	// Set segment 
	LCD_SetSeg(segment, mode);
}



void LCD_SetDOWBar(uint8_t val, uint8_t mode)
{
	uint8_t i;
	// Only Segments 0:6
	if (val > 6)
		val = 6;
	// For each Segment 0:6
	for (i=0; i<7; i++)
	{
		if (i > val)
			LCD_SetDOW(i, LCD_MODE_OFF);
		else
			LCD_SetDOW(i, mode);
	}

}


/*!
 *******************************************************************************
 *  Set segment of the hour-bar
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param seg No of the hour bar segment 0-23
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_SetHourBarSeg(uint8_t seg, uint8_t mode)
{
	uint8_t segment;
	// Get segment number for this element
	segment = pgm_read_byte(&LCD_SegHourBarOffsetTablePrgMem[seg]);
	// Set segment 
	LCD_SetSeg(segment, mode);
}

/*!
 *******************************************************************************
 *  Set all segments of the hour-bar (ON/OFF) like bitmap
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param bitmap of hour bar segment 0-23 (bit0 is segment0 etc.)
 *  \note blink is not supported
 ******************************************************************************/
void LCD_HourBarBitmap(uint32_t bitmap)
{
	#if 0
	uint8_t i;
	for (i=0;i<24;i++)
	{
		uint8_t segment = pgm_read_byte(&LCD_SegHourBarOffsetTablePrgMem[i]);
		LCD_SetSeg(segment, (((uint8_t)bitmap & 1)? LCD_MODE_ON : LCD_MODE_OFF ));
		bitmap = bitmap>>1;
	}
	#else
	asm volatile (
	"    movw r14,r22                                     " "\n"
	"    mov  r16,r24                                     " "\n"
	"    ldi r28,lo8(LCD_SegHourBarOffsetTablePrgMem)     " "\n"       
	"    ldi r29,hi8(LCD_SegHourBarOffsetTablePrgMem)     " "\n"
	"L2:                                                  " "\n"
	"    movw r30,r28                                     " "\n"
	"	 lpm r24, Z                                       " "\n"
	"	 clr r22                                          " "\n"
	"	 lsr r16                                          " "\n"
	"	 ror r15                                          " "\n"
	"	 ror r14                                          " "\n"
	"    brcc L3                                          " "\n"
	"    ldi r22,lo8(%0)                                  " "\n"
	"L3:                                                  " "\n"
	"	call LCD_SetSeg                                   " "\n"
	"	adiw r28,1                                        " "\n"
	"	cpi r28,lo8(LCD_SegHourBarOffsetTablePrgMem+24)   " "\n"
	"	brne L2                                           " "\n"
	:
	: "I" (LCD_MODE_ON)
	: "r14", "r15", "r16", "r28", "r29", "r30", "r31"
	);
	#endif
}


/*!
 *******************************************************************************
 *  Set all segments from left up to val and clear all other segments
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param seg No of the last hour bar segment to be set 0-23
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
#if 0
void LCD_SetHourBarBar(uint8_t val, uint8_t mode)
{
	uint8_t i;
	// Only Segment 0:23
	if (val > 23)
		val = 23;
	// For each Segment 0:23
	for (i=0; i<24; i++)
	{
		if (i > val)
			LCD_SetHourBarSeg(i, LCD_MODE_OFF);
		else
			LCD_SetHourBarSeg(i, mode);
	}
}
#endif


/*!
 *******************************************************************************
 *  Set only one segment and clear all others
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param seg No of the hour bar segment to be set 0-23
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
#if 0
void LCD_SetHourBarVal(uint8_t val, uint8_t mode)
{
	uint8_t i;
	// Only Segment 0:23
	if (val > 23)
		val = 23;
	// For each Segment 0:23
	for (i=0; i<24; i++)
	{
		if (i == val)
			LCD_SetHourBarSeg(i, mode);
		else
			LCD_SetHourBarSeg(i, LCD_MODE_OFF);
	}
}
#endif


/*!
 *******************************************************************************
 *  Clear LCD Display
 *
 *  \note Sets all Segments of the to \ref LCD_MODE_OFF
 ******************************************************************************/
#if 0
void LCD_ClearAll(void)
{
	LCD_AllSegments(LCD_MODE_OFF);
}
#endif

/*!
 *******************************************************************************
 *  Clear hour-bar 
 *
 *  \note Sets all Hour-Bar Segments to \ref LCD_MODE_OFF
 ******************************************************************************/
#if 0
void LCD_ClearHourBar(void)
{
	LCD_SetHourBarVal(23, LCD_MODE_OFF);
	LCD_Update();
}
#endif

/*!
 *******************************************************************************
 *  Clear all Symbols 
 *
 *  \note  Sets Symbols <tt> AUTO MANU PROG SUN MOON SNOW</tt>
 *         to \ref LCD_MODE_OFF
 ******************************************************************************/
#if 0
void LCD_ClearSymbols(void)
{
	LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_OFF);
	LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_OFF);
	LCD_SetSeg(LCD_SEG_OUTSIDE, LCD_MODE_OFF);
	LCD_SetSeg(LCD_SEG_INSIDE, LCD_MODE_OFF);
	LCD_SetSeg(LCD_SEG_MOON, LCD_MODE_OFF);
	LCD_SetSeg(LCD_SEG_SNOW, LCD_MODE_OFF);

	LCD_Update();
}
#endif

/*!
 *******************************************************************************
 *  Clear all 7 segment fields
 *
 *  \note  Sets the four 7 Segment and the Columns to \ref LCD_MODE_OFF
 ******************************************************************************/
#if 0
void LCD_ClearNumbers(void)
{
	LCD_PrintChar(LCD_CHAR_NULL, 3, LCD_MODE_OFF);
	LCD_PrintChar(LCD_CHAR_NULL, 2, LCD_MODE_OFF);
	LCD_PrintChar(LCD_CHAR_NULL, 1, LCD_MODE_OFF);
	LCD_PrintChar(LCD_CHAR_NULL, 0, LCD_MODE_OFF);
	LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_OFF);
	LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_OFF);

	LCD_Update();
}
#endif

/*!
 *******************************************************************************
 *  Set segment of LCD
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param seg No of the segment to be set see \ref LCD_SEG_B0 ...
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_SetSeg(uint8_t seg, uint8_t mode)
{
	LCD_SetSegReg(seg / 8, 1<<(seg % 8), mode);
}

/*!
 *******************************************************************************
 *  Set segment of LCD
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param Register = segment DIV 8 to be set see \ref LCD_SEG_B0 ...
 *  \param Bitposition = segment mod 8
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_SetSegReg(uint8_t r, uint8_t b, uint8_t mode)
{
	// Set bits in each bitplane
	if (mode & 1)
	{
		// Set Bit in Bitplane if ON (0b11) or Blinkmode 1 (0b01)
		LCD_Data[0][r] |= b;
	}
	else
	{
		// Clear Bit in Bitplane if OFF (0b00) or Blinkmode 2 (0b10)
		LCD_Data[0][r] &= ~b;
	} 
	
	if (mode & 2)
	{
		// Set Bit in Bitplane if ON (0b11) or Blinkmode 2 (0b10)
		LCD_Data[1][r] |= b;
	}
	else
	{
		// Clear Bit in Bitplane if OFF (0b00) or Blinkmode 1 (0b01)
		LCD_Data[1][r] &= ~b;
	} 
	
	LCD_calc_used_bitplanes(mode);
}

/*!
 *******************************************************************************
 *  Calculate used bitplanes
 *
 *	\note used only for update LCD, in any other cases intterupt is disabled
 *  \note copy LCD_Data to LCDREG
 *
 ******************************************************************************/
static void LCD_calc_used_bitplanes(uint8_t mode)
{
	uint8_t i;
	if ((mode==LCD_MODE_BLINK_1) || (mode==LCD_MODE_BLINK_2))
	{
		LCD_used_bitplanes = 2;
		return; // just optimalization
	} 
	
	// mode must be LCD_MODE_ON or LCD_MODE_OFF
	if (LCD_used_bitplanes == 1)
		return; // just optimalization, nothing to do

	for (i=0; i<LCD_REGISTER_COUNT; i++)
	{
		if (LCD_Data[0][i] != LCD_Data[1][i])
		{
			LCD_used_bitplanes = 2;
			return; // it is done
		}
	}
	
	LCD_used_bitplanes = 1;
}


/*!
 *******************************************************************************
 *
 *	LCD_BlinkCounter and LCD_Bitplane for LCD blink
 *
 ******************************************************************************/
static uint8_t LCD_BlinkCounter;   //!< \brief counter for bitplane change
static uint8_t LCD_Bitplane;       //!< \brief currently active bitplane
uint8_t LCD_force_update=0;        //!< \brief force update LCD


/*!
 *******************************************************************************
 *  LCD Interrupt Routine
 *
 *	\note used only for update LCD, in any other cases intterupt is disabled
 *  \note copy LCD_Data to LCDREG
 *
 ******************************************************************************/

void task_lcd_update(void)
{
	if (++LCD_BlinkCounter > LCD_BLINK_FRAMES)
	{
		LCD_Bitplane = (LCD_Bitplane +1) & 1;
		LCD_BlinkCounter = 0;
		LCD_force_update = 1;
	}


	if (LCD_force_update)
	{
		LCD_force_update = 0;
		// Copy desired segment buffer to the real segments
		LCDDR0 = LCD_Data[LCD_Bitplane][0];
		LCDDR1 = LCD_Data[LCD_Bitplane][1];
		LCDDR2 = LCD_Data[LCD_Bitplane][2];
		LCDDR3 = LCD_Data[LCD_Bitplane][3];
		LCDDR5 = LCD_Data[LCD_Bitplane][4];
		LCDDR6 = LCD_Data[LCD_Bitplane][5];
		LCDDR7 = LCD_Data[LCD_Bitplane][6];
		LCDDR8 = LCD_Data[LCD_Bitplane][7];
		LCDDR10 = LCD_Data[LCD_Bitplane][8];
		LCDDR11 = LCD_Data[LCD_Bitplane][9];
		LCDDR12 = LCD_Data[LCD_Bitplane][10];
		LCDDR13 = LCD_Data[LCD_Bitplane][11];
		LCDDR15 = LCD_Data[LCD_Bitplane][12];
		LCDDR16 = LCD_Data[LCD_Bitplane][13];
		LCDDR17 = LCD_Data[LCD_Bitplane][14];
		LCDDR18 = LCD_Data[LCD_Bitplane][15];
	}

	if (LCD_used_bitplanes == 1)
	{
		// only one bitplane used, no blinking
		// Updated; disable LCD start of frame interrupt
		LCDCRA &= ~(1<<LCDIE);
	}
}

/*!
 *******************************************************************************
 *  LCD Interrupt Routine
 *
 *	\note used only for update LCD, in any other cases intterupt is disabled
 *  \note copy LCD_Data to LCDREG
 *
 ******************************************************************************/

// not optimized
/*ISR(LCD_vect)
{
	task |= TASK_LCD;
}
*/

// optimized
ISR_NAKED ISR (LCD_vect)
{
	asm volatile
	(
		// prologue and epilogue is not needed, this code  not touch flags in SREG
		"	sbi %0,%1" "\t\n"
		"	reti" "\t\n"
		::"I" (_SFR_IO_ADDR(task)) , "I" (TASK_LCD_BIT)
	);
}

