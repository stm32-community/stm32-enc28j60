/*
 * net.h
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */

#ifndef NET_H
#define NET_H

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#include "main.h"

#if defined(STM32F0)
/*#	if STM32F091xC
#		include "stm32f091xc.h"
#	endif*/
#	include "stm32f0xx.h"
#	include "stm32f0xx_hal_def.h"
#	include "stm32f0xx_hal_spi.h"
#elif defined(STM32F1)
//#	include "stm32f103xb.h"
#	include "stm32f1xx.h"
#	include "stm32f1xx_hal_def.h"
#	include "stm32f1xx_hal_spi.h"
#elif defined(STM32F4)
# include "stm32f4xx.h"
# include "stm32f4xx_hal.h"
# include "stm32f4xx_hal_gpio.h"
# include "stm32f4xx_hal_def.h"
# include "stm32f4xx_hal_spi.h"
#endif


// Ethernet Header
#define ETH_HEADER_LEN    14
#define ETHTYPE_ARP_H_V   0x08
#define ETHTYPE_ARP_L_V   0x06
#define ETHTYPE_IP_H_V    0x08
#define ETHTYPE_IP_L_V    0x00
#define ETH_TYPE_H_P      12
#define ETH_TYPE_L_P      13
#define ETH_DST_MAC       0
#define ETH_SRC_MAC       6
// ARP Header
#define ETH_ARP_OPCODE_REPLY_H_V  0x0
#define ETH_ARP_OPCODE_REPLY_L_V  0x02
#define ETH_ARP_OPCODE_REQ_H_V    0x0
#define ETH_ARP_OPCODE_REQ_L_V    0x01
#define ETH_ARP_P                 0xe
#define ETH_ARP_DST_IP_P          0x26
#define ETH_ARP_OPCODE_H_P        0x14
#define ETH_ARP_OPCODE_L_P        0x15
#define ETH_ARP_SRC_MAC_P         0x16
#define ETH_ARP_SRC_IP_P          0x1c
#define ETH_ARP_DST_MAC_P         0x20
#define ETH_ARP_DST_IP_P          0x26
// IP Header
#define IP_HEADER_LEN         20
#define IP_PROTO_ICMP_V       0x01
#define IP_PROTO_TCP 		  0x06 // Protocol number for TCP
#define IP_PROTO_TCP_V        0x06
#define IP_PROTO_UDP_V        0x11
#define IP_V4_V               0x40
#define IP_HEADER_LENGTH_V    0x05
#define IP_P                  0x0E
#define IP_HEADER_VER_LEN_P   0x0E
#define IP_TOS_P              0x0F
#define IP_TOTLEN_H_P         0x10
#define IP_TOTLEN_L_P         0x11
#define IP_ID_H_P             0x12
#define IP_ID_L_P             0x13
#define IP_FLAGS_H_P          0x14
#define IP_FLAGS_L_P          0x15
#define IP_TTL_P              0x16
#define IP_PROTO_P            0x17
#define IP_CHECKSUM_H_P       0x18
#define IP_CHECKSUM_L_P       0x19
#define IP_SRC_IP_P           0x1A
#define IP_DST_IP_P           0x1E
#define IP_HEADER_LEN_VER_P   0xe
// ICMP Header
#define ICMP_TYPE_ECHOREPLY_V    0
#define ICMP_TYPE_ECHOREQUEST_V  8
#define ICMP_TYPE_P              0x22
#define ICMP_CHECKSUM_H_P        0x24
#define ICMP_CHECKSUM_L_P        0x25
#define ICMP_IDENT_H_P           0x26
#define ICMP_IDENT_L_P           0x27
#define ICMP_DATA_P              0x2a
// UDP Header
#define UDP_HEADER_LEN      8
#define UDP_SRC_PORT_H_P    0x22
#define UDP_SRC_PORT_L_P    0x23
#define UDP_DST_PORT_H_P    0x24
#define UDP_DST_PORT_L_P    0x25
#define UDP_LEN_H_P         0x26
#define UDP_LEN_L_P         0x27
#define UDP_CHECKSUM_H_P    0x28
#define UDP_CHECKSUM_L_P    0x29
#define UDP_DATA_P          0x2a
// TCP Header
#define TCP_STATE_CLOSED        0
#define TCP_STATE_SYN_SENT      1
#define TCP_STATE_SYN_RECEIVED  2
#define TCP_STATE_ESTABLISHED   3
#define TCP_STATE_FIN_WAIT_1    4
#define TCP_STATE_FIN_WAIT_2    5
#define TCP_STATE_CLOSE_WAIT    6
#define TCP_STATE_CLOSING       7
#define TCP_STATE_LAST_ACK      8
#define TCP_STATE_TIME_WAIT     9
#define TCP_FLAGS_FIN_V     0x01
#define TCP_FLAGS_SYN_V     0x02
#define TCP_FLAGS_RST_V     0x04
#define TCP_FLAGS_PUSH_V    0x08
#define TCP_FLAGS_ACK_V     0x10
#define TCP_FLAGS_SYNACK_V  0x12
#define TCP_FLAGS_PSHACK_V  0x18
#define TCP_SRC_PORT_H_P    0x22
#define TCP_SRC_PORT_L_P    0x23
#define TCP_DST_PORT_H_P    0x24
#define TCP_DST_PORT_L_P    0x25
#define TCP_SEQ_P           0x26
#define TCP_SEQ_H_P         0x26
#define TCP_SEQ_L_P         (TCP_SEQ_H_P + 1)
#define TCP_SEQACK_P        0x2a
#define TCP_SEQACK_H_P      0x2a
#define TCP_FLAGS_P         0x2f
#define TCP_WINDOWSIZE_H_P  0x30
#define TCP_WINDOWSIZE_L_P  0x31
#define TCP_CHECKSUM_H_P    0x32
#define TCP_CHECKSUM_L_P    0x33
#define TCP_URGENT_PTR_H_P  0x34
#define TCP_URGENT_PTR_L_P  0x35
#define TCP_OPTIONS_P       0x36
#define TCP_DATA_P          0x36
#define TCP_ACK_H_P         0x2C
#define TCP_ACK_L_P         (TCP_ACK_H_P + 1)
#define TCP_HEADER_LEN_PLAIN 20
#define TCP_HEADER_LEN_P    0x2e
#define TCP_WIN_SIZE        0x30
#define TCP_LEN_H_P         0x2C // Position of TCP length high byte
#define TCP_LEN_L_P         0x2D // Position of TCP length low byte
// DNS
#define DNSCLIENT_SRC_PORT_H 0xe0


