 /*
 * RTC.c
 *
 * Created on: Jun 3, 2024
 * Author: dtneo
 *
 * This file contains the implementation for interacting with the Real-Time Clock (RTC)
 * on the STM32 microcontroller. It provides a function to obtain the current date and
 * time from the RTC and formats it into a human-readable string for logging purposes.
 */

#include "RTC.h"

// Function to obtain the current date and time from the RTC
DateTime getRTCDateTime() {
    DateTime currentDateTime;
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    // Get the current time and date
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);  // Necessary to unlock the time registers

    // Fill the DateTime structure
    currentDateTime.hours = sTime.Hours;
    currentDateTime.minutes = sTime.Minutes;
    currentDateTime.seconds = sTime.Seconds;
    currentDateTime.day = sDate.Date;
    currentDateTime.month = sDate.Month;
    currentDateTime.year = sDate.Year + 1970;  // Convert to full year

    // Format the date and time into a string
    char timeStr[32]; // Adjusted size to hold formatted date and time
    sprintf(timeStr, "%02d/%02d/%04d %02d:%02d:%02d",
            currentDateTime.day,
            currentDateTime.month,
            currentDateTime.year,
            currentDateTime.hours,
            currentDateTime.minutes,
            currentDateTime.seconds);
    udpLog2("DateTime_RTC", timeStr);

    return currentDateTime;  // Return the filled DateTime structure
}
