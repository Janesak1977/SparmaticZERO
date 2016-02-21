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
 * \file       lcd.h  
 * \brief      header file for lcd.c, functions to control the HR20 LCD
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       $Date$
 * $Rev$
 */

#pragma once

/*****************************************************************************
*   Macros
*****************************************************************************/

// Modes for LCD_SetSymbol
#define LCD_MODE_OFF        0		//!< (0b00) segment off
#define LCD_MODE_BLINK_1    1		//!< (0b01) segment on during 1. frame (blinking)
#define LCD_MODE_BLINK_2    2		//!< (0b10) segment on during 2. frame (blinking)
#define LCD_MODE_ON         3		//!< (0b11) segment permanent on

#define LCD_CONTRAST_INITIAL  14	//!< initial LCD contrast (0-15)
#define LCD_BLINK_FRAMES      12	//!< refreshes for each frame @ 64 frames/s
									//!< 12 refreshes -> 5Hz Blink frequency
#define LCD_BITPLANES          2	//!< \brief two bitplanes for blinking

/*****************************************************************************
*   Global Vars
*****************************************************************************/
extern volatile uint8_t LCD_used_bitplanes; //!< \brief number of used bitplanes / used for power
extern uint8_t LCD_force_update;        //!< \brief force update LCD

extern const uint8_t LCD_SegDayOfWeek[] PROGMEM;

/*****************************************************************************
*   Prototypes
*****************************************************************************/

void LCD_Init(void);                       // Init the LCD Controller
void LCD_AllSegments(uint8_t);             // Set all segments to LCD_MODE
void LCD_ClearAll(void);                   // Clear all segments
void LCD_ClearHourBar(void);               // Clear 24 bar segments
void LCD_ClearSymbols(void);               // Clear AUTO MANU PROG SUN MOON SNOW
void LCD_ClearNumbers(void);               // Clear 7 Segments and Collumns

void LCD_PrintDec(uint8_t, uint8_t, uint8_t);  // Print DEC-val (0-99)
void LCD_PrintDec3(uint16_t value, uint8_t pos, uint8_t mode); // Print DEC-val (0-255)
void LCD_PrintDecW(uint16_t, uint8_t);         // Print DEC-val (0-9999)                       
void LCD_PrintHex(uint8_t, uint8_t, uint8_t);  // Print HEX-val (0-ff)
void LCD_PrintHexW(uint16_t, uint8_t);         // Print HEX-val (0-ffff) 
void LCD_PrintChar(uint8_t, uint8_t, uint8_t); // Print one digit 
void LCD_PrintTemp(uint8_t, uint8_t);          // Print temperature (val+4,9)°C
void LCD_PrintTempInt(int16_t , uint8_t);      // Print temperature (val/100)°C
void LCD_PrintDayOfWeek(uint8_t, uint8_t);     // Print Day of Week (german)
void LCD_PrintString(char *string, uint8_t mode); // Print LCD string ID

void LCD_SetSeg(uint8_t, uint8_t);              // Set one Segment (0-69)
void LCD_SetSegReg(uint8_t, uint8_t, uint8_t);  // Set multiple LCD Segment(s) with common register 
void LCD_SetHourBarSeg(uint8_t, uint8_t);       // Set HBS (0-23) (Hour-Bar-Segment)
void LCD_HourBarBitmap(uint32_t bitmap);        // Set HBS like bitmap
void task_lcd_update(void);

#define  LCD_Update()  ((LCDCRA |= (1<<LCDIE)),(LCD_force_update=1))
	// Update at next LCD_ISR
	// Enable LCD start of frame interrupt



//***************************
// LCD Chars:
//***************************
#define LCD_CHAR_DEGREE  91  //!< degree char
#define LCD_CHAR_neg     92  //!< char "-"
#define LCD_CHAR_NULL    93  //!< space
#define LCD_CHAR_PERCENT 94  //!< percent %
	

