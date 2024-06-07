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

#include "defines.h"

#define bool _Bool
#define TRUE 1
#define FALSE 0

void __attribute__((weak)) ES_PingCallback(void);

// -- web server functions --
// you must call this function once before you use any of the other server functions:
void init_ip_arp_udp_tcp(uint8_t *mymac,uint8_t *myip,uint16_t port);
// for a UDP server:
uint8_t eth_type_is_arp_and_my_ip(uint8_t *buf,uint16_t len);
uint8_t eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len);
uint8_t eth_type_is_arp_reply(uint8_t *buf);
uint8_t eth_type_is_arp_req(uint8_t *buf);

void make_udp_reply_from_request(uint8_t *buf,char *data,uint16_t datalen,uint16_t port);
void make_echo_reply_from_request(uint8_t *buf,uint16_t len);

void make_arp_answer_from_request(uint8_t *buf);
void make_tcp_synack_from_syn(uint8_t *buf);
void init_len_info(uint8_t *buf);
uint16_t get_tcp_data_pointer(void);

void make_tcp_ack_from_any(uint8_t *buf,int16_t datlentoack,uint8_t addflags);
void make_tcp_ack_with_data(uint8_t *buf,uint16_t dlen);
void make_tcp_ack_with_data_noflags(uint8_t *buf,uint16_t dlen);

//extern void fill_buf_p(uint8_t *buf,uint16_t len, const prog_char *progmem_s);
void fill_ip_hdr_checksum(uint8_t *buf);
uint16_t checksum(uint8_t *buf, uint16_t len,uint8_t type);

// for a UDP server:
uint8_t eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len);
void make_udp_reply_from_request(uint8_t *buf,char *data,uint16_t datalen,uint16_t port);


// functions to fill the web pages with data:
//extern uint16_t fill_tcp_data_p(uint8_t *buf,uint16_t pos, const prog_char *progmem_s);
uint16_t fill_tcp_data(uint8_t *buf,uint16_t pos, const char *s);
uint16_t fill_tcp_data_len(uint8_t *buf,uint16_t pos, const char *s, uint16_t len);
// send data from the web server to the client:
void www_server_reply(uint8_t *buf,uint16_t dlen);

// -- client functions --
#if defined (WWW_client) || defined (NTP_client)  || defined (UDP_client) || defined (TCP_client) || defined (PING_client)
uint8_t client_store_gw_mac(uint8_t *buf);

void client_set_gwip(uint8_t *gwipaddr);
// do a continues refresh until found:
void client_gw_arp_refresh(void);
// do an arp request once (call this function only if enc28j60PacketReceive returned zero:
void client_arp_whohas(uint8_t *buf,uint8_t *gwipaddr);
uint8_t client_waiting_gw(void); // 1 no GW mac yet, 0 have a gw mac

uint16_t build_tcp_data(uint8_t *buf, uint16_t srcPort );
void send_tcp_data(uint8_t *buf,uint16_t dlen );
#endif          // WWW_client TCP_client etc


//#if defined (WWW_client) || defined (TCP_client) 
//#define client_set_wwwip client_tcp_set_serverip
// set the ip of the next tcp session that we open to a server
void client_tcp_set_serverip(uint8_t *ipaddr);
//#endif


#ifdef TCP_client
uint8_t client_tcp_req(uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),uint16_t (*datafill_callback)(uint8_t fd),uint16_t port);
void tcp_client_send_packet(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint8_t max_segment_size, 
	uint8_t clear_seqck, uint16_t next_ack_num, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip);
uint16_t tcp_get_dlength ( uint8_t *buf );

#endif          // TCP_client

#define HTTP_HEADER_START ((uint16_t)TCP_SRC_PORT_H_P+(buf[TCP_HEADER_LEN_P]>>4)*4)

#ifdef WWW_client

// ----- http get
void client_http_get(char *urlbuf, char *urlbuf_varpart, char *hoststr, void (*callback)(uint8_t,uint16_t,uint16_t));

// The callback is a reference to a function which must look like this:
// void browserresult_callback(uint8_t statuscode,uint16_t datapos)
// statuscode=0 means a good webpage was received, with http code 200 OK
// statuscode=1 an http error was received
// statuscode=2 means the other side in not a web server and in this case datapos is also zero
// ----- http post
// client web browser using http POST operation:
// additionalheaderline must be set to NULL if not used.
// postval is a string buffer which can only be de-allocated by the caller 
// when the post operation was really done (e.g when callback was executed).
// postval must be urlencoded.

void client_http_post(char *urlbuf, char *hoststr, char *additionalheaderline, char *method, char *postval,void (*callback)(uint8_t,uint16_t));

// The callback is a reference to a function which must look like this:
// void browserresult_callback(uint8_t statuscode,uint16_t datapos)
// statuscode=0 means a good webpage was received, with http code 200 OK
// statuscode=1 an http error was received
// statuscode=2 means the other side in not a web server and in this case datapos is also zero
#endif          // WWW_client

#ifdef NTP_client
void client_ntp_request(uint8_t *buf,uint8_t *ntpip,uint8_t srcport);
void client_ntp_process_answer(uint8_t *buf, uint16_t plen);
#endif

#ifdef UDP_client
// There are two ways of using this UDP client:
//
// 1) you call send_udp_prepare, you fill the data yourself into buf starting at buf[UDP_DATA_P], 
// you send the packet by calling send_udp_transmit
//
// 2) You just allocate a large enough buffer for you data and you call send_udp and nothing else
// needs to be done.
//
void send_udp_prepare(uint8_t *buf,uint16_t sport, uint8_t *dip, uint16_t dport);
void send_udp_transmit(uint8_t *buf,uint16_t datalen);

// send_udp sends via gwip, you must call client_set_gwip at startup, datalen must be less than 220 bytes
void send_udp(uint8_t *buf,char *data,uint16_t datalen,uint16_t sport, uint8_t *dip, uint16_t dport);
void udp_packet_process(uint8_t *buf, uint16_t plen);
#endif          // UDP_client


// you can find out who ping-ed you if you want:
void register_ping_rec_callback(void (*callback)(uint8_t *srcip));

#ifdef PING_client
void client_icmp_request(uint8_t *buf,uint8_t *destip);
// you must loop over this function to check if there was a ping reply:
uint8_t packetloop_icmp_checkreply(uint8_t *buf,uint8_t *ip_monitoredhost);
#endif // PING_client

#ifdef WOL_client
void send_wol(uint8_t *buf,uint8_t *wolmac);
#endif // WOL_client

uint8_t allocateIPAddress(uint8_t *buf, uint16_t buffer_size, uint8_t *mymac, uint16_t myport, uint8_t *myip, uint8_t *mynetmask, uint8_t *gwip, uint8_t *dnsip, uint8_t *dhcpsvrip );
uint8_t resolveHostname(uint8_t *buf, uint16_t buffer_size, uint8_t *hostname );

// return 0 to just continue in the packet loop and return the position
// of the tcp data if there is tcp data part
uint16_t packetloop_icmp_tcp(uint8_t *buf,uint16_t plen);

uint8_t nextTcpState( uint8_t *buf,uint16_t plen );
uint8_t currentTcpState( );
uint8_t tcpActiveOpen( uint8_t *buf,uint16_t plen,
        uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),
        uint16_t (*datafill_callback)(uint8_t fd),
        uint16_t port);

void tcpPassiveOpen( uint8_t *buf,uint16_t plen );
void tcpClose( uint8_t *buf,uint16_t plen );

#endif /* IP_ARP_UDP_TCP_H */
//@}
