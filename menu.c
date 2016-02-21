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
 * \file       menu.c
 * \brief      menu view & controler for free HR20E project
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <stdint.h>

#include "config.h"
#include "main.h"
#include "keyboard.h"
#include "adc.h"
#include "lcd.h"
#include "rtc.h"
#include "motor.h"
#include "eeprom.h"
#include "debug.h"
#include "controller.h"
#include "menu.h"
#include "watch.h"


/*!
 *******************************************************************************
 * \brief state_timeout is timer for autoupdates of menu
 * \note val<0 means no autoupdate 
 ******************************************************************************/
int8_t state_timeout = 2;

/*!
 *******************************************************************************
 * \brief flag for locking keys of the unit
 ******************************************************************************/
bool menu_locked = false; 

/*!
 *******************************************************************************
 * \brief possible states for the menu state machine
 ******************************************************************************/
typedef enum
{
	// startup
	state_startup, state_dispversion,
	// home screens
	home_screen_no_alter, home_screen, home_screen2, home_screen3, home_screen4, home_screen5,

	state_mainmenu,
	
	menu_MODE, menu_PROG, menu_TEMP, menu_TIME, menu_WINDOW, menu_ADAPT
} state_t;


const uint8_t MainMenuItems[][4] PROGMEM =
{
	"MODE", "PROG", "TEMP", "TIME", "WNDW", "ADAP"
};

const uint8_t LCD_ProgTypeStringTable[][4] PROGMEM =
{
	"DAY1", "DAY2", "DAY3", "DAY4", "DAY5", "DAY6", "DAY7", "D1\\5", "D1\\7"		//note "\" char is displayed as "-", see LCD_CHAR_neg definition
};



/*!
 *******************************************************************************
 * \brief kb_events common reactions
 * 
 * \returns true for controler restart
 ******************************************************************************/

static int8_t wheel_proccess(void)
{
	int8_t ret=0;
	if (kb_events & KB_EVENT_WHEEL_PLUS)
		ret= 1;
	else
		if (kb_events & KB_EVENT_WHEEL_MINUS)
			ret= -1;
	return ret;
}

/*!
 *******************************************************************************
 * local menu static variables
 ******************************************************************************/
static state_t state;
static uint8_t submenu;
static uint8_t submenu_state;
#define menu_PROG_state submenu_state //reuse variable in another part of menu
static uint8_t CTLMODEAUTO_temp;
static uint8_t menu_set_temp_type;
static uint8_t menu_set_dow;
static uint8_t menu_prog_hour;
#define menu_set_temp menu_set_dow //reuse variable in another part of menu
#define menu_time_state menu_set_temp_type	//reuse variable in another part of menu
#define menu_prog_state menu_set_temp_type	//reuse variable in another part of menu
#define menu_prog_day menu_set_dow 
static uint16_t menu_set_time;
static timermode_t menu_set_mode;
static uint8_t menu_set_slot;
static uint8_t service_idx = 0;
static uint8_t service_watch_n = 0;
static uint32_t hourbar_buff;
static uint32_t menu_prog_hourbarbitmap;

/*!
 *******************************************************************************
 * \brief kb_events common reactions to key presses
 * 
 * \returns true to request display update
 ******************************************************************************/

static bool events_common(void)
{
	bool ret=false;
	if (kb_events & KB_EVENT_LOCK_LONG)               // key lock
	{
		menu_locked = ! menu_locked;
		state = home_screen_no_alter;
		ret = true;
	}
	else
		if (!menu_locked)
		{    
/*			if (kb_events & KB_EVENT_ALL_LONG)            // no activity
			{
				ret = true;
			}
			else
			if ( kb_events & KB_EVENT_MENU_LONG )  	// no activity
			{
				ret = true; 
			}
			else
			if ( kb_events & KB_EVENT_TIMER_LONG )  // no activity
			{
				ret = true;
			}
			else
			if ( kb_events & KB_EVENT_OK_LONG )     // no activity
			{
				ret = true;
			}
			else */
			if ( ( kb_events & KB_EVENT_NONE_LONG )   // retun to main home screen on timeout
				  && (state==state_mainmenu || (state>=menu_MODE && state<=menu_ADAPT))
				)
			{
				state = home_screen_no_alter;
				ret = true;
			}
		}
		
	return ret;
}

