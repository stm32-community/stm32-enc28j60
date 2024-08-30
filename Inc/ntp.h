/**
 * @file ntp.h
 * @brief NTP (Network Time Protocol) functions for the STM32 using ENC28J60 Ethernet controller.
 *
 * This header file provides the necessary function declarations for sending NTP requests and processing
 * NTP responses. It facilitates synchronization of the STM32 microcontroller's Real-Time Clock (RTC)
 * with a remote NTP server.
 */

#ifndef NTP_H
#define NTP_H

#include "main.h"

void client_ntp_request(uint8_t *buf, uint8_t *ntpip, uint8_t srcport);
void client_ntp_process_answer(uint8_t *buf, uint16_t plen);


#endif
