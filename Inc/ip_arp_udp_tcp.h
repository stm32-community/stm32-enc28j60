/*
 * ip_arp_udp_tcp.h
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */

#ifndef IP_ARP_UDP_TCP_H
#define IP_ARP_UDP_TCP_H

#include "main.h"

// -- web server functions --
// you must call this function once before you use any of the other server functions:
void init_ip_arp_udp_tcp(uint8_t *mymac,uint8_t *myip,uint16_t port);
// for a UDP server:
uint8_t eth_type_is_arp_and_my_ip(uint8_t *buf,uint16_t len);
uint8_t eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len);
uint8_t eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len);
void make_udp_reply_from_request(uint8_t *buf,char *data,uint16_t datalen,uint16_t port);

void make_arp_answer_from_request(uint8_t *buf);
void init_len_info(uint8_t *buf);

void fill_buf_p(uint8_t *buf, uint16_t len, const char *s);
void fill_ip_hdr_checksum(uint8_t *buf);
uint16_t checksum(uint8_t *buf, uint16_t len,uint8_t type);

// -- client functions --
#if defined (WWW_client) || defined (NTP_client)  || defined (UDP_client) || defined (TCP_client) || defined (PING_client)
uint8_t client_store_gw_mac(uint8_t *buf);
void client_set_gwip(uint8_t *gwipaddr);
void client_gw_arp_refresh(void);
void client_arp_whohas(uint8_t *buf,uint8_t *gwipaddr);
uint8_t client_waiting_gw(void); // 1 no GW mac yet, 0 have a gw mac

//#if defined (WWW_client) || defined (TCP_client) 
//#define client_set_wwwip client_tcp_set_serverip
// set the ip of the next tcp session that we open to a server
//#endif
#endif // WWW_client TCP_client etc

#ifdef TCP_client
void tcp_client_send_packet(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint8_t max_segment_size, 
	uint8_t clear_seqck, uint16_t next_ack_num, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip);
uint16_t tcp_get_dlength ( uint8_t *buf );

#endif // TCP_client

void send_udp_prepare(uint8_t *buf,uint16_t sport, uint8_t *dip, uint16_t dport);
void send_udp_transmit(uint8_t *buf,uint16_t datalen);
void send_udp(uint8_t *buf,char *data,uint16_t datalen,uint16_t sport, uint8_t *dip, uint16_t dport);
#endif /* IP_ARP_UDP_TCP_H */
//@}
