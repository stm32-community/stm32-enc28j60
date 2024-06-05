/*
 * ENC28J60.h
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */

#ifndef DHCP_H
#define DHCP_H

#include "defines.h"

extern void dhcp_start(uint8_t *buf, uint8_t *macaddrin, uint8_t *ipaddrin,
                uint8_t *maskin, uint8_t *gwipin, uint8_t *dhcpsvrin,
                uint8_t *dnssvrin );

extern uint8_t dhcp_state( void );

void dhcp_send(uint8_t *buf, uint8_t requestType);
uint8_t check_for_dhcp_answer(uint8_t *buf,uint16_t plen);
uint8_t have_dhcpoffer(uint8_t *buf,uint16_t plen);
uint8_t have_dhcpack(uint8_t *buf,uint16_t plen);

#endif /* DHCP_H */
