/*
 *  Sparmatic Comet (based on Open HR20)
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
 *				2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
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
 * \file       main.c
 * \brief      the main file for Open HR20 project
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>; Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

// AVR LibC includes
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>
#include <avr/wdt.h>

// HR20 Project includes
#include "config.h"
#include "main.h"
#include "adc.h"
#include "lcd.h"
#include "motor.h"
#include "rtc.h"
#include "task.h"
#include "keyboard.h"
#include "eeprom.h"
#include "debug.h"
#include "menu.h"
#include "com.h"
#include "controller.h"
#include "rfm_config.h"
#include "rfm.h"
#include "wireless.h"


// global Vars
//volatile bool    m_automatic_mode;         // auto mode (false: manu mode)

// global Vars for default values: temperatures and speed
// uint8_t valve_wanted=0;

// prototypes
int main(void);                            // main with main loop
static inline void init(void);                           // init the whole thing
void load_defauls(void);                   // load default values
                                           // (later from eeprom using config.c)
void callback_settemp(uint8_t);            // called from RTC to set new reftemp
void setautomode(bool);                    // activate/deactivate automode
uint8_t input_temp(uint8_t);

bool reboot = false;

// Check AVR LibC Version >= 1.6.0
#if __AVR_LIBC_VERSION__ < 10600UL
#warning "avr-libc >= version 1.6.0 recommended"
#warning "This code has not been tested with older versions."
#endif

/*!
 *******************************************************************************
 * main program
 ******************************************************************************/
