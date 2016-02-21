/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:   WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
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
 * \file       com.c
 * \brief      comunication
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/wdt.h>


#include "config.h"
#include "com.h"
#include "main.h"
#include "rtc.h"
#include "adc.h"
#include "task.h"
#include "watch.h"
#include "eeprom.h"
#include "controller.h"
#include "menu.h"
#include "wireless.h"
#include "debug.h"



/*!
 *******************************************************************************
 *  \brief helper function print version string
 *
 *  \note
 ******************************************************************************/
static void print_version(bool sync)
{
	const char * s = (PSTR(VERSION_STRING "\n"));
	
	char c;
	for (c = pgm_read_byte(s); c; ++s, c = pgm_read_byte(s))
	{
		if (sync)
			wireless_putchar(c);
   	}
}



/*!
 *******************************************************************************
 *  \brief init communication
 *
 *  \note
 ******************************************************************************/
void COM_init(void)
{
	print_version(false);
}


/*!
 *******************************************************************************
 *  \brief Print debug line
 *
 *  \note
 ******************************************************************************/
void COM_print_debug(uint8_t type)
{
	bool sync = (type==2);
	if (!sync)
	{
		wireless_async = true;
		wireless_putchar('D');
	}
	wireless_putchar(RTC_GetMinute() | (CTL_test_auto()?0x40:0) | ((CTL_mode_auto)?0x80:0));
	wireless_putchar(RTC_GetSecond() | ((mode_window())?0x40:0) | ((menu_locked)?0x80:0));
	wireless_putchar(CTL_error);
	wireless_putchar(temp_average >> 8); // current temp
	wireless_putchar(temp_average & 0xff);
	wireless_putchar(bat_average >> 8); // current temp
	wireless_putchar(bat_average & 0xff);
	wireless_putchar(CTL_temp_wanted); // wanted temp
	wireless_putchar(valve_wanted); // valve pos
	wireless_async = false;
	rfm_start_tx();
}


static void COM_wireless_word(uint16_t w)
{
	wireless_putchar(w >> 8);
	wireless_putchar(w & 0xff); 
}

/*!
 *******************************************************************************
 *  \brief parse command from wireless
 *******************************************************************************
 */ 
void COM_wireless_command_parse (uint8_t * rfm_framebuf, uint8_t rfm_framepos)
{
	uint8_t pos=0;
	while (rfm_framepos > pos)
	{
		uint8_t c=rfm_framebuf[pos++];
		wireless_putchar(c|0x80);
		switch(c)
		{
		case 'V':
			print_version(true);
		break;
			
		case 'D':
			COM_print_debug(2);
		break;
		
		case 'T':
			wireless_putchar(rfm_framebuf[pos]);
  			COM_wireless_word(watch(rfm_framebuf[pos]));
			pos++;
		break;
		
		case 'G':
		case 'S':
			if (c == 'S')
			{
  				if (rfm_framebuf[pos] < CONFIG_RAW_SIZE)
				{
  					config_raw[rfm_framebuf[pos]]=(uint8_t)(rfm_framebuf[pos+1]);
  					eeprom_config_save(rfm_framebuf[pos]);
  				}
			}
			wireless_putchar(rfm_framebuf[pos]);
			if (rfm_framebuf[pos] == 0xff)
			{
				 wireless_putchar(EE_LAYOUT);
			}
			else
			{
				 wireless_putchar(config_raw[rfm_framebuf[pos]]);
			}
			if (c == 'S')
				pos++;
			pos++;
		break;
		
		case 'R':
		case 'W':
			if (c == 'W')
			{
  				RTC_DowTimerSet(rfm_framebuf[pos]>>4, rfm_framebuf[pos]&0xf, (((uint16_t) (rfm_framebuf[pos+1])&0xf)<<8)+(uint16_t)(rfm_framebuf[pos+2]), (rfm_framebuf[pos+1])>>4);
				CTL_update_temp_auto();
			}
			wireless_putchar(rfm_framebuf[pos]);
			COM_wireless_word(eeprom_timers_read_raw(timers_get_raw_index((rfm_framebuf[pos]>>4), (rfm_framebuf[pos]&0xf))));
			if (c == 'W')
				pos+=2;
			pos++;
		break;
		
		case 'B':
  			if ((rfm_framebuf[pos]==0x13) && (rfm_framebuf[pos+1]==0x24))
				reboot = true;
			wireless_putchar(rfm_framebuf[pos]);
			wireless_putchar(rfm_framebuf[pos+1]);
			pos += 2;
		break;
		
		case 'M':
			CTL_change_mode(rfm_framebuf[pos++]);
			COM_print_debug(2);
		break;
		
		case 'A':
			if (rfm_framebuf[pos] < TEMP_MIN-1)
			{
				break;
			}
			
			if (rfm_framebuf[pos] > TEMP_MAX+1)
			{
				break;
			}
			
			CTL_set_temp(rfm_framebuf[pos++]);
			COM_print_debug(2);
		break;
		
		case 'L':
			if (rfm_framebuf[pos] <= 1)
				menu_locked = rfm_framebuf[pos];
			wireless_putchar(menu_locked);
			pos++;
		break;
		
		default:
		break;
		}
	}
}

