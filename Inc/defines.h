/**
 * @file defines.h
 * @brief Header file containing global definitions and macros for configuring the STM32 and ENC28J60 Ethernet module.
 *
 * This file includes configurations for SPI settings, conditional compilation flags for enabling various
 * networking clients (e.g., NTP, DNS, DHCP), and external declarations of global variables and function prototypes.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
 */

#ifndef INC_DEFINES_H_
#define INC_DEFINESE_H_

#include "main.h"

// Comment only one of this two. Depend of your SPI configuration.
// CS_only is simple (no option from SPI configuration).
// NSS is better. configuration details in enc28J60.h
//#define CS_Only
#define NSS_OutputSignal

// # CS SECTION
#ifdef CS_Only
#define ETHERNET_LED_GPIO GPIOC
#define ETHERNET_LED_PIN GPIO_PIN_13
#define ETHERNET_CS_GPIO GPIOA
#define ETHERNET_CS_PIN GPIO_PIN_4
#define ETHERNET_CS_DELAY 10 // Start with 2, then 1 and 0. 1000 is an other step. if it work try 10, could be Unstable but fast.
#endif

// # Parameters SECTION
// Comment or not depend on your need below :
#define ETHERSHIELD_DEBUG 1
#define NTP_client
#define DNS_client
#define DHCP_client
#define HOSTNAME "STM32_ENC28J60"
#define DHCP_HOSTNAME_LEN 14 // here the len of the HOSTNAME define just before
#define DHCP_HOSTNAME "STM32_ENC28J60"
#define PING_client
//#define PINGPATTERN 0x42 (element needed in net.c - is it very usefull ?
#define TCP_client
#define WWW_client //uncoment next one too
#define WWW_USER_AGENT "MyUserAgent"
#define FROMDECODE_websrv_help
#define URLENCODE_websrv_help



//extern volatile uint8_t dma_isr_called;


// Define this in defines.c file
extern uint8_t macaddrin[6];
extern uint8_t ipaddrin[4];
extern uint8_t maskin[4];
extern uint8_t gwipin[4];
extern uint8_t dhcpsvrin[4];
extern uint8_t dnssvrin[4];
extern uint8_t dnsipaddr[4];
extern uint8_t ntpip[4];
extern char domainName[];
extern uint8_t dest_ip[4];
extern uint16_t dest_port;
extern uint16_t srcport;
extern uint8_t dstport_h;
extern uint8_t dstport_l;
extern uint8_t dip[];
extern uint16_t dport;
extern uint16_t sport;

// Define the structure for command mapping
typedef void (*CommandFunction)();
typedef void (*CommandFunctionWithArg)(char*);

typedef struct {
    const char *command;
    CommandFunction function;
    CommandFunctionWithArg function_with_arg;
} CommandMapping;

// Declare the command table
extern CommandMapping commandTable[];

// Declare command functions here
void sendSTM32State();       // Function to send the STM32 state
void sendRTCDateTime();      // Function to send the RTC date and time

#endif /* INC_DEFINES_H_ */