int __attribute__ ((noreturn)) main(void)
// __attribute__((noreturn)) mean that we not need prologue and epilogue for main()
{
	//! initalization
	init();

	task = 0;
	display_task = 0;
	kb_events = KB_EVENT_UPDATE_LCD;	//Initial condition

	//! Enable interrupts
	sei();

	/* check EEPROM layout */
	if (EEPROM_read((uint16_t)&ee_layout) != EE_LAYOUT)
	{
		LCD_PrintString(PSTR("E5  "),LCD_MODE_ON);
		task_lcd_update();
		for(;;) {;}  //fatal error (EEPROM), stop startup
	}
	

	// enable persistent RX for initial sync
	RFM_FIFO_ON();
	RFM_RX_ON();
	RFM_INT_EN(); // enable RFM interrupt
	rfm_mode = rfmmode_rx;


	// We should do the following once here to have valid data from the start
    

	
	/*!
	****************************************************************************
	* main loop
	***************************************************************************/
	for (;;)
	{
	ADCSRA |= (1<<ADSC);

		// go to sleep with ADC conversion start
		asm volatile ("cli");
		if (! task && ((ASSR & (_BV(OCR2UB)|_BV(TCN2UB)|_BV(TCR2UB))) == 0))			// ATmega169 datasheet chapter 17.8.1
		{
  		// nothing to do, go to sleep
			if (timer0_need_clock())
			{
				SMCR = (0<<SM1)|(0<<SM0)|(1<<SE); // Idle mode
			}
			else
			{
				if (sleep_with_ADC)
				{
					SMCR = (0<<SM1)|(1<<SM0)|(1<<SE); // ADC noise reduction mode
				}
				else
				{
					SMCR = (1<<SM1)|(1<<SM0)|(1<<SE); // Power-save mode
				}
			}

			if (sleep_with_ADC)
			{
				sleep_with_ADC=false;
				// start conversions
				ADCSRA |= (1<<ADSC);
			}

			asm volatile ("sei");							//  sequence from ATMEL datasheet chapter 6.8.
			asm volatile ("sleep");
			asm volatile ("nop");

			SMCR = (1<<SM1)|(1<<SM0)|(0<<SE);	// Disable Power-save mode
			
		}
		else
		{
			asm volatile ("sei");
		}

		// RFM12
		if (task & TASK_RFM)
		{
			task &= ~TASK_RFM;

			if (rfm_mode == rfmmode_tx_done)
			{
				wirelessSendDone();
				if (reboot)
				{
					cli();
					wdt_enable(WDTO_15MS); //wd on,15ms
					while(1); //loop till reset
				}
			}
			else
				if ((rfm_mode == rfmmode_rx) || (rfm_mode == rfmmode_rx_owf))
				{
					wirelessReceivePacket();
				}
			continue; // on most case we have only 1 task, improve time to sleep
		}

		// update LCD task
		if (task & TASK_LCD)
		{
			task &= ~TASK_LCD;
			task_lcd_update();
			continue; // on most case we have only 1 task, improve time to sleep
		}

		if (task & TASK_ADC)
		{
			task &= ~TASK_ADC;
			if (!task_ADC())
			{
				// ADC is done
			}
			continue; // on most case we have only 1 task, improve time to sleep
		}

		// motor stop
		if (task & TASK_MOTOR_STOP)
		{
			task &= ~TASK_MOTOR_STOP;
			MOTOR_timer_stop();
			continue; // on most case we have only 1 task, improve time to sleep
		}

		//! check keyboard and set keyboards events
		if (task & TASK_KB)
		{
			task &= ~TASK_KB;
			task_keyboard();
		}

		if (task & TASK_RTC)
		{
			task &= ~TASK_RTC;
			if (RTC_timer_done & _BV(RTC_TIMER_RTC))
				RTC_AddOneSecond();
			
			if (RTC_timer_done & (_BV(RTC_TIMER_OVF)|_BV(RTC_TIMER_RTC)))
			{
				cli();
				RTC_timer_done &= ~(_BV(RTC_TIMER_OVF)|_BV(RTC_TIMER_RTC));
				sei();
				bool minute=(RTC_GetSecond()==0);
				CTL_update(minute);
				if (minute)
				{
					if (((CTL_error &  (CTL_ERR_BATT_LOW | CTL_ERR_BATT_WARNING)) == 0) && (RTC_GetDayOfWeek()==6) && (RTC_GetHour()==10) && (RTC_GetMinute()==0))
					{
						// every saturday 10:00AM
						// TODO: improve this code!
						// valve protection / CyCL
						MOTOR_updateCalibration(0);
					}
					wirelesTimeSyncCheck();
				}


				if ((config.RFM_devaddr!=0) && (time_sync_tmo>1))
				{
					if (((RTC_GetSecond() == config.RFM_devaddr) && (wireless_buf_ptr)) ||
					(
						(
								(RTC_GetSecond()>30) && ((RTC_GetSecond()&1) ? (wl_force_addr1==config.RFM_devaddr) : (wl_force_addr2==config.RFM_devaddr))
								) || (
								(wl_force_addr1==0xff) && (RTC_GetSecond()%30 == config.RFM_devaddr) && ((wl_force_flags>>config.RFM_devaddr)&1)
								)
						)
					) // collission protection: every HR20 shall send when the second counter is equal to it's own address.
					{
						wirelessTimerCase = WL_TIMER_FIRST;
						RTC_timer_set(RTC_TIMER_RFM, WLTIME_START);
					}
					
					if ((RTC_GetSecond() == 59) || (RTC_GetSecond() == 29))
					{
						#if (WL_SKIP_SYNC)
						if (wl_skip_sync!=0)
							wl_skip_sync--;
						else
						#endif
						{
							wirelessTimerCase = WL_TIMER_SYNC;
							RTC_timer_set(RTC_TIMER_RFM, WLTIME_SYNC);
						}
					}
				}
				
// !!!DEBUG!!!	if (bat_average>0)
//				{
//					MOTOR_updateCalibration(1);
//					MOTOR_Goto(valve_wanted);
//				}
				
				task_keyboard_long_press_detect();

				if ((MOTOR_Dir==stop) || (config.allow_ADC_during_motor))
					start_task_ADC();

				if (state_timeout>=0)
					state_timeout--;		//decrement every RTC Interrupt (1sec)
				
					display_task |= DISP_TASK_UPDATE;
			}
			
		
			if (RTC_timer_done & _BV(RTC_TIMER_RFM))
			{
				cli();
				RTC_timer_done &= ~_BV(RTC_TIMER_RFM);
				sei();
				wirelessTimer();
			}

			// do not use continue here (state_timeout==0)
		}

		// menu state machine
		if (kb_events || (state_timeout==0))
		{
			display_task |= DISP_TASK_UPDATE;
			if (state_controller())
				display_task = DISP_TASK_CLEAR | DISP_TASK_UPDATE;
		}

		// update motor PWM
		if (task & TASK_MOTOR_PULSE)
		{
			task &= ~TASK_MOTOR_PULSE;
			MOTOR_updateCalibration(1);
//			MOTOR_timer_pulse();
		}

		if (display_task)
		{
			menu_view(display_task & DISP_TASK_CLEAR);
			display_task = 0;
		}
	} //End Main loop
}

