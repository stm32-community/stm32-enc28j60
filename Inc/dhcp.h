/**
 * @file dhcp.h
 * @brief Header file for DHCP client function prototypes and definitions.
 *
 * This file provides the function prototypes and necessary definitions for implementing DHCP client functionality.
 * It includes declarations for initializing the DHCP process, sending requests, and handling DHCP responses
 * such as DHCPOFFER and DHCPACK.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
 */


#ifndef DHCP_H
#define DHCP_H

#include "main.h"
#include "defines.h"

extern void dhcp_start(uint8_t *buf, uint8_t *macaddrDCPin, uint8_t *ipaddrin,
                uint8_t *maskin, uint8_t *gwipin, uint8_t *dhcpsvrin,
                uint8_t *dnssvrin );

extern uint8_t dhcp_state( void );

void dhcp_send(uint8_t *buf, uint8_t requestType);
uint8_t check_for_dhcp_answer(uint8_t *buf,uint16_t plen);
uint8_t have_dhcpoffer(uint8_t *buf,uint16_t plen);
uint8_t have_dhcpack(uint8_t *buf,uint16_t plen);

uint8_t allocateIPAddress(uint8_t *buf, uint16_t buffer_size, uint8_t *mymac, uint16_t myport, uint8_t *myip, uint8_t *mynetmask, uint8_t *gwip, uint8_t *dnsip, uint8_t *dhcpsvrip );

#endif /* DHCP_H */
