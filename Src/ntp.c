/**
 * @file packet.c
 * @brief Functions for handling network packets, including NTP requests and responses.
 *
 * This file contains functions to send NTP UDP packets and process NTP server responses. It also
 * includes the logic to update the RTC (Real-Time Clock) of the STM32 microcontroller based on the NTP
 * timestamp received from the server. The NTP implementation follows the specifications from
 * RFC 958. Additional utility functions for handling various network packets are provided.
 */

#include "ntp.h"
#include "packet.h"
#include "tcp.h"
#include "net.h"
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"
#include "websrv_help_functions.h"

// Sends an NTP UDP packet
// See http://tools.ietf.org/html/rfc958 for details
void client_ntp_request(uint8_t *buf, uint8_t *ntpip, uint8_t srcport) {
    // Fill Ethernet header
    memcpy(&buf[ETH_DST_MAC], gwmacaddr, 6);
    memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
    buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
    buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;

    // Fill IP header
    fill_buf_p(&buf[IP_P], 9, iphdr);
    buf[IP_TOTLEN_L_P] = 0x4c;         // Total length
    buf[IP_PROTO_P] = IP_PROTO_UDP_V;  // UDP protocol
    memcpy(&buf[IP_DST_IP_P], ntpip, 4);
    memcpy(&buf[IP_SRC_IP_P], ipaddr, 4);
    fill_ip_hdr_checksum(buf);

    // Fill UDP header
    buf[UDP_DST_PORT_H_P] = 0;
    buf[UDP_DST_PORT_L_P] = 0x7b;  // NTP port = 123
    buf[UDP_SRC_PORT_H_P] = 10;
    buf[UDP_SRC_PORT_L_P] = srcport;  // Lower 8 bits of source port
    buf[UDP_LEN_H_P] = 0;
    buf[UDP_LEN_L_P] = 56;  // Fixed length
    buf[UDP_CHECKSUM_H_P] = 0;  // Zero the checksum
    buf[UDP_CHECKSUM_L_P] = 0;

    // Fill NTP data
    memset(&buf[UDP_DATA_P], 0, 48);
    fill_buf_p(&buf[UDP_DATA_P], 10, ntpreqhdr);

    // Calculate and fill UDP checksum
    uint16_t ck = checksum(&buf[IP_SRC_IP_P], 16 + 48, 1);
    buf[UDP_CHECKSUM_H_P] = ck >> 8;
    buf[UDP_CHECKSUM_L_P] = ck & 0xff;

    // Send the packet
    enc28j60PacketSend(90, buf);
}

/*
 * This function processes the response from an NTP server and updates the RTC of the STM32 microcontroller.
 * It extracts the NTP timestamp from the received buffer, converts it to a UNIX timestamp, and then
 * updates the RTC with the current date and time.
 */
void client_ntp_process_answer(uint8_t *buf, uint16_t plen) {
    uint32_t unix_timestamp;

    // Extract the NTP timestamp (seconds since 1900)
    unix_timestamp = ((uint32_t)buf[0x52] << 24) | ((uint32_t)buf[0x53] << 16) | ((uint32_t)buf[0x54] << 8) | (uint32_t)buf[0x55];

    // Convert the NTP timestamp to UNIX timestamp (seconds since 1970)
    // by subtracting the seconds between 1900 and 1970.
    if (unix_timestamp > 2208988800UL) {
        unix_timestamp -= 2208988800UL;
    } else {
        // Handle the case where the NTP timestamp is before the UNIX epoch (very unlikely, but for robustness)
        unix_timestamp = 0;
    }

    // Create structures for time and date
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // Calculate the time components
    uint32_t seconds = unix_timestamp % 60;
    uint32_t minutes = (unix_timestamp / 60) % 60;
    uint32_t hours = (unix_timestamp / 3600) % 24;
    uint32_t days = unix_timestamp / 86400; // Total number of days since 1970

    // The start of 1970 was a Thursday (day 4 of the week)
    uint32_t dayOfWeek = (4 + days) % 7 + 1; // 1 = Monday, ..., 7 = Sunday

    // Determine the year
    uint32_t year = 1970;
    while (days >= 365) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            if (days < 366) break;
            days -= 366;
        } else {
            if (days < 365) break;
            days -= 365;
        }
        year++;
    }

    // Determine the month and the day of the month
    uint32_t month_lengths[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        month_lengths[1] = 29; // February has 29 days in leap years
    }

    uint32_t month = 0;
    for (month = 0; month < 12; month++) {
        if (days < month_lengths[month]) {
            break;
        }
        days -= month_lengths[month];
    }
    month++; // months are 1-12
    uint32_t dayOfMonth = days + 1; // days are 1-31

    // Set the time
    sTime.Hours = hours;
    sTime.Minutes = minutes;
    sTime.Seconds = seconds;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    // Set the date
    sDate.WeekDay = dayOfWeek;
    sDate.Month = month;
    sDate.Date = dayOfMonth;
    sDate.Year = year - 1970; // Year in the RTC structure is the year since 1970

    // Update the RTC
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    char timeStr[32]; // Adjusted size to hold formatted date and time
    sprintf(timeStr, "%02d/%02d/%04d %02d:%02d:%02d UTC",
            sDate.Date,
            sDate.Month,
            year,
            sTime.Hours,
            sTime.Minutes,
            sTime.Seconds);
    udpLog2("NTP indicates the following date and time:", timeStr);
}