/*!
 *******************************************************************************
 * \brief Main state Controller: state transitions in state machine
 * 
 * \returns true to request display update
 ******************************************************************************/
bool state_controller(void)
{
	int8_t wheel = wheel_proccess(); //signed number
	bool ret = false;
	switch (state)
	{
		case state_startup:
			if (state_timeout==0)
			{
				state = state_dispversion;
				state_timeout = 1;
				ret = true;
			}
		break;
		
		case state_dispversion:
			if (state_timeout==0)
			{
			//	menu_update_hourbar((config.timer_mode==1)?RTC_GetDayOfWeek():0);
				state = home_screen_no_alter;
				ret=true;
			}
		break;
		
	#if (! REMOTE_SETTING_ONLY)

		case state_mainmenu:
			
				
				if (wheel==1)
					if (submenu==menu_ADAPT)
						submenu = menu_MODE;
					else
						submenu++;		
				else
				if (wheel==-1)
					if (submenu==menu_MODE)
						submenu = menu_ADAPT;
					else
						submenu--;
				else
				if	( kb_events & KB_EVENT_OK)
				{
					state = submenu;
					if (submenu==menu_MODE)
						CTLMODEAUTO_temp = CTL_mode_auto;		//Initialization
					if (submenu==menu_TEMP);
					{
						menu_set_temp_type = 0;		//tempterature0 first
						menu_set_temp = temperature_table[menu_set_temp_type];
					}
					if (submenu==menu_TIME);
						menu_time_state = 0;		// set Year first

					if (submenu==menu_PROG);
					{
						menu_PROG_state = 0;	// start with DOW select
						menu_set_dow = 0;		// start with Monday
						menu_prog_hourbarbitmap = RTC_DowTimerGetHourBar(menu_set_dow);
					}
				}

				if ( kb_events & (KB_EVENT_OK_REWOKE | KB_EVENT_MENU_REWOKE) )
				{
					state = home_screen_no_alter;
					ret = true;
				}

		break;

		case menu_MODE:
			if (wheel!=0)
				CTLMODEAUTO_temp = !CTLMODEAUTO_temp;
			else
			if ( kb_events & KB_EVENT_OK)
			{
				if (CTL_mode_auto!=CTLMODEAUTO_temp)
					CTL_change_mode(CTL_CHANGE_MODE);
				state = home_screen_no_alter;
				ret = true;
			}
			else
			if (kb_events & KB_EVENT_MENU)
			{
				state = home_screen_no_alter;
				ret = true;
			}
		break;

		case menu_TEMP:
			if (wheel != 0)
			{
				menu_set_temp += wheel;
				if (menu_set_temp > TEMP_MAX+1)
					menu_set_temp = TEMP_MAX+1;
					
				if (menu_set_temp < TEMP_MIN-1)
					menu_set_temp = TEMP_MIN-1;
			}
			else
			if (kb_events & KB_EVENT_OK)
			{
				if (menu_set_temp_type==0)
				{
					temperature_table[menu_set_temp_type] = menu_set_temp;	//store temperature0
					menu_set_temp_type = 1;									// now go to set temperature1
					menu_set_temp = temperature_table[menu_set_temp_type];	// read actual temperature1
				}
				else
				{
					temperature_table[menu_set_temp_type] = menu_set_temp;
					state = home_screen_no_alter;
				}
			}
			else
			if (kb_events & KB_EVENT_MENU)
				state = home_screen_no_alter;
		break;

		case menu_TIME:
			switch(menu_time_state)
			{
				case 0:
					if (wheel!=0)
						RTC_SetYear(RTC_GetYearYY()+wheel);
					else
					if (kb_events & KB_EVENT_OK)
						menu_time_state = 1;	// go to next state
					else 
					if (kb_events & KB_EVENT_MENU)
					{
						state_timeout = 0;
						CTL_update_temp_auto();
						state = home_screen_no_alter;
						ret = true;
					}
				break;
				
				case 1:
					if (wheel!=0)
						RTC_SetMonth(RTC_GetMonth()+wheel);
					else
					if (kb_events & KB_EVENT_OK)
						menu_time_state = 2;	// go to next state
					else 
					if (kb_events & KB_EVENT_MENU)
					{
						state_timeout = 0;
						CTL_update_temp_auto();
						state = home_screen_no_alter;
						ret = true;
					}
				break;
				
				case 2:
					if (wheel!=0)
						RTC_SetDay(RTC_GetDay()+wheel);
					else
					if (kb_events & KB_EVENT_OK)
						menu_time_state = 3;	// go to next state
					else 
					if (kb_events & KB_EVENT_MENU)
					{
						state_timeout = 0;
						CTL_update_temp_auto();
						state = home_screen_no_alter;
						ret = true;
					}
				break;
				
				case 3:
					if (wheel!=0)
						RTC_SetHour(RTC_GetHour()+wheel);
					else
					if (kb_events & KB_EVENT_OK)
						menu_time_state = 4;	// go to next state
					else 
					if (kb_events & KB_EVENT_MENU)
					{
						state_timeout = 0;
						CTL_update_temp_auto();
						state = home_screen_no_alter;
						ret = true;
					}
				break;
				
				case 4:
					if (wheel!=0)
					{
						RTC_SetMinute(RTC_GetMinute()+wheel);
						RTC_SetSecond(0);
					}
					else
					if ((kb_events & KB_EVENT_OK) || (kb_events & KB_EVENT_MENU) )
					{
						state_timeout = 0;
						CTL_update_temp_auto();
						state = home_screen_no_alter;
						ret = true;
					}
				break;						

			}

		break;

		case menu_PROG:
			if(menu_PROG_state==0)		// select DOW for timer set
			{
				if (wheel != 0)
				{
					menu_set_dow = (menu_set_dow+wheel+9)%9;	//Increment DOW (0 to 8)
					if (menu_set_dow<7)
						menu_prog_hourbarbitmap = RTC_DowTimerGetHourBar(menu_set_dow);		//Get actual houtbar bitmap for DOW
					else
						menu_prog_hourbarbitmap = DEFAULT_FACTORY_HOURBAR;		//For block (T1-5,T1-7) set default value
						
				}
				if (kb_events & KB_EVENT_OK)
				{
					menu_PROG_state = 1;
					menu_set_slot = 0;
					if (menu_set_dow<6)
						menu_set_time = RTC_DowTimerGet(menu_set_dow, menu_set_slot, &menu_set_mode);
					else
					if (menu_set_dow>=7)			// for MON-FRI and MON-SUN days 
					{
						menu_set_time = 7*60;			//default COMFORT temp start time
						menu_set_mode = temperature1;	
					}
					
					ret = true;
				}
				else
				if (kb_events & KB_EVENT_MENU)
				{
					menu_update_hourbar((config.timer_mode==1)?RTC_GetDayOfWeek():0);
					state = home_screen_no_alter;
					ret = true;		
				}		
			}
			else
			if (menu_PROG_state==1)		// Select starting of heating time
			{
				if (wheel != 0)
					menu_set_time = ((menu_set_time/10+(24*6+1)+wheel)%(24*6+1))*10;
				else
				if (kb_events & KB_EVENT_OK)
				{
					if ( (menu_set_time>(24*60+10)) && (menu_set_slot>1) && (menu_set_mode==1) )
					{
						// exit only if in COMFORTtemp(temperature1) and Slot>1 and Time>24*6+1
						menu_update_hourbar((config.timer_mode==1)?RTC_GetDayOfWeek():0);
						state = home_screen_no_alter;
						ret = true;
					}
					else
					{
						// Store actual slot
						if (menu_set_dow<7)
						{
							RTC_DowTimerSet(menu_set_dow, menu_set_slot, menu_set_time, menu_set_mode);
						
							if (++menu_set_slot>=RTC_TIMERS_PER_DOW)
							{
								if (menu_set_dow != 0)
									menu_set_dow = menu_set_dow%7+1; 
							}
							else
								menu_set_time = RTC_DowTimerGet(menu_set_dow, menu_set_slot, &menu_set_mode);
						}
						else
						{
							if (menu_set_dow==7)	// set timers for Monday to Sunday
							{
								uint8_t i;
								for (i=0;i<5;i++)
									RTC_DowTimerSet(i, menu_set_slot, menu_set_time, menu_set_mode);
							}
							else
							{
								uint8_t i;
								for (i=0;i<7;i++)
									RTC_DowTimerSet(i, menu_set_slot, menu_set_time, menu_set_mode);
							}

							if (++menu_set_slot>=RTC_TIMERS_PER_DOW)
							{	// exit
								menu_update_hourbar((config.timer_mode==1)?RTC_GetDayOfWeek():0);
								state = home_screen_no_alter;
								ret = true;
							}
							else
							{
								if ((menu_set_slot%2)==0)
									menu_set_time = 7*60; 
								else
									menu_set_time = 22*60;
								
								menu_set_mode = (menu_set_mode + 1) % 2;
							}
								 
						}
					}
				}
				else
				if (kb_events & KB_EVENT_MENU)
				{
					menu_update_hourbar((config.timer_mode==1)?RTC_GetDayOfWeek():0);
					state = home_screen_no_alter;
					ret = true;	
				}	
			}

		break;

	
		#endif
		
		case home_screen_no_alter: // same as home screen, but without alternate content
			if ( kb_events & KB_EVENT_OK )
			{
				state = home_screen;
				state_timeout = HOME_SCREEN_AUTOUPDATE_INTERVAL;
				ret = true;
			}
			else
			if (!menu_locked)
			{
				if ( kb_events & KB_EVENT_MENU )
				{		
					state = state_mainmenu;
					submenu = menu_MODE;
					ret = true; 
				}
				else
				if (wheel != 0)
				{
					CTL_temp_change_inc(wheel);
					ret = true; 
				}
			}	
		break;

		case home_screen:         // home screen
		case home_screen2:        // alternate version, real temperature
		case home_screen3:        // alternate version, valve pos
		case home_screen4:        // alternate version, time
		case home_screen5:        // alternate version, battery  
			if( kb_events & KB_EVENT_OK_LONG )
			{	
				state = home_screen_no_alter;
				ret = true;
			}
			else   
			if ( (kb_events & KB_EVENT_OK) || (state_timeout==0) )
			{
				state_timeout = HOME_SCREEN_AUTOUPDATE_INTERVAL;
				state++;       // go to next alternate home screen
				if (state > home_screen5)
					state = home_screen;
				ret = true; 
			}
				
		break;
	}
	
	if (events_common())
		ret = true;
		
	// turn off LCD blinking on wheel activity
/*	if (wheel != 0 && (
		
		(state >= state_mainmenu && state <= menu_ADAPT) || (state >= state_preset_temp0 && state <= state_preset_temp3) ||
		
		))
	{
		state_timeout=2;
	}
*/	
	kb_events = 0; // clear unused keys
	return ret;
} 

