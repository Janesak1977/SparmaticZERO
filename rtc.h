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
 * \file       rtc.h
 * \brief      header file for rtc.c, to control the HR20 clock and timers
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       $Date$
 * $Rev$
 */

#pragma once

#ifndef RTC_H
#define RTC_H

/*****************************************************************************
*   Macros
*****************************************************************************/

//! How many timers per day of week, original 4 we use 8
#define RTC_TIMERS_PER_DOW    4

//! RTC high precision timers
#define RTC_TIMER_OVF 0 //
#define RTC_TIMER_RTC 7 //
#define RTC_TIMER_KB  1 // keyboard timer
#define RTC_TIMER_RFM 2
#define RTC_TIMERS 2
#define RTC_TIMER_CALC(t) ((uint8_t)((t*256L)/1000L))
// only for DEBUG build
#define TCCR2A_INIT ((0<<CS22) | (1<<CS21) |(0<<CS20))     // select precaler: 32.768 kHz / 8
//#define TCCR2A_INIT ((1<<CS22) | (1<<CS20))     // select precaler: 32.768 kHz / 128 =
                                        // => 1 sec between each overflow
//! Do we support calibrate_rco
#define	HAS_CALIBRATE_RCO     1

/*****************************************************************************
*   Typedefs
*****************************************************************************/

//! day of week
typedef enum
{
   COMPLETE_WEEK, MO, TU, WE, TH, FR, SA, SU, 
} rtc_dow_t;

//! 
typedef enum {
    temperature0=0, temperature1=1
} timermode_t;

/* rtc_t structure can be used for encryption, in this case it must be 8 byte ling and same on both sides */ 
typedef struct {
    uint8_t YY; //!< \brief Date: Year (0-255) -> 2000 - 2255
    uint8_t MM; //!< \brief Date: Month
    uint8_t DD; //!< \brief Date: Day
    uint8_t hh; //!< \brief Time: Hours
    uint8_t mm; //!< \brief Time: Minutes
    uint8_t ss; //!< \brief Time: Seconds
    uint8_t DOW;  //!< Date: Day of Week
    uint8_t pkt_cnt;
} rtc_t;


#define DEFAULT_FACTORY_HOURBAR 0x003FFF00

/*****************************************************************************
*   Prototypes
*****************************************************************************/
extern volatile uint8_t RTC_s100; //!< \brief Time: 1/100 Seconds
#ifdef RTC_TICKS
    extern uint32_t RTC_Ticks; //!< Ticks since last RTC.Init
    #define RTC_GetTicks() ((uint32_t) RTC_Ticks)          // 1s ticks from startup
#endif
extern rtc_t RTC;
void RTC_Init(void);                 // init Timer, activate 500ms IRQ
#define RTC_GetHour() ((uint8_t) RTC.hh)                 // get hour
#define RTC_GetMinute() ((uint8_t) RTC.mm)               // get minute
#define RTC_GetSecond() ((uint8_t) RTC.ss)               // get second
#define RTC_s256 TCNT2
#define RTC_GetDay() ((uint8_t) RTC.DD)                  // get day
#define RTC_GetMonth() ((uint8_t) RTC.MM)                // get month
#define RTC_GetYearYY() ((uint8_t) RTC.YY)               // get year (00-255)
#define RTC_GetYearYYYY() (2000 + (uint16_t) RTC.YY)  // get year (2000-2255) 
#define RTC_GetDayOfWeek() ((uint8_t) RTC.DOW)          // get day of week (0:monday)

void RTC_SetHour(int8_t);                     // Set hour
void RTC_SetMinute(int8_t);                   // Set minute
void RTC_SetSecond(int8_t);                   // Set second
void RTC_SetDay(int8_t);                      // Set day
void RTC_SetMonth(int8_t);                    // Set month
void RTC_SetYear(uint8_t);                     // Set year

#define RTC_SetDate(d,m,y) (RTC_SetYear(y),RTC_SetMonth(m),RTC_SetDay(d))

bool RTC_DowTimerSet(rtc_dow_t, uint8_t, uint16_t, timermode_t timermode); // set day of week timer
uint16_t RTC_DowTimerGet(rtc_dow_t dow, uint8_t slot, timermode_t *timermode);
uint8_t RTC_ActualTimerTemperatureType(bool exact);
int32_t RTC_DowTimerGetHourBar(uint8_t dow);
void RTC_AddOneSecond(void);

extern uint8_t RTC_timer_done;
extern uint8_t RTC_timer_todo;
void RTC_timer_set(uint8_t timer_id, uint8_t time);
#define RTC_timer_destroy(timer_id) (RTC_timer_todo &= ~_BV(timer_id), RTC_timer_done &= ~_BV(timer_id))

#if	HAS_CALIBRATE_RCO
void calibrate_rco(void);
#else
inline void calibrate_rco(void) {}
#endif

#endif /* RTC_H */
