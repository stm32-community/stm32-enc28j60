/**
 * @file RTC.h
 * @brief Real-Time Clock (RTC) interaction functions for STM32 microcontroller.
 *
 * This header file provides the declaration of the function used to obtain the current
 * date and time from the RTC of the STM32 microcontroller. The function formats the
 * date and time into a human-readable string for logging purposes.
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