#define	LCD_SEG_SCALE	0		// 0, 0 | SEG000 | [0], BIT 0
#define	LCD_SEG_2C	1		// 0, 1 | SEG001 | [0], BIT 1
#define	LCD_SEG_2D	2		// 0, 2 | SEG002 | [0], BIT 2
#define	LCD_SEG_2M	3		// 0, 3 | SEG003 | [0], BIT 3
#define	LCD_SEG_2E	4		// 0, 4 | SEG004 | [0], BIT 4
#define	LCD_SEG_1D	5		// 0, 5 | SEG005 | [0], BIT 5
#define	LCD_SEG_1M	6		// 0, 6 | SEG006 | [0], BIT 6
#define	LCD_SEG_1E	7		// 0, 7 | SEG007 | [0], BIT 7
#define	LCD_SEG_B2	8		// 1, 0 | SEG008 | [1], BIT 0
#define	LCD_SEG_B3	9		// 1, 1 | SEG009 | [1], BIT 1
#define	LCD_SEG_B8	10		// 1, 2 | SEG010 | [1], BIT 2
#define	LCD_SEG_B9	11		// 1, 3 | SEG011 | [1], BIT 3
#define	LCD_SEG_B14	12		// 1, 4 | SEG012 | [1], BIT 4
#define	LCD_SEG_B15	13		// 1, 5 | SEG013 | [1], BIT 5
#define	LCD_SEG_B21	14		// 1, 6 | SEG014 | [1], BIT 6
#define	LCD_SEG_B22	15		// 1, 7 | SEG015 | [1], BIT 7
#define	LCD_SEG_3E	16		// 2, 0 | SEG016 | [2], BIT 0
#define	LCD_SEG_3M	17		// 2, 1 | SEG017 | [2], BIT 1
#define	LCD_SEG_3D	18		// 2, 2 | SEG018 | [2], BIT 2
#define	LCD_SEG_4E	19		// 2, 3 | SEG019 | [2], BIT 3
#define	LCD_SEG_4M	20		// 2, 4 | SEG020 | [2], BIT 4
#define	LCD_SEG_4D	21		// 2, 5 | SEG021 | [2], BIT 5
#define	LCD_SEG_4C	22		// 2, 6 | SEG022 | [2], BIT 6
#define	LCD_SEG_RADIO	23		// 2, 7 | SEG023 | [2], BIT 7
#define	LCD_SEG_INSIDE	24		// 3, 0 | SEG024 | [3], BIT 0
#define	LCD_SEG_BAG	32		// 5, 0 | SEG100 | [4], BIT 0
#define	LCD_SEG_2K	33		// 5, 1 | SEG101 | [4], BIT 1
#define	LCD_SEG_2L	34		// 5, 2 | SEG102 | [4], BIT 2
#define	LCD_SEG_2G1	35		// 5, 3 | SEG103 | [4], BIT 3
#define	LCD_SEG_1C	36		// 5, 4 | SEG104 | [4], BIT 4
#define	LCD_SEG_1K	37		// 5, 5 | SEG105 | [4], BIT 5
#define	LCD_SEG_1L	38		// 5, 6 | SEG106 | [4], BIT 6
#define	LCD_SEG_1G1	39		// 5, 7 | SEG107 | [4], BIT 7
#define	LCD_SEG_B1	40		// 6, 0 | SEG108 | [5], BIT 0
#define	LCD_SEG_B4	41		// 6, 1 | SEG109 | [5], BIT 1
#define	LCD_SEG_B7	42		// 6, 2 | SEG110 | [5], BIT 2
#define	LCD_SEG_B10	43		// 6, 3 | SEG111 | [5], BIT 3
#define	LCD_SEG_B13	44		// 6, 4 | SEG112 | [5], BIT 4
#define	LCD_SEG_B16	45		// 6, 5 | SEG113 | [5], BIT 5
#define	LCD_SEG_B20	46		// 6, 6 | SEG114 | [5], BIT 6
#define	LCD_SEG_B23	47		// 6, 7 | SEG115 | [5], BIT 7
#define	LCD_SEG_3G1	48		// 7, 0 | SEG116 | [6], BIT 0
#define	LCD_SEG_3L	49		// 7, 1 | SEG117 | [6], BIT 1
#define	LCD_SEG_3K	50		// 7, 2 | SEG118 | [6], BIT 2
#define	LCD_SEG_3C	51		// 7, 3 | SEG119 | [6], BIT 3
#define	LCD_SEG_4G1	52		// 7, 4 | SEG120 | [6], BIT 4
#define	LCD_SEG_4L	53		// 7, 5 | SEG121 | [6], BIT 5
#define	LCD_SEG_4K	54		// 7, 6 | SEG122 | [6], BIT 6
#define	LCD_SEG_DOT	55		// 7, 7 | SEG123 | [6], BIT 7
#define	LCD_SEG_OUTSIDE	56		// 8, 0 | SEG124 | [7], BIT 0
#define	LCD_SEG_AUTO	64		// 10, 0 | SEG200 | [8], BIT 0
#define	LCD_SEG_2G2	65		// 10, 1 | SEG201 | [8], BIT 1
#define	LCD_SEG_2I	66		// 10, 2 | SEG202 | [8], BIT 2
#define	LCD_SEG_2H	67		// 10, 3 | SEG203 | [8], BIT 3
#define	LCD_SEG_2F	68		// 10, 4 | SEG204 | [8], BIT 4
#define	LCD_SEG_1G2	69		// 10, 5 | SEG205 | [8], BIT 5
#define	LCD_SEG_1I	70		// 10, 6 | SEG206 | [8], BIT 6
#define	LCD_SEG_1H	71		// 10, 7 | SEG207 | [8], BIT 7
#define	LCD_SEG_B0	72		// 11, 0 | SEG208 | [9], BIT 0
#define	LCD_SEG_B5	73		// 11, 1 | SEG209 | [9], BIT 1
#define	LCD_SEG_B6	74		// 11, 2 | SEG210 | [9], BIT 2
#define	LCD_SEG_B11	75		// 11, 3 | SEG211 | [9], BIT 3
#define	LCD_SEG_B12	76		// 11, 4 | SEG212 | [9], BIT 4
#define	LCD_SEG_B17	77		// 11, 5 | SEG213 | [9], BIT 5
#define	LCD_SEG_B19	78		// 11, 6 | SEG214 | [9], BIT 6
#define	LCD_SEG_SUN	79		// 11, 7 | SEG215 | [9], BIT 7
#define	LCD_SEG_3H	80		// 12, 0 | SEG216 | [10], BIT 0
#define	LCD_SEG_3I	81		// 12, 1 | SEG217 | [10], BIT 1
#define	LCD_SEG_3G2	82		// 12, 2 | SEG218 | [10], BIT 2
#define	LCD_SEG_4F	83		// 12, 3 | SEG219 | [10], BIT 3
#define	LCD_SEG_4H	84		// 12, 4 | SEG220 | [10], BIT 4
#define	LCD_SEG_4I	85		// 12, 5 | SEG221 | [10], BIT 5
#define	LCD_SEG_4G2	86		// 12, 6 | SEG222 | [10], BIT 6
#define	LCD_SEG_BAT	87		// 12, 7 | SEG223 | [10], BIT 7
#define	LCD_SEG_MOON	88		// 13, 0 | SEG224 | [11], BIT 0
#define	LCD_SEG_MANU	96		// 15, 0 | SEG300 | [12], BIT 0
#define	LCD_SEG_2B	97		// 15, 1 | SEG301 | [12], BIT 1
#define	LCD_SEG_2J	98		// 15, 2 | SEG302 | [12], BIT 2
#define	LCD_SEG_2A	99		// 15, 3 | SEG303 | [12], BIT 3
#define	LCD_SEG_1B	100		// 15, 4 | SEG304 | [12], BIT 4
#define	LCD_SEG_1J	101		// 15, 5 | SEG305 | [12], BIT 5
#define	LCD_SEG_1A	102		// 15, 6 | SEG306 | [12], BIT 6
#define	LCD_SEG_1F	103		// 15, 7 | SEG307 | [12], BIT 7
#define	LCD_SEG_MON	104		// 16, 0 | SEG308 | [13], BIT 0
#define	LCD_SEG_TUE	105		// 16, 1 | SEG309 | [13], BIT 1
#define	LCD_SEG_WED	106		// 16, 2 | SEG310 | [13], BIT 2
#define	LCD_SEG_THU	107		// 16, 3 | SEG311 | [13], BIT 3
#define	LCD_SEG_FRI	108		// 16, 4 | SEG312 | [13], BIT 4
#define	LCD_SEG_SAT	109		// 16, 5 | SEG313 | [13], BIT 5
#define	LCD_SEG_B18	110		// 16, 6 | SEG314 | [13], BIT 6
#define	LCD_SEG_DOUBLECOL	111		// 16, 7 | SEG315 | [13], BIT 7
#define	LCD_SEG_3F	112		// 17, 0 | SEG316 | [14], BIT 0
#define	LCD_SEG_3A	113		// 17, 1 | SEG317 | [14], BIT 1
#define	LCD_SEG_3J	114		// 17, 2 | SEG318 | [14], BIT 2
#define	LCD_SEG_3B	115		// 17, 3 | SEG319 | [14], BIT 3
#define	LCD_SEG_4A	116		// 17, 4 | SEG320 | [14], BIT 4
#define	LCD_SEG_4J	117		// 17, 5 | SEG321 | [14], BIT 5
#define	LCD_SEG_4B	118		// 17, 6 | SEG322 | [14], BIT 6
#define	LCD_SEG_PADLOCK	119		// 17, 7 | SEG323 | [14], BIT 7
#define	LCD_SEG_SNOW 	120		// 18, 0 | SEG324 | [15], BIT 0

#define LCD_DECIMAL_DOT		LCD_SEG_DOT


