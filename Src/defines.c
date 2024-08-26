/**
 * @file defines.c
 * @brief Source file containing the implementation of global variables and command functions for the STM32 and ENC28J60 Ethernet module.
 *
 * This file defines the network configuration variables such as MAC address, IP address, subnet mask,
 * gateway, and DNS server addresses. It also includes the implementation of command functions for
 * interacting with the ENC28J60 and RTC modules.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
 */


#include "defines.h"

uint8_t macaddrin[6] = {00, 00, 00, 00, 00, 01}; // MAC address of ENC28J60
uint8_t ipaddrin[4] = {192, 168, 0, 100}; // IP address, prioritize DHCP server assignment
uint8_t maskin[4] = {255, 255, 255, 0}; // Subnet mask
uint8_t gwipin[4] = {192, 168, 0, 1}; // Default gateway
uint8_t dhcpsvrin[4] = {192, 168, 0, 1}; // DHCP server
uint8_t dnssvrin[4] = {192, 168, 0, 1}; // DNS server confirmed by DHCP
uint8_t dnsipaddr[4] = {192, 168, 0, 1}; // DNS server confirmed by DHCP
uint8_t ntpip[4] = {192, 168, 0, 1}; // NTP server IP address
char domainName[] = "www.google.com";
uint8_t dest_ip[4] = {192, 168, 0, 254}; // Destination IP address, corresponding to a syslog server running on Debian (syslog-ng)
uint16_t dest_port = 10001; // Destination port
uint16_t srcport = 10002; // Source port
uint8_t dstport_h = 0x27; // High byte of port 10001
uint8_t dstport_l = 0x11; // Low byte of port 10001
uint8_t dip[] = {192, 168, 0, 254}; // Destination IP address, corresponding to a syslog server running on Debian (syslog-ng)
uint16_t dport = 514; // UDP port of the syslog server running on Debian (syslog-ng)
uint16_t sport = 8; // Source port



// Define the command table
CommandMapping commandTable[] = {
    {"sendSTM32State", sendSTM32State, NULL},
    {"sendRTCDateTime", sendRTCDateTime, NULL},
    {NULL, NULL, NULL}  // End marker
};


// Implement the command functions
void sendSTM32State() {
	//enc28j60getrev(void);
}

void sendRTCDateTime() {
	getRTCDateTime();
}