/*!
 *******************************************************************************
 * view helper funcions for clear display
 *******************************************************************************/
/*
	// not used generic version, easy to read but longer code
	static void clr_show(uint8_t n, ...) {
		uint8_t i;
		uint8_t *p;
		LCD_AllSegments(LCD_MODE_OFF);
		p=&n+1;
		for (i=0;i<n;i++) {
			LCD_SetSeg(*p, LCD_MODE_ON);
			p++;
		}
	}
*/


static void clr_show1(uint8_t seg1)
{
	LCD_AllSegments(LCD_MODE_OFF);
	LCD_SetSeg(seg1, LCD_MODE_ON);
}

static void clr_show2(uint8_t seg1, uint8_t seg2)
{
	LCD_AllSegments(LCD_MODE_OFF);
	LCD_SetSeg(seg1, LCD_MODE_ON);
	LCD_SetSeg(seg2, LCD_MODE_ON);
}

static void clr_show3(uint8_t seg1, uint8_t seg2, uint8_t seg3)
{
	LCD_AllSegments(LCD_MODE_OFF);
	LCD_SetSeg(seg1, LCD_MODE_ON);
	LCD_SetSeg(seg2, LCD_MODE_ON);
	LCD_SetSeg(seg3, LCD_MODE_ON);
}

