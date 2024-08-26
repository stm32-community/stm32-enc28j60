/*
 * RTC.h
 *
 *  Created on: Jun 3, 2024
 *      Author: dtneo
 */

#ifndef INC_RTC_H_
#define INC_RTC_H_

#include "main.h"
//#include "defines.h"


typedef struct {
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} DateTime;

DateTime getRTCDateTime();

#endif /* INC_RTC_H_ */