//TCP personalyse ?
#define TCPCLIENT_SRC_PORT_H 11
#define WGW_INITIAL_ARP 1
#define WGW_HAVE_GW_MAC 2
#define WGW_REFRESHING 4
#define WGW_ACCEPT_ARP_REPLY 8
#define CLIENTMSS 550
#define TCP_DATA_START ((uint16_t)TCP_SRC_PORT_H_P+(buf[TCP_HEADER_LEN_P]>>4)*4)

// DNS States for access in applications
#define DNS_STATE_INIT 0
#define DNS_STATE_REQUESTED 1
#define DNS_STATE_ANSWER 2
// DHCP States for access in applications
#define DHCP_STATE_INIT 0
#define DHCP_STATE_DISCOVER 1
#define DHCP_STATE_OFFER 2
#define DHCP_STATE_REQUEST 3
#define DHCP_STATE_ACK 4
#define DHCP_STATE_OK 5
#define DHCP_STATE_RENEW 6

#define bool _Bool
#define true 1
#define false 0

#define PINGPATTERN 0x42

extern uint16_t len;
extern uint16_t plen;

// Web server port, used when implementing webserver
extern uint8_t wwwport_l; // server port
extern uint8_t wwwport_h;  // Note: never use same as TCPCLIENT_SRC_PORT_H

extern uint8_t www_fd;
extern uint8_t browsertype; // 0 = get, 1 = post
extern char *client_additionalheaderline;
extern char *client_method;  // for POST or PUT, no trailing space needed
extern char *client_urlbuf;
extern char *client_hoststr;
extern char *client_postval;
extern char *client_urlbuf_var;
extern uint8_t *bufptr; // ugly workaround for backward compatibility

// just lower byte, the upper byte is TCPCLIENT_SRC_PORT_H:
extern uint8_t tcpclient_src_port_l;
extern uint8_t tcp_fd; // a file descriptor, will be encoded into the port
extern uint8_t tcp_client_state;
// TCP client Destination port
extern uint8_t tcp_client_port_h;
extern uint8_t tcp_client_port_l;

#define MAX_TCP_CONNECTIONS 10

typedef struct {
    uint8_t state;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t remote_ip[4];
    uint16_t remote_port;
    uint16_t local_port;
} tcp_connection_t;

extern tcp_connection_t tcp_connections[MAX_TCP_CONNECTIONS];


extern int16_t delaycnt;
extern uint8_t gwip[4];
extern uint8_t gwmacaddr[6];
extern uint8_t tcpsrvip[4];
extern uint8_t macaddr[6];
extern uint8_t ipaddr[4];
extern uint16_t info_data_len;
extern uint16_t info_hdr_len;
extern uint8_t seqnum;
extern const char arpreqhdr[];
extern const char iphdr[];
extern const char ntpreqhdr[];

extern uint8_t waitgwmac;


extern void (*icmp_callback)(uint8_t *ip);


// TCP functions on packet.c
extern uint8_t send_fin;
extern uint16_t tcpstart;
extern uint16_t save_len;


void client_icmp_request(uint8_t *buf,uint8_t *destip);
void register_ping_rec_callback(void (*callback)(uint8_t *srcip));
uint8_t packetloop_icmp_checkreply(uint8_t *buf,uint8_t *ip_monitoredhost);
void make_echo_reply_from_request(uint8_t *buf,uint16_t len);
void __attribute__((weak)) pingCallback(void);

void send_wol(uint8_t *buf,uint8_t *wolmac);


#endif