// default fuses for ELF file

#ifdef FUSE_BOOTSZ0
FUSES =
{
    .low = (uint8_t)(FUSE_CKSEL0 & FUSE_CKSEL2 & FUSE_CKSEL3 & FUSE_SUT0 & FUSE_CKDIV8),  //0x62
    .high = (uint8_t)(FUSE_BOOTSZ0 & FUSE_BOOTSZ1 & FUSE_EESAVE & FUSE_SPIEN & FUSE_JTAGEN),
    .extended = (uint8_t)(FUSE_BODLEVEL0),
};
#else
FUSES =
{
    .low = (uint8_t)(CKSEL0 & CKSEL2 & CKSEL3 & SUT0 & CKDIV8),  //0x62
    .high = (uint8_t)(BOOTSZ0 & BOOTSZ1 & EESAVE & SPIEN & JTAGEN),
    .extended = (uint8_t)(BODLEVEL0),
};
#endif

/*!
 *******************************************************************************
 * Initialize all modules
 ******************************************************************************/
static inline void init(void)
{
	//cli();
	uint8_t t = MCUCR | _BV(JTD);
	MCUCR=t;
	MCUCR=t;

	//! set Clock to 4 Mhz
	CLKPR = (1<<CLKPCE);            // prescaler change enable
	CLKPR = (1<<CLKPS0);            // prescaler = 2 (internal RC runs @ 8MHz)

	//! Calibrate the internal RC Oscillator
//	calibrate_rco();

	//! Disable Analog Comparator (power save)
	ACSR = (1<<ACD);

	//! Disable Digital input on PF0-7 (power save)
	DIDR0 = 0x07;

	//! Power reduction mode
	power_down_ADC();

	//! digital I/O port direction
	DDRE = (1<<PE7) | (1<<PE6); // PE6, PE7 Motor out

	//! enable pullup on all inputs (keys and m_wheel)
	DDRB =  (0<<PB0)|(1<<PB1)|(0<<PB2)|(0<<PB3)|(0<<PB4)|(0<<PB5)|(0<<PB6)|(0<<PB7);
	PORTB = (1<<PB0)|(0<<PB1)|(0<<PB2)|(0<<PB3)|(1<<PB4)|(1<<PB5)|(1<<PB6)|(1<<PB7);
	
	DDRE |= (1<<PE3);
	PORTE |= (1<<PE3);	// PE3 output (photo eye enable)
	DDRF = (1<<PF3);	// PF3 activate tempsensor
	PORTF = 0x0E0;

	//! remark for PCMSK0:
	//! PCINT0 for lighteye (motor monitor) is activated in motor.c using
	//! mask register PCMSK0: PCMSK0=(1<<PCINT4) and PCMSK0&=~(1<<PCINT4)



	PCMSK0=(1<<PCINT1);
	//! PCMSK1 for keyactions
	PCMSK1 = (1<<PCINT12)|(1<<PCINT13)|(1<<PCINT14)|(1<<PCINT15);

	//! activate PCINT0 + PCINT1
	EIMSK = _BV(PCIE1) | _BV(PCIE0); 



	//! Initialize the RTC
	RTC_Init();

	// press all keys on boot reload default eeprom values
	eeprom_config_init((PINB & (KBI_TIMER | KBI_OK | KBI_MENU))==0);


//	crypto_init();


	//! Initialize the motor
	//MOTOR_Init();

	//1 Initialize the LCD
	LCD_Init();


	RFM_init();
	RFM_OFF();


	// init keyboard
	state_wheel_prev = ~PINB & (KBI_ROT1 | KBI_ROT2);

	// restore saved temperature from config.timer
    if ( config.timer_mode > 1 )
	{
		CTL_temp_wanted = config.timer_mode >> 1;
		CTL_mode_auto = false;
	}
}

// interrupts: 

/*!
 *******************************************************************************
 * Pinchange Interupt INT0
 ******************************************************************************/
ISR (PCINT0_vect)
{
	uint8_t pine=PINE;

	MOTOR_interrupt(pine);

	RFM_interrupt(pine);
	// do NOT add anything after RFM part
}
