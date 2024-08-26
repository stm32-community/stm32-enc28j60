/*
 * ntp.h
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */
#ifndef NTP_H
#define NTP_H

#include "main.h"

void client_ntp_request(uint8_t *buf, uint8_t *ntpip, uint8_t srcport);
void client_ntp_process_answer(uint8_t *buf, uint16_t plen);


#endif
