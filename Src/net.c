/*
 * net.C
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */

#include "net.h"

uint16_t len;
uint16_t plen;

// Web server port, used when implementing webserver
uint8_t wwwport_l=80; // server port
uint8_t wwwport_h=0;  // Note: never use same as TCPCLIENT_SRC_PORT_H

uint8_t www_fd=0;
uint8_t browsertype=0; // 0 = get, 1 = post
char *client_additionalheaderline;
char *client_method;  // for POST or PUT, no trailing space needed
char *client_urlbuf;
char *client_hoststr;
char *client_postval;
char *client_urlbuf_var;
uint8_t *bufptr=0; // ugly workaround for backward compatibility

// just lower byte, the upper byte is TCPCLIENT_SRC_PORT_H:
uint8_t tcpclient_src_port_l=1;
uint8_t tcp_fd=0; // a file descriptor, will be encoded into the port
uint8_t tcp_client_state=0;
// TCP client Destination port
uint8_t tcp_client_port_h=0;
uint8_t tcp_client_port_l=0;

int16_t delaycnt=0;
uint8_t gwip[4];
uint8_t gwmacaddr[6];
uint8_t tcpsrvip[4];
uint8_t macaddr[6];
uint8_t ipaddr[4];
uint16_t info_data_len=0;
uint16_t info_hdr_len = 0;
uint8_t seqnum=0xa; // my initial tcp sequence number
const char arpreqhdr[] ={0,1,8,0,6,4,0,1};
const char iphdr[] ={0x45,0,0,0x82,0,0,0x40,0,0x20}; // 0x82 is the total len on ip, 0x20 is ttl (time to live)
const char ntpreqhdr[] ={0xe3,0,4,0xfa,0,1,0,1,0,0};

uint8_t waitgwmac=WGW_INITIAL_ARP;

void (*icmp_callback)(uint8_t *ip) = NULL;



// TCP functions on packet.c
uint8_t send_fin = 0;
uint16_t tcpstart;
uint16_t save_len;


// Sends an ICMP echo request (ping) to the specified destination IP using the gateway MAC address.
void client_icmp_request(uint8_t *buf, uint8_t *destip) {
    // Fill Ethernet header
    memcpy(&buf[ETH_DST_MAC], gwmacaddr, 6);  // Gateway MAC address in local LAN or host MAC address
    memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
    buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
    buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;

    // Fill IP header
    fill_buf_p(&buf[IP_P], 9, iphdr);
    buf[IP_TOTLEN_L_P] = 0x82;               // Total length
    buf[IP_PROTO_P] = IP_PROTO_ICMP_V;       // ICMP protocol
    memcpy(&buf[IP_DST_IP_P], destip, 4);
    memcpy(&buf[IP_SRC_IP_P], ipaddr, 4);
    fill_ip_hdr_checksum(buf);

    // Fill ICMP header and data
    buf[ICMP_TYPE_P] = ICMP_TYPE_ECHOREQUEST_V;
    buf[ICMP_TYPE_P + 1] = 0;               // Code
    buf[ICMP_CHECKSUM_H_P] = 0;             // Zero the checksum
    buf[ICMP_CHECKSUM_L_P] = 0;
    buf[ICMP_IDENT_H_P] = 5;                // Some number for identification
    buf[ICMP_IDENT_L_P] = ipaddr[3];        // Last byte of my IP for identification
    buf[ICMP_IDENT_L_P + 1] = 0;            // Sequence number, high byte
    buf[ICMP_IDENT_L_P + 2] = 1;            // Sequence number, low byte

    // Copy the data pattern
    memset(&buf[ICMP_DATA_P], PINGPATTERN, 56);

    // Calculate checksum
    uint16_t ck = checksum(&buf[ICMP_TYPE_P], 56 + 8, 0);
    buf[ICMP_CHECKSUM_H_P] = ck >> 8;
    buf[ICMP_CHECKSUM_L_P] = ck & 0xff;

    // Send the packet
    enc28j60PacketSend(98, buf);
}

/**
 * @brief Register a callback function for receiving ping replies.
 *
 * @param callback The callback function to be called when a ping reply is received.
 */
void register_ping_rec_callback(void (*callback)(uint8_t *srcip)) {
    icmp_callback = callback;
}

/**
 * @brief Check for an ICMP ping reply packet.
 *
 * This function should be called in a loop to check if a ping reply has been received.
 *
 * @param buf The buffer containing the packet data.
 * @param ip_monitoredhost The IP address of the monitored host.
 * @return 1 if the ping reply is from the monitored host, 0 otherwise.
 */
uint8_t packetloop_icmp_checkreply(uint8_t *buf, uint8_t *ip_monitoredhost) {
    if (buf[IP_PROTO_P] == IP_PROTO_ICMP_V &&
        buf[ICMP_TYPE_P] == ICMP_TYPE_ECHOREPLY_V &&
        buf[ICMP_DATA_P] == PINGPATTERN &&
        check_ip_message_is_from(buf, ip_monitoredhost)) {
        return 1;
    }
    return 0;
}

// Make an ICMP echo reply from a received echo request
void make_echo_reply_from_request(uint8_t *buf, uint16_t len) {
    make_eth(buf);
    make_ip(buf);
    buf[ICMP_TYPE_P] = ICMP_TYPE_ECHOREPLY_V;
    if (buf[ICMP_CHECKSUM_H_P] > (0xff - 0x08)) {
        buf[ICMP_CHECKSUM_H_P + 1]++;
    }
    buf[ICMP_CHECKSUM_H_P] += 0x08;
    enc28j60PacketSend(len, buf);
}

void __attribute__((weak)) pingCallback(void) {}


// A WOL (Wake on Lan) packet is a UDP packet to the broadcast
// address and UDP port 9. The data part contains 6x FF followed by
// 16 times the mac address of the host to wake-up
void send_wol(uint8_t *buf,uint8_t *wolmac)
{
  uint16_t ck;
  //
  memset(&buf[ETH_DST_MAC], 0xff, 6);
  memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
  buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
  buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
  fill_buf_p(&buf[IP_P],9,iphdr);

  buf[IP_TOTLEN_L_P]=0x82;
  buf[IP_PROTO_P]=IP_PROTO_UDP_V;
  memcpy(&buf[IP_SRC_IP_P], ipaddr, 4);
  memset(&buf[IP_DST_IP_P], 0xff, 4);
  fill_ip_hdr_checksum(buf);
  buf[UDP_DST_PORT_H_P]=0;
  buf[UDP_DST_PORT_L_P]=0x9; // wol=normally 9
  buf[UDP_SRC_PORT_H_P]=10;
  buf[UDP_SRC_PORT_L_P]=0x42; // source port does not matter
  buf[UDP_LEN_H_P]=0;
  buf[UDP_LEN_L_P]=110; // fixed len
  // zero the checksum
  buf[UDP_CHECKSUM_H_P]=0;
  buf[UDP_CHECKSUM_L_P]=0;
  // copy the data (102 bytes):

  // first mac - 0xFFs
  memset(&buf[UDP_DATA_P], 0xff, 6);
  // next 16 macs - wolmac repeated
  // TODO: may need testing
  for (unsigned i = 1; i <= 16; i++) {
    memcpy(&buf[UDP_DATA_P + i*6], wolmac, 6);
  }

  ck=checksum(&buf[IP_SRC_IP_P], 118,1);
  buf[UDP_CHECKSUM_H_P]=ck>>8;
  buf[UDP_CHECKSUM_L_P]=ck& 0xff;

  enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+102,buf);
}