static void show_selected_temperature_type (uint8_t type, uint8_t mode)
{
	//  temperature0    INHOUSE         
	//  temperature1    MOON
	LCD_SetSeg(LCD_SEG_INSIDE,((type==temperature1) ? mode:LCD_MODE_OFF));
	LCD_SetSeg(LCD_SEG_MOON,((type==temperature0) ? mode:LCD_MODE_OFF));
}

/*!
 *******************************************************************************
 * \brief Update hourbar bitmap buffer to be used by LCD update
 * 
 * \returns true for controler restart
 ******************************************************************************/
void menu_update_hourbar(uint8_t dow)
{
	hourbar_buff = RTC_DowTimerGetHourBar(dow);
}

/*!
 *******************************************************************************
 * \brief menu View
 ******************************************************************************/
void menu_view(bool clear)
{
	uint8_t lcd_blink_mode = state_timeout>0?LCD_MODE_ON:LCD_MODE_BLINK_1;

	
	switch (state)
	{
		case state_startup:
			LCD_AllSegments(LCD_MODE_ON);                   // all segments on
		break;
	
		case state_dispversion:
			clr_show1(LCD_DECIMAL_DOT);
			LCD_PrintHexW(VERSION_N,LCD_MODE_ON);
		break; 
		
	 	case state_mainmenu:
			if (clear)
				clr_show1(LCD_SEG_SCALE);	
			
			LCD_PrintString(MainMenuItems[submenu-9], LCD_MODE_ON);
	 	break;

		case menu_MODE:
			if (CTLMODEAUTO_temp)
				LCD_PrintString(PSTR("AUTO"),LCD_MODE_BLINK_1);
			else
				LCD_PrintString(PSTR("MANU"),LCD_MODE_BLINK_1);
		break;

		case menu_TEMP:
			show_selected_temperature_type(menu_set_temp_type, LCD_MODE_ON);
			LCD_PrintTemp(menu_set_temp,LCD_MODE_BLINK_1);
		break;

		case menu_TIME:
			switch(menu_time_state)
			{
				case 0:
					clr_show1(LCD_SEG_SCALE);           // show only hour scale
					LCD_PrintDecW(RTC_GetYearYYYY(),lcd_blink_mode);	
				break;
			
				case 1:
				case 2:
					clr_show1(LCD_DECIMAL_DOT);           // decimal point
					LCD_PrintDec(RTC_GetMonth(), 0, ((menu_time_state==1)?LCD_MODE_BLINK_1:LCD_MODE_ON));
					LCD_PrintDec(RTC_GetDay(), 2, ((menu_time_state==2)?LCD_MODE_BLINK_1:LCD_MODE_ON));
				break;

				case 3:
				case 4:
					clr_show1(LCD_SEG_DOUBLECOL);
					LCD_PrintDec(RTC_GetHour(), 2, ((menu_time_state == 3) ? LCD_MODE_BLINK_1 : LCD_MODE_ON));
					LCD_PrintDec(RTC_GetMinute(), 0, ((menu_time_state == 4) ? LCD_MODE_BLINK_1 : LCD_MODE_ON));
				break;	
			}
		break;

		case menu_PROG:
			if (menu_PROG_state==0)
			{
				clr_show1(LCD_SEG_SCALE);
				LCD_PrintString(LCD_ProgTypeStringTable[menu_set_dow], LCD_MODE_BLINK_1);
				
				if (menu_set_dow<7)
				{
					LCD_SetDOW(menu_set_dow, LCD_MODE_ON);
					LCD_HourBarBitmap(menu_prog_hourbarbitmap);
				}
				else
				{
					LCD_HourBarBitmap(DEFAULT_FACTORY_HOURBAR);
					if (menu_set_dow==7)
						LCD_SetDOWBar(4,LCD_MODE_ON);
					else
						LCD_SetDOWBar(6,LCD_MODE_ON);
				}
			}
			else
			if (menu_PROG_state==1)
			{
				clr_show1(LCD_SEG_SCALE);
				LCD_HourBarBitmap(menu_prog_hourbarbitmap);
				LCD_SetSeg(LCD_SEG_DOUBLECOL, LCD_MODE_ON);
				LCD_SetDOW(menu_set_dow, LCD_MODE_ON);
				LCD_SetHourBarSeg(menu_set_time/60, LCD_MODE_BLINK_1);
				if (menu_set_time < 24*60)
				{
					LCD_PrintDec(menu_set_time/60, 0, LCD_MODE_BLINK_1);
					LCD_PrintDec(menu_set_time%60, 2, LCD_MODE_BLINK_1);
				}
				else
				{
					LCD_PrintChar(LCD_CHAR_neg, 0, LCD_MODE_BLINK_1);
					LCD_PrintChar(LCD_CHAR_neg, 1, LCD_MODE_BLINK_1);
					LCD_PrintChar(LCD_CHAR_neg, 2, LCD_MODE_BLINK_1);
					LCD_PrintChar(LCD_CHAR_neg, 3, LCD_MODE_BLINK_1);		//show ---- time on display		
				}
				
				show_selected_temperature_type(menu_set_mode, LCD_MODE_ON);
			}
		break;

		case home_screen_no_alter: // wanted temp
			if (clear)
				clr_show2(CTL_mode_auto ? LCD_SEG_AUTO : LCD_SEG_MANU, LCD_SEG_SCALE);
				//! \note hourbar status calculation is complex we don't want calculate it every view, use cache

			LCD_HourBarBitmap(hourbar_buff);
			// day of week icon
			LCD_SetSeg(pgm_read_byte(&LCD_SegDayOfWeek[RTC_GetDayOfWeek()]), LCD_MODE_ON);

			// display active temperature type in automatic mode
			if (CTL_test_auto())
				show_selected_temperature_type(CTL_temp_auto_type, LCD_MODE_ON);

			LCD_PrintTemp(CTL_temp_wanted,LCD_MODE_ON);
		break;


		case home_screen: // wanted temp / error code / adaptation status
			if (clear)
				clr_show1(CTL_mode_auto ? LCD_SEG_AUTO : LCD_SEG_MANU);
			if (MOTOR_calibration_step > 0)
			{
				LCD_PrintString(PSTR("ADPT"),LCD_MODE_ON);
				//if (MOTOR_ManuCalibration==-1)
				//	LCD_PrintChar('d',1,LCD_MODE_ON);
				//LCD_PrintChar(MOTOR_calibration_step%10+48, 0, LCD_MODE_ON);
				break;
			}
			else
			{
				if ((CTL_error & ~(CTL_ERR_BATT_LOW | CTL_ERR_BATT_WARNING))!=0)
				{
					if (CTL_error & CTL_ERR_MONTAGE)
						LCD_PrintString(PSTR("E2  "),LCD_MODE_ON);
					else

					if (CTL_error & CTL_ERR_MOTOR)
						LCD_PrintString(PSTR("E3  "),LCD_MODE_ON);
					else

					if (CTL_error & CTL_ERR_RFM_SYNC)
						LCD_PrintString(PSTR("E4  "),LCD_MODE_ON);
				break;
				}
			}
		break;

		case home_screen2: // real temperature
			if (clear)
				LCD_AllSegments(LCD_MODE_OFF);

			LCD_PrintTempInt(temp_average,LCD_MODE_ON);
		break;
	
		case home_screen3: // valve pos
			if (clear)			
				LCD_AllSegments(LCD_MODE_OFF);
			uint8_t prc = MOTOR_GetPosPercent();
			
			if (prc <= 100)
			{
				LCD_PrintDec3(prc, 0 ,LCD_MODE_ON);
				LCD_PrintChar(LCD_CHAR_PERCENT,3,LCD_MODE_ON);
			}
			else
				LCD_PrintString(PSTR("100>"),LCD_MODE_ON);
		break;

		case home_screen4: // time
			if (clear)
				clr_show1(LCD_SEG_DOUBLECOL);
			LCD_PrintDec(RTC_GetHour(), 0,  LCD_MODE_ON);
			LCD_PrintDec(RTC_GetMinute(), 2, LCD_MODE_ON);
		break;                                       

		case home_screen5:        // battery 
			LCD_AllSegments(LCD_MODE_OFF);
			LCD_PrintDec(bat_average/100, 0, LCD_MODE_ON);
			LCD_PrintDec(bat_average%100, 2, LCD_MODE_ON);
		break; 	       
              
	}	//switch statement end


	if (menu_locked)
		LCD_SetSeg(LCD_SEG_PADLOCK, LCD_MODE_ON);
	
	mode_window()?LCD_SetSeg(LCD_SEG_SNOW, LCD_MODE_ON) : LCD_SetSeg(LCD_SEG_SNOW, LCD_MODE_OFF);

	if (bat_average < 20*(uint16_t)config.bat_low_thld)
		if (state != state_startup)
			LCD_SetSeg(LCD_SEG_BAT, LCD_MODE_BLINK_1);
	else
		if (bat_average < 20*(uint16_t)config.bat_half_thld)
			LCD_SetSeg(LCD_SEG_BAT, LCD_MODE_ON);
	
	LCD_Update();
}

/*!
 * \note Switch used in this file can generate false warning on some AvrGCC versions
 *       we can ignore it
 *       details:  http://osdir.com/ml/hardware.avr.libc.devel/2006-11/msg00005.html
 */
