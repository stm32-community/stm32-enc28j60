/**
 * @file dnslkup.h
 * @brief Header file for DNS lookup function prototypes and related definitions.
 *
 * This file provides the function prototypes for DNS client operations, such as sending DNS requests,
 * processing DNS responses, and resolving hostnames to IP addresses. It also includes functions
 * for setting the DNS server and checking for errors during DNS lookups.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
 */

#ifndef DNSLKUP_H
#define DNSLKUP_H

#include "main.h"
//#include "defines.h"

// look-up a hostname (you should check client_waiting_gw() before calling this function):
extern void dnslkup_request(uint8_t *buf, uint8_t *hostname);
//extern void dnslkup_request(uint8_t *buf,const prog_char *progmem_hostname);
// returns 1 if we have an answer from an DNS server and an IP
extern uint8_t dnslkup_haveanswer(void);
// get information about any error (zero means no error, otherwise see dnslkup.c)
extern uint8_t dnslkup_get_error_info(void);
// loop over this function to search for the answer of the
// DNS server.
// You call this function when enc28j60PacketReceive returned non
// zero and packetloop_icmp_tcp did return zero.
uint8_t udp_client_check_for_dns_answer(uint8_t *buf,uint16_t plen);
// returns the host IP of the name that we looked up if dnslkup_haveanswer did return 1
extern uint8_t *dnslkup_getip(void);

// set DNS server to be used for lookups.
extern void dnslkup_set_dnsip(uint8_t *dnsipaddr);

uint8_t resolveHostname(uint8_t *buf, uint16_t buffer_size, uint8_t *hostname );

#endif /* DNSLKUP_H */
