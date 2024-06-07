/*
 * ip_arp_udp_tcp.c
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */

#include "defines.h"
#include "net.h"
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"
#include "UDPCommandHandler.h"
#include "websrv_help_functions.h"

// Web server port, used when implementing webserver
static uint8_t wwwport_l=80; // server port
static uint8_t wwwport_h=0;  // Note: never use same as TCPCLIENT_SRC_PORT_H

#if defined (WWW_client) || defined (TCP_client) 

// just lower byte, the upper byte is TCPCLIENT_SRC_PORT_H:
static uint8_t tcpclient_src_port_l=1; 
static uint8_t tcp_fd=0; // a file descriptor, will be encoded into the port
static uint8_t tcp_client_state=0;
// TCP client Destination port
static uint8_t tcp_client_port_h=0;
static uint8_t tcp_client_port_l=0;
// This function will be called if we ever get a result back from the
// TCP connection to the sever:
// close_connection= your_client_tcp_result_callback(uint8_t fd, uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data){...your code}
// statuscode=0 means the buffer has valid data
static uint8_t (*client_tcp_result_callback)(uint8_t,uint8_t,uint16_t,uint16_t);
// len_of_data_filled_in=your_client_tcp_datafill_callback(uint8_t fd){...your code}
static uint16_t (*client_tcp_datafill_callback)(uint8_t);
#endif
#define TCPCLIENT_SRC_PORT_H 11
#if defined (WWW_client)
static uint8_t www_fd=0;
static uint8_t browsertype=0; // 0 = get, 1 = post
static void (*client_browser_callback)(uint8_t,uint16_t,uint16_t);
static char *client_additionalheaderline;
static char *client_method;  // for POST or PUT, no trailing space needed
static char *client_urlbuf;
static char *client_hoststr;
static char *client_postval;
static char *client_urlbuf_var;
static uint8_t *bufptr=0; // ugly workaround for backward compatibility
#endif
static void (*icmp_callback)(uint8_t *ip);
// 0=wait, 1=first req no anser, 2=have gwmac, 4=refeshing but have gw mac, 8=accept an arp reply
#define WGW_INITIAL_ARP 1
#define WGW_HAVE_GW_MAC 2
#define WGW_REFRESHING 4
#define WGW_ACCEPT_ARP_REPLY 8
static int16_t delaycnt=0;
static uint8_t gwip[4];
static uint8_t gwmacaddr[6];
static uint8_t tcpsrvip[4];
static volatile uint8_t waitgwmac=WGW_INITIAL_ARP;

uint8_t macaddr[6];
static uint8_t ipaddr[4];
static uint16_t info_data_len=0;
static uint16_t info_hdr_len = 0;
static uint8_t seqnum=0xa; // my initial tcp sequence number

#define CLIENTMSS 550
#define TCP_DATA_START ((uint16_t)TCP_SRC_PORT_H_P+(buf[TCP_HEADER_LEN_P]>>4)*4)

const char arpreqhdr[] ={0,1,8,0,6,4,0,1};
#if defined (NTP_client) ||  defined (WOL_client) || defined (UDP_client) || defined (TCP_client) || defined (PING_client)
const char iphdr[] ={0x45,0,0,0x82,0,0,0x40,0,0x20}; // 0x82 is the total len on ip, 0x20 is ttl (time to live)
#endif
#ifdef NTP_client
const char ntpreqhdr[] ={0xe3,0,4,0xfa,0,1,0,1,0,0};
#endif


#ifndef DISABLE_IP_STACK
// The checksum function calculates checksums for IP, UDP, and TCP packets.
// For IP and ICMP, the checksum is calculated over the IP header only.
// For UDP and TCP, the checksum includes a pseudo-header using the source and destination IP fields.
uint16_t checksum(uint8_t *buf, uint16_t len, uint8_t type) {
    uint32_t sum = 0;

    // Add protocol-specific values for UDP and TCP
    if (type == 1) {
        sum += IP_PROTO_UDP_V; // Protocol UDP
        sum += len - 8;        // UDP length (data + header)
    } else if (type == 2) {
        sum += IP_PROTO_TCP_V; // Protocol TCP
        sum += len - 8;        // TCP length (data + header)
    }

    // Sum up 16-bit words
    while (len > 1) {
        sum += 0xFFFF & (*buf << 8 | *(buf + 1));
        buf += 2;
        len -= 2;
    }

    // If there's a leftover byte, add it (padded with zero)
    if (len) {
        sum += (0xFF & *buf) << 8;
    }

    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Return one's complement of the sum
    return (uint16_t)(sum ^ 0xFFFF);
}
#endif

// This function initializes the web server.
// You must call this function once before you use any of the other functions.
void init_ip_arp_udp_tcp(uint8_t *mymac, uint8_t *myip, uint16_t port) {
    wwwport_h = (port >> 8) & 0xFF;
    wwwport_l = port & 0xFF;
    memcpy(ipaddr, myip, 4);
    memcpy(macaddr, mymac, 6);
}
#ifndef DISABLE_IP_STACK

// Check if an IP message is from a specified IP address
uint8_t check_ip_message_is_from(uint8_t *buf, uint8_t *ip) {
    return memcmp(&buf[IP_SRC_P], ip, 4) == 0;
}

// Check if the Ethernet frame is an ARP request for my IP address
uint8_t eth_type_is_arp_and_my_ip(uint8_t *buf, uint16_t len) {
    if (len < 41) {
        return 0;
    }
    if (buf[ETH_TYPE_H_P] != ETHTYPE_ARP_H_V || buf[ETH_TYPE_L_P] != ETHTYPE_ARP_L_V) {
        return 0;
    }
    return memcmp(&buf[ETH_ARP_DST_IP_P], ipaddr, 4) == 0;
}

// Check if the Ethernet frame is an IP packet addressed to my IP address
uint8_t eth_type_is_ip_and_my_ip(uint8_t *buf, uint16_t len) {
    // eth + ip header is 34 bytes + udp header is 8 bytes
    if (len < 42) {
        return 0;
    }
    if (buf[ETH_TYPE_H_P] != ETHTYPE_IP_H_V || buf[ETH_TYPE_L_P] != ETHTYPE_IP_L_V) {
        return 0;
    }
    // must be IP V4 and 20 byte header
    if (buf[IP_HEADER_LEN_VER_P] != 0x45) {
        return 0;
    }
    return memcmp(&buf[IP_DST_P], ipaddr, 4) == 0;
}

// Make a return Ethernet header from a received Ethernet packet
void make_eth(uint8_t *buf) {
    memcpy(&buf[ETH_DST_MAC], &buf[ETH_SRC_MAC], 6);
    memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
}

// Make a new Ethernet header for an IP packet
void make_eth_ip_new(uint8_t *buf, uint8_t *dst_mac) {
    memcpy(&buf[ETH_DST_MAC], dst_mac, 6);
    memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
    buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
    buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
}

// Fill the IP header checksum
void fill_ip_hdr_checksum(uint8_t *buf) {
    uint16_t ck;
    buf[IP_CHECKSUM_P] = 0;
    buf[IP_CHECKSUM_P + 1] = 0;
    buf[IP_FLAGS_P] = 0x40; // Don't fragment
    buf[IP_FLAGS_P + 1] = 0; // Fragment offset
    buf[IP_TTL_P] = 64; // TTL
    ck = checksum(&buf[IP_P], IP_HEADER_LEN, 0);
    buf[IP_CHECKSUM_P] = ck >> 8;
    buf[IP_CHECKSUM_P + 1] = ck & 0xff;
}

// Check if it's an ARP reply
uint8_t eth_type_is_arp_reply(uint8_t *buf) {
    return buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REPLY_L_V;
}

// Check if it's an ARP request
uint8_t eth_type_is_arp_req(uint8_t *buf) {
    return buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REQ_L_V;
}

// Make a new IP header for a TCP packet
static uint16_t ip_identifier = 1;

void make_ip_tcp_new(uint8_t *buf, uint16_t len, uint8_t *dst_ip) {
    buf[IP_P] = IP_V4_V | IP_HEADER_LENGTH_V;
    buf[IP_TOS_P] = 0x00; // TOS to default 0x00
    buf[IP_TOTLEN_H_P] = (len >> 8) & 0xff;
    buf[IP_TOTLEN_L_P] = len & 0xff;
    buf[IP_ID_H_P] = (ip_identifier >> 8) & 0xff;
    buf[IP_ID_L_P] = ip_identifier & 0xff;
    ip_identifier++;
    buf[IP_FLAGS_H_P] = 0x00; // Fragment flags
    buf[IP_FLAGS_L_P] = 0x00;
    buf[IP_TTL_P] = 128; // Time To Live
    buf[IP_PROTO_P] = IP_PROTO_TCP_V; // IP packet type to TCP
    memcpy(&buf[IP_DST_P], dst_ip, 4); // Destination IP address
    memcpy(&buf[IP_SRC_P], ipaddr, 4); // Source IP address
    fill_ip_hdr_checksum(buf);
}

// Make a return IP header from a received IP packet
void make_ip(uint8_t *buf) {
    memcpy(&buf[IP_DST_P], &buf[IP_SRC_P], 4);
    memcpy(&buf[IP_SRC_P], ipaddr, 4);
    fill_ip_hdr_checksum(buf);
}

// Swap seq and ack number and count ack number up
void step_seq(uint8_t *buf, uint16_t rel_ack_num, uint8_t cp_seq) {
    uint8_t i = 4;
    uint8_t tseq;

    // Sequence numbers: add the relative ack num to SEQACK
    while (i > 0) {
        rel_ack_num += buf[TCP_SEQ_H_P + i - 1];
        tseq = buf[TCP_SEQACK_H_P + i - 1];
        buf[TCP_SEQACK_H_P + i - 1] = rel_ack_num & 0xFF;

        if (cp_seq) {
            // Copy the acknum sent to us into the sequence number
            buf[TCP_SEQ_H_P + i - 1] = tseq;
        } else {
            buf[TCP_SEQ_H_P + i - 1] = 0; // Some preset value
        }

        rel_ack_num >>= 8;
        i--;
    }
}

// Make a return TCP header from a received TCP packet
// rel_ack_num is how much we must step the seq number received from the
// other side. We do not send more than 765 bytes of text (=data) in the TCP packet.
// No mss is included here.
//
// After calling this function you can fill in the first data byte at TCP_OPTIONS_P+4
// If cp_seq=0 then an initial sequence number is used (should be used in synack)
// otherwise it is copied from the packet we received
void make_tcphead(uint8_t *buf, uint16_t rel_ack_num, uint8_t cp_seq) {
    // Swap source and destination ports
    uint8_t temp = buf[TCP_DST_PORT_H_P];
    buf[TCP_DST_PORT_H_P] = buf[TCP_SRC_PORT_H_P];
    buf[TCP_SRC_PORT_H_P] = temp;

    temp = buf[TCP_DST_PORT_L_P];
    buf[TCP_DST_PORT_L_P] = buf[TCP_SRC_PORT_L_P];
    buf[TCP_SRC_PORT_L_P] = temp;

    // Update sequence and acknowledgment numbers
    step_seq(buf, rel_ack_num, cp_seq);

    // Zero the checksum
    buf[TCP_CHECKSUM_H_P] = 0;
    buf[TCP_CHECKSUM_L_P] = 0;

    // No options, header length is 20 bytes (0x50)
    buf[TCP_HEADER_LEN_P] = 0x50;
}

// Make an ARP reply from a received ARP request
void make_arp_answer_from_request(uint8_t *buf) {
    make_eth(buf);
    buf[ETH_ARP_OPCODE_H_P] = ETH_ARP_OPCODE_REPLY_H_V;
    buf[ETH_ARP_OPCODE_L_P] = ETH_ARP_OPCODE_REPLY_L_V;
    memcpy(&buf[ETH_ARP_DST_MAC_P], &buf[ETH_ARP_SRC_MAC_P], 6);
    memcpy(&buf[ETH_ARP_SRC_MAC_P], macaddr, 6);
    memcpy(&buf[ETH_ARP_DST_IP_P], &buf[ETH_ARP_SRC_IP_P], 4);
    memcpy(&buf[ETH_ARP_SRC_IP_P], ipaddr, 4);
    enc28j60PacketSend(42, buf);
}

// Make an ICMP echo reply from a received echo request
void make_echo_reply_from_request(uint8_t *buf, uint16_t len) {
    make_eth(buf);
    make_ip(buf);
    buf[ICMP_TYPE_P] = ICMP_TYPE_ECHOREPLY_V;
    if (buf[ICMP_CHECKSUM_P] > (0xff - 0x08)) {
        buf[ICMP_CHECKSUM_P + 1]++;
    }
    buf[ICMP_CHECKSUM_P] += 0x08;
    enc28j60PacketSend(len, buf);
}

// Make a UDP reply from a received UDP request, sending a max of 220 bytes of data
void make_udp_reply_from_request(uint8_t *buf, char *data, uint16_t datalen, uint16_t port) {
    if (datalen > 220) {
        datalen = 220;
    }

    make_eth(buf);

    // Total length field in the IP header
    uint16_t total_len = IP_HEADER_LEN + UDP_HEADER_LEN + datalen;
    buf[IP_TOTLEN_H_P] = total_len >> 8;
    buf[IP_TOTLEN_L_P] = total_len & 0xff;

    make_ip(buf);

    // Swap source and destination ports
    buf[UDP_DST_PORT_H_P] = buf[UDP_SRC_PORT_H_P];
    buf[UDP_DST_PORT_L_P] = buf[UDP_SRC_PORT_L_P];
    buf[UDP_SRC_PORT_H_P] = port >> 8;
    buf[UDP_SRC_PORT_L_P] = port & 0xff;

    // Calculate UDP length
    uint16_t udp_len = UDP_HEADER_LEN + datalen;
    buf[UDP_LEN_H_P] = udp_len >> 8;
    buf[UDP_LEN_L_P] = udp_len & 0xff;

    // Zero the checksum
    buf[UDP_CHECKSUM_H_P] = 0;
    buf[UDP_CHECKSUM_L_P] = 0;

    // Copy data
    memcpy(&buf[UDP_DATA_P], data, datalen);

    // Calculate checksum
    uint16_t ck = checksum(&buf[IP_SRC_P], 16 + datalen, 1);
    buf[UDP_CHECKSUM_H_P] = ck >> 8;
    buf[UDP_CHECKSUM_L_P] = ck & 0xff;

    enc28j60PacketSend(UDP_HEADER_LEN + IP_HEADER_LEN + ETH_HEADER_LEN + datalen, buf);
}

// Make a TCP SYN-ACK response from a received SYN packet
void make_tcp_synack_from_syn(uint8_t *buf) {
    uint16_t ck;
    make_eth(buf);

    // Total length field in the IP header
    buf[IP_TOTLEN_H_P] = 0;
    buf[IP_TOTLEN_L_P] = IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + 4;

    make_ip(buf);

    buf[TCP_FLAGS_P] = TCP_FLAGS_SYNACK_V;
    make_tcphead(buf, 1, 0);

    // Initialize sequence number
    buf[TCP_SEQ_H_P] = 0;
    buf[TCP_SEQ_H_P + 1] = 0;
    buf[TCP_SEQ_H_P + 2] = seqnum;
    buf[TCP_SEQ_H_P + 3] = 0;

    // Increment the sequence number
    seqnum += 3;

    // MSS option field
    buf[TCP_OPTIONS_P] = 2;
    buf[TCP_OPTIONS_P + 1] = 4;
    buf[TCP_OPTIONS_P + 2] = 0x05;
    buf[TCP_OPTIONS_P + 3] = 0x00;

    // TCP header length
    buf[TCP_HEADER_LEN_P] = 0x60;

    // Window size
    buf[TCP_WIN_SIZE] = 0x05;
    buf[TCP_WIN_SIZE + 1] = 0x78;

    // Calculate the checksum
    ck = checksum(&buf[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN + 4, 2);
    buf[TCP_CHECKSUM_H_P] = ck >> 8;
    buf[TCP_CHECKSUM_L_P] = ck & 0xff;

    // Send the packet
    enc28j60PacketSend(IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + 4 + ETH_HEADER_LEN, buf);
}

// Calculate the length of TCP data and store the result in static variables
uint16_t get_tcp_data_len(uint8_t *buf) {
    int16_t total_len = ((int16_t)buf[IP_TOTLEN_H_P] << 8) | (buf[IP_TOTLEN_L_P] & 0xff);
    int16_t header_len = IP_HEADER_LEN + ((buf[TCP_HEADER_LEN_P] >> 4) * 4); // Calculate header length in bytes
    int16_t data_len = total_len - header_len;

    return (data_len > 0) ? (uint16_t)data_len : 0;
}

// Get a pointer to the start of TCP data in buf
// Returns 0 if there is no data
// You must call init_len_info once before calling this function
uint16_t get_tcp_data_pointer(void) {
    if (info_data_len) {
        return ((uint16_t)TCP_SRC_PORT_H_P + info_hdr_len);
    } else {
        return 0;
    }
}

// Do some basic length calculations and store the result in static variables
void init_len_info(uint8_t *buf) {
    info_data_len = (((int16_t)buf[IP_TOTLEN_H_P]) << 8) | (buf[IP_TOTLEN_L_P] & 0xff);
    info_data_len -= IP_HEADER_LEN;
    info_hdr_len = (buf[TCP_HEADER_LEN_P] >> 4) * 4; // Generate length in bytes
    info_data_len -= info_hdr_len;
    if (info_data_len <= 0) {
        info_data_len = 0;
    }
}

// Fill a binary string of len data into the TCP packet
uint16_t fill_tcp_data_len(uint8_t *buf, uint16_t pos, const char *s, uint16_t len) {
    // Fill in TCP data at position pos
    memcpy(&buf[TCP_CHECKSUM_L_P + 3 + pos], s, len);
    return pos + len;
}

// Fill in TCP data at position pos. pos=0 means start of TCP data.
// Returns the position at which the string after this string could be filled.
uint16_t fill_tcp_data(uint8_t *buf, uint16_t pos, const char *s) {
    return fill_tcp_data_len(buf, pos, s, strlen(s));
}

// Make just an ACK packet with no TCP data inside
// This will modify the ETH/IP/TCP header
void make_tcp_ack_from_any(uint8_t *buf, int16_t datlentoack, uint8_t addflags) {
    uint16_t len;

    make_eth(buf);

    // Fill the header
    buf[TCP_FLAGS_P] = TCP_FLAGS_ACK_V | addflags;

    if (addflags == TCP_FLAGS_RST_V) {
        make_tcphead(buf, datlentoack, 1);
    } else {
        if (datlentoack == 0) {
            // If there is no data, then we must still acknowledge one packet
            datlentoack = 1;
        }
        make_tcphead(buf, datlentoack, 1); // No options
    }

    // Total length field in the IP header must be set
    len = IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN;
    buf[IP_TOTLEN_H_P] = len >> 8;
    buf[IP_TOTLEN_L_P] = len & 0xff;

    make_ip(buf);

    // Use a low window size, otherwise we have to have timers and cannot just react on every packet
    buf[TCP_WIN_SIZE] = 0x4;  // 1024 = 0x400
    buf[TCP_WIN_SIZE + 1] = 0x0;

    // Calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
    len = checksum(&buf[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN, 2);
    buf[TCP_CHECKSUM_H_P] = len >> 8;
    buf[TCP_CHECKSUM_L_P] = len & 0xff;

    enc28j60PacketSend(IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + ETH_HEADER_LEN, buf);
}

// dlen is the amount of TCP data (HTTP data) we send in this packet
// You can use this function only immediately after make_tcp_ack_from_any
// This is because this function will NOT modify the eth/ip/tcp header except for
// length and checksum
// You must set TCP_FLAGS before calling this
void make_tcp_ack_with_data_noflags(uint8_t *buf, uint16_t dlen) {
    uint16_t total_len = IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + dlen;

    // Set the total length field in the IP header
    buf[IP_TOTLEN_H_P] = total_len >> 8;
    buf[IP_TOTLEN_L_P] = total_len & 0xff;

    // Fill in the IP header checksum
    fill_ip_hdr_checksum(buf);

    // Zero the TCP checksum field
    buf[TCP_CHECKSUM_H_P] = 0;
    buf[TCP_CHECKSUM_L_P] = 0;

    // Calculate the checksum, len = 8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
    uint16_t checksum_len = checksum(&buf[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN + dlen, 2);
    buf[TCP_CHECKSUM_H_P] = checksum_len >> 8;
    buf[TCP_CHECKSUM_L_P] = checksum_len & 0xff;

    // Send the packet
    enc28j60PacketSend(total_len + ETH_HEADER_LEN, buf);
}

// You must have called init_len_info at some time before calling this function
// dlen is the amount of TCP data (HTTP data) we send in this packet
// You can use this function only immediately after make_tcp_ack_from_any
// This is because this function will NOT modify the eth/ip/tcp header except for
// length and checksum
void make_tcp_ack_with_data(uint8_t *buf, uint16_t dlen) {
    uint16_t total_len = IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + dlen;

    // Fill the header with appropriate flags
    buf[TCP_FLAGS_P] = TCP_FLAGS_ACK_V | TCP_FLAGS_PUSH_V | TCP_FLAGS_FIN_V;

    // Set the total length field in the IP header
    buf[IP_TOTLEN_H_P] = total_len >> 8;
    buf[IP_TOTLEN_L_P] = total_len & 0xff;

    // Fill in the IP header checksum
    fill_ip_hdr_checksum(buf);

    // Zero the TCP checksum field
    buf[TCP_CHECKSUM_H_P] = 0;
    buf[TCP_CHECKSUM_L_P] = 0;

    // Calculate the checksum, len = 8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
    uint16_t checksum_len = checksum(&buf[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN + dlen, 2);
    buf[TCP_CHECKSUM_H_P] = checksum_len >> 8;
    buf[TCP_CHECKSUM_L_P] = checksum_len & 0xff;

    // Send the packet
    enc28j60PacketSend(total_len + ETH_HEADER_LEN, buf);
}

// You must have initialized info_data_len at some time before calling this function
// This info_data_len initialization is done automatically if you call
// packetloop_icmp_tcp(buf, enc28j60PacketReceive(BUFFER_SIZE, buf));
// and test the return value for non-zero.
//
// dlen is the amount of TCP data (HTTP data) we send in this packet
// You can use this function only immediately after make_tcp_ack_from_any
// This is because this function will NOT modify the eth/ip/tcp header except for
// length and checksum
void www_server_reply(uint8_t *buf, uint16_t dlen) {
    // Send ACK for HTTP GET
    make_tcp_ack_from_any(buf, info_data_len, 0);

    // Fill the header with appropriate flags
    buf[TCP_FLAGS_P] = TCP_FLAGS_ACK_V | TCP_FLAGS_PUSH_V | TCP_FLAGS_FIN_V;

    // Send data
    make_tcp_ack_with_data_noflags(buf, dlen);
}

#if defined(NTP_client) || defined(WOL_client) || defined(UDP_client) || defined(TCP_client) || defined(PING_client)
// Fills the buffer with a string of specified length
void fill_buf_p(uint8_t *buf, uint16_t len, const char *s) {
    memcpy(buf, s, len);
}
#endif

#ifdef PING_client
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
    memcpy(&buf[IP_DST_P], destip, 4);
    memcpy(&buf[IP_SRC_P], ipaddr, 4);
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
#endif // PING_client

void __attribute__((weak)) ES_PingCallback(void) {}

#ifdef NTP_client
// Sends an NTP UDP packet
// See http://tools.ietf.org/html/rfc958 for details
void client_ntp_request(uint8_t *buf, uint8_t *ntpip, uint8_t srcport) {
    // Fill Ethernet header
    memcpy(&buf[ETH_DST_MAC], gwmacaddr, 6);
    memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
    buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
    buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;

    // Fill IP header
    fill_buf_p(&buf[IP_P], 9, iphdr);
    buf[IP_TOTLEN_L_P] = 0x4c;         // Total length
    buf[IP_PROTO_P] = IP_PROTO_UDP_V;  // UDP protocol
    memcpy(&buf[IP_DST_P], ntpip, 4);
    memcpy(&buf[IP_SRC_P], ipaddr, 4);
    fill_ip_hdr_checksum(buf);

    // Fill UDP header
    buf[UDP_DST_PORT_H_P] = 0;
    buf[UDP_DST_PORT_L_P] = 0x7b;  // NTP port = 123
    buf[UDP_SRC_PORT_H_P] = 10;
    buf[UDP_SRC_PORT_L_P] = srcport;  // Lower 8 bits of source port
    buf[UDP_LEN_H_P] = 0;
    buf[UDP_LEN_L_P] = 56;  // Fixed length
    buf[UDP_CHECKSUM_H_P] = 0;  // Zero the checksum
    buf[UDP_CHECKSUM_L_P] = 0;

    // Fill NTP data
    memset(&buf[UDP_DATA_P], 0, 48);
    fill_buf_p(&buf[UDP_DATA_P], 10, ntpreqhdr);

    // Calculate and fill UDP checksum
    uint16_t ck = checksum(&buf[IP_SRC_P], 16 + 48, 1);
    buf[UDP_CHECKSUM_H_P] = ck >> 8;
    buf[UDP_CHECKSUM_L_P] = ck & 0xff;

    // Send the packet
    enc28j60PacketSend(90, buf);
}

/*
 * This function processes the response from an NTP server and updates the RTC of the STM32 microcontroller.
 * It extracts the NTP timestamp from the received buffer, converts it to a UNIX timestamp, and then
 * updates the RTC with the current date and time.
 */
void client_ntp_process_answer(uint8_t *buf, uint16_t plen) {
    uint32_t unix_timestamp;

    // Extract the NTP timestamp (seconds since 1900)
    unix_timestamp = ((uint32_t)buf[0x52] << 24) | ((uint32_t)buf[0x53] << 16) | ((uint32_t)buf[0x54] << 8) | (uint32_t)buf[0x55];

    // Convert the NTP timestamp to UNIX timestamp (seconds since 1970)
    // by subtracting the seconds between 1900 and 1970.
    if (unix_timestamp > 2208988800UL) {
        unix_timestamp -= 2208988800UL;
    } else {
        // Handle the case where the NTP timestamp is before the UNIX epoch (very unlikely, but for robustness)
        unix_timestamp = 0;
    }

    // Create structures for time and date
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // Calculate the time components
    uint32_t seconds = unix_timestamp % 60;
    uint32_t minutes = (unix_timestamp / 60) % 60;
    uint32_t hours = (unix_timestamp / 3600) % 24;
    uint32_t days = unix_timestamp / 86400; // Total number of days since 1970

    // The start of 1970 was a Thursday (day 4 of the week)
    uint32_t dayOfWeek = (4 + days) % 7 + 1; // 1 = Monday, ..., 7 = Sunday

    // Determine the year
    uint32_t year = 1970;
    while (days >= 365) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            if (days < 366) break;
            days -= 366;
        } else {
            if (days < 365) break;
            days -= 365;
        }
        year++;
    }

    // Determine the month and the day of the month
    uint32_t month_lengths[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        month_lengths[1] = 29; // February has 29 days in leap years
    }

    uint32_t month = 0;
    for (month = 0; month < 12; month++) {
        if (days < month_lengths[month]) {
            break;
        }
        days -= month_lengths[month];
    }
    month++; // months are 1-12
    uint32_t dayOfMonth = days + 1; // days are 1-31

    // Set the time
    sTime.Hours = hours;
    sTime.Minutes = minutes;
    sTime.Seconds = seconds;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    // Set the date
    sDate.WeekDay = dayOfWeek;
    sDate.Month = month;
    sDate.Date = dayOfMonth;
    sDate.Year = year - 1970; // Year in the RTC structure is the year since 1970

    // Update the RTC
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    char timeStr[32]; // Adjusted size to hold formatted date and time
    sprintf(timeStr, "%02d/%02d/%04d %02d:%02d:%02d UTC",
            sDate.Date,
            sDate.Month,
            year,
            sTime.Hours,
            sTime.Minutes,
            sTime.Seconds);
    udpLog2("NTP indicates the following date and time:", timeStr);
}
#endif // NTP_client

#ifdef UDP_client
// Prepare a spontaneous UDP packet to a server
void send_udp_prepare(uint8_t *buf, uint16_t sport, uint8_t *dip, uint16_t dport) {
    memcpy(&buf[ETH_DST_MAC], gwmacaddr, 6);
    memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
    buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
    buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
    fill_buf_p(&buf[IP_P], 9, iphdr);

    buf[IP_TOTLEN_H_P] = 0;
    buf[IP_PROTO_P] = IP_PROTO_UDP_V;
    memcpy(&buf[IP_DST_P], dip, 4);
    memcpy(&buf[IP_SRC_P], ipaddr, 4);

    buf[UDP_DST_PORT_H_P] = (dport >> 8);
    buf[UDP_DST_PORT_L_P] = dport & 0xff;
    buf[UDP_SRC_PORT_H_P] = (sport >> 8);
    buf[UDP_SRC_PORT_L_P] = sport & 0xff;
    buf[UDP_LEN_H_P] = 0;
    buf[UDP_CHECKSUM_H_P] = 0;
    buf[UDP_CHECKSUM_L_P] = 0;
}

// Transmit a prepared UDP packet
void send_udp_transmit(uint8_t *buf, uint16_t datalen) {
    buf[IP_TOTLEN_H_P] = (IP_HEADER_LEN + UDP_HEADER_LEN + datalen) >> 8;
    buf[IP_TOTLEN_L_P] = (IP_HEADER_LEN + UDP_HEADER_LEN + datalen) & 0xff;
    fill_ip_hdr_checksum(buf);

    buf[UDP_LEN_H_P] = (UDP_HEADER_LEN + datalen) >> 8;
    buf[UDP_LEN_L_P] = (UDP_HEADER_LEN + datalen) & 0xff;

    uint16_t ck = checksum(&buf[IP_SRC_P], 16 + datalen, 1);
    buf[UDP_CHECKSUM_H_P] = ck >> 8;
    buf[UDP_CHECKSUM_L_P] = ck & 0xff;

    enc28j60PacketSend(UDP_HEADER_LEN + IP_HEADER_LEN + ETH_HEADER_LEN + datalen, buf);
}

// Send a UDP packet
void send_udp(uint8_t *buf, char *data, uint16_t datalen, uint16_t sport, uint8_t *dip, uint16_t dport) {
    send_udp_prepare(buf, sport, dip, dport);
    memcpy(&buf[UDP_DATA_P], data, datalen);
    send_udp_transmit(buf, datalen);
}

// Process UDP packets
void udp_packet_process(uint8_t *buf, uint16_t plen) {
    uint16_t data_pos = UDP_DATA_P;
    uint8_t *data = &buf[data_pos];
    uint16_t data_len = plen - data_pos;

    char msg[data_len + 1];
    memcpy(msg, data, data_len);
    msg[data_len] = '\0';

    for (CommandMapping *cmd = commandTable; cmd->command != NULL; cmd++) {
        if (strncmp(msg, cmd->command, strlen(cmd->command)) == 0) {
            cmd->function();
            return;
        }
    }

    udpLog2("Error", "Unexpected UDP message");
}
#endif // UDP_client

#ifdef WOL_client
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
  memcpy(&buf[IP_SRC_P], ipaddr, 4);
  memset(&buf[IP_DST_P], 0xff, 4);
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

  ck=checksum(&buf[IP_SRC_P], 118,1);
  buf[UDP_CHECKSUM_H_P]=ck>>8;
  buf[UDP_CHECKSUM_L_P]=ck& 0xff;

  enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+102,buf);
}
#endif // WOL_client

#if defined (NTP_client) || defined (UDP_client) || defined (TCP_client) || defined (PING_client)
// Make an ARP request
void client_arp_whohas(uint8_t *buf, uint8_t *ip_we_search) {
    // Prepare Ethernet header
    memset(&buf[ETH_DST_MAC], 0xFF, 6);
    memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
    buf[ETH_TYPE_H_P] = ETHTYPE_ARP_H_V;
    buf[ETH_TYPE_L_P] = ETHTYPE_ARP_L_V;

    // Prepare ARP request header
    fill_buf_p(&buf[ETH_ARP_P], 8, arpreqhdr);
    memcpy(&buf[ETH_ARP_SRC_MAC_P], macaddr, 6);
    memset(&buf[ETH_ARP_DST_MAC_P], 0, 6);
    memcpy(&buf[ETH_ARP_DST_IP_P], ip_we_search, 4);
    memcpy(&buf[ETH_ARP_SRC_IP_P], ipaddr, 4);

    // Set the flag to accept ARP reply
    waitgwmac |= WGW_ACCEPT_ARP_REPLY;

    // Send the packet
    enc28j60PacketSend(0x2a, buf); // 0x2a = 42 = length of the packet
}

// Check if the client is waiting for the gateway MAC address
uint8_t client_waiting_gw(void) {
    return (waitgwmac & WGW_HAVE_GW_MAC) ? 0 : 1;
}

// Store the MAC address from an ARP reply
uint8_t client_store_gw_mac(uint8_t *buf) {
    if (memcmp(&buf[ETH_ARP_SRC_IP_P], gwip, 4) == 0) {
        memcpy(gwmacaddr, &buf[ETH_ARP_SRC_MAC_P], 6);
        return 1;
    }
    return 0;
}

// Refresh the gateway ARP entry
void client_gw_arp_refresh(void) {
    if (waitgwmac & WGW_HAVE_GW_MAC) {
        waitgwmac |= WGW_REFRESHING;
    }
}

//void client_set_wwwip(uint8_t *wwwipaddr) {
//    memcpy(wwwip, wwwipaddr, 4);
//}

void client_set_gwip(uint8_t *gwipaddr)
{
    waitgwmac = WGW_INITIAL_ARP; // causes an arp request in the packet loop
    memcpy(gwip, gwipaddr, 4);
}
#endif // defined (NTP_client) || defined (UDP_client) || defined (TCP_client) || defined (PING_client)

void client_tcp_set_serverip(uint8_t *ipaddr)
{
    memcpy(tcpsrvip, ipaddr, 4);
}

#if defined (TCP_client)
// Make a tcp syn packet
void client_syn(uint8_t *buf, uint8_t srcport, uint8_t dstport_h, uint8_t dstport_l)
{
    // -- make the main part of the eth/IP/tcp header:
    memcpy(&buf[ETH_DST_MAC], gwmacaddr, 6);
    memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
    buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
    buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
    fill_buf_p(&buf[IP_P], 9, iphdr);

    buf[IP_TOTLEN_L_P] = 44; // good for syn
    buf[IP_PROTO_P] = IP_PROTO_TCP_V;
    memcpy(&buf[IP_DST_P], tcpsrvip, 4);
    memcpy(&buf[IP_SRC_P], ipaddr, 4);
    fill_ip_hdr_checksum(buf);

    buf[TCP_DST_PORT_H_P] = dstport_h;
    buf[TCP_DST_PORT_L_P] = dstport_l;
    buf[TCP_SRC_PORT_H_P] = TCPCLIENT_SRC_PORT_H;
    buf[TCP_SRC_PORT_L_P] = srcport; // lower 8 bit of src port
    // zero out sequence number and acknowledgement number
    memset(&buf[TCP_SEQ_H_P], 0, 8);

    // -- header ready
    // put initial seq number
    buf[TCP_SEQ_H_P+2] = seqnum;
    seqnum += 3; // step the initial seq num by something we will not use during this tcp session

    buf[TCP_HEADER_LEN_P] = 0x60; // 0x60=24 len: (0x60>>4) * 4
    buf[TCP_FLAGS_P] = TCP_FLAGS_SYN_V;

    // use a low window size otherwise we have to have timers and can not just react on every packet.
    buf[TCP_WIN_SIZE] = 0x4; // 1024=0x400, 768 = 0x300, initial window
    buf[TCP_WIN_SIZE+1] = 0x0;

    // zero the checksum
    buf[TCP_CHECKSUM_H_P] = 0;
    buf[TCP_CHECKSUM_L_P] = 0;

    // urgent pointer
    buf[TCP_CHECKSUM_L_P+1] = 0;
    buf[TCP_CHECKSUM_L_P+2] = 0;

    // MSS=768, must be more than 50% of the window size we use
    buf[TCP_OPTIONS_P] = 2;
    buf[TCP_OPTIONS_P+1] = 4;
    buf[TCP_OPTIONS_P+2] = (CLIENTMSS >> 8);
    buf[TCP_OPTIONS_P+3] = CLIENTMSS & 0xff;

    uint16_t ck = checksum(&buf[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN + 4, 2);
    buf[TCP_CHECKSUM_H_P] = ck >> 8;
    buf[TCP_CHECKSUM_L_P] = ck & 0xff;

    // 4 is the tcp mss option:
    enc28j60PacketSend(IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + ETH_HEADER_LEN + 4, buf);

#if ETHERSHIELD_DEBUG
    ethershieldDebug("Sent TCP Syn\n");
#endif
}

uint16_t build_tcp_data(uint8_t *buf, uint16_t srcPort)
{
    // -- make the main part of the eth/IP/tcp header:
    memcpy(&buf[ETH_DST_MAC], gwmacaddr, 6);
    memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
    buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
    buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
    fill_buf_p(&buf[IP_P], 9, iphdr);

    buf[IP_TOTLEN_L_P] = 40;
    buf[IP_PROTO_P] = IP_PROTO_TCP_V;
    memcpy(&buf[IP_DST_P], tcpsrvip, 4);
    memcpy(&buf[IP_SRC_P], ipaddr, 4);
    fill_ip_hdr_checksum(buf);

    buf[TCP_DST_PORT_H_P] = tcp_client_port_h;
    buf[TCP_DST_PORT_L_P] = tcp_client_port_l;
    buf[TCP_SRC_PORT_H_P] = srcPort >> 8;
    buf[TCP_SRC_PORT_L_P] = srcPort & 0xff;

    // zero out sequence number and acknowledgement number
    memset(&buf[TCP_SEQ_H_P], 0, 8);

    // put initial seq number
    buf[TCP_SEQ_H_P + 2] = seqnum;
    seqnum += 3; // step the initial seq num by something we will not use during this tcp session

    buf[TCP_HEADER_LEN_P] = 0x60; // 0x60=24 len: (0x60>>4) * 4
    buf[TCP_FLAGS_P] = TCP_FLAGS_PUSH_V;

    buf[TCP_WIN_SIZE] = 0x4; // 1024=0x400, 768 = 0x300, initial window
    buf[TCP_WIN_SIZE + 1] = 0x0;

    // zero the checksum
    buf[TCP_CHECKSUM_H_P] = 0;
    buf[TCP_CHECKSUM_L_P] = 0;

    // urgent pointer
    buf[TCP_CHECKSUM_L_P + 1] = 0;
    buf[TCP_CHECKSUM_L_P + 2] = 0;

    // MSS=768, must be more than 50% of the window size we use
    buf[TCP_OPTIONS_P] = 2;
    buf[TCP_OPTIONS_P + 1] = 4;
    buf[TCP_OPTIONS_P + 2] = CLIENTMSS >> 8;
    buf[TCP_OPTIONS_P + 3] = CLIENTMSS & 0xff;

    uint16_t ck = checksum(&buf[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN + 4, 2);
    buf[TCP_CHECKSUM_H_P] = ck >> 8;
    buf[TCP_CHECKSUM_L_P] = ck & 0xff;

    return IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + ETH_HEADER_LEN + 4;
}

void send_tcp_data(uint8_t *buf, uint16_t dlen)
{
    // fill the header:
    buf[TCP_FLAGS_P] = TCP_FLAGS_PUSH_V;

    // total length field in the IP header must be set:
    uint16_t total_len = IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + dlen;
    buf[IP_TOTLEN_H_P] = total_len >> 8;
    buf[IP_TOTLEN_L_P] = total_len & 0xff;

    fill_ip_hdr_checksum(buf);

    // zero the checksum
    buf[TCP_CHECKSUM_H_P] = 0;
    buf[TCP_CHECKSUM_L_P] = 0;

    // calculate the checksum
    uint16_t ck = checksum(&buf[IP_SRC_P], 8 + TCP_HEADER_LEN_PLAIN + dlen, 2);
    buf[TCP_CHECKSUM_H_P] = ck >> 8;
    buf[TCP_CHECKSUM_L_P] = ck & 0xff;

    enc28j60PacketSend(total_len + ETH_HEADER_LEN, buf);
}

/**
 * @brief Requests a TCP connection to a server.
 *
 * This function sets up the necessary callback functions for handling the TCP request and response.
 *
 * @param result_callback       Pointer to the callback function for processing the server response.
 * @param datafill_callback     Pointer to the callback function for filling the request data.
 * @param port                  The port number of the server to connect to.
 * @return                      A file descriptor for the TCP session.
 */
uint8_t client_tcp_req(
    uint8_t (*result_callback)(uint8_t fd, uint8_t statuscode, uint16_t data_start_pos_in_buf, uint16_t len_of_data),
    uint16_t (*datafill_callback)(uint8_t fd),
    uint16_t port)
{
    client_tcp_result_callback = result_callback;
    client_tcp_datafill_callback = datafill_callback;
    tcp_client_port_h = (port >> 8) & 0xff;
    tcp_client_port_l = port & 0xff;
    tcp_client_state = 1;

    if (++tcp_fd > 7) {
        tcp_fd = 0;
    }

    return tcp_fd;
}
#endif //  TCP_client

#if defined (WWW_client)
/**
 * @brief Internal data fill callback for the WWW client.
 *
 * This function fills the buffer with the appropriate HTTP GET or POST request.
 *
 * @param fd The file descriptor.
 * @return The length of the data filled.
 */
uint16_t www_client_internal_datafill_callback(uint8_t fd) {
    char strbuf[6];  // Adjusted to ensure it can hold up to 5 digits and the null terminator
    uint16_t len = 0;

    if (fd != www_fd) {
        return 0;
    }

    if (browsertype == 0) {
        // GET Request
        len = fill_tcp_data(bufptr, len, "GET ");
        len = fill_tcp_data(bufptr, len, client_urlbuf);
        if (client_urlbuf_var) {
            len = fill_tcp_data(bufptr, len, client_urlbuf_var);
        }
        len = fill_tcp_data(bufptr, len, " HTTP/1.1\r\nHost: ");
        len = fill_tcp_data(bufptr, len, client_hoststr);
        len = fill_tcp_data(bufptr, len, "\r\nUser-Agent: " WWW_USER_AGENT "\r\nAccept: text/html\r\nConnection: close\r\n\r\n");
    } else {
        // POST Request
        len = fill_tcp_data(bufptr, len, client_method ? client_method : "POST ");
        len = fill_tcp_data(bufptr, len, client_urlbuf);
        len = fill_tcp_data(bufptr, len, " HTTP/1.1\r\nHost: ");
        len = fill_tcp_data(bufptr, len, client_hoststr);

        if (client_additionalheaderline) {
            len = fill_tcp_data(bufptr, len, "\r\n");
            len = fill_tcp_data(bufptr, len, client_additionalheaderline);
        }

        len = fill_tcp_data(bufptr, len, "\r\nUser-Agent: " WWW_USER_AGENT "\r\nAccept: */*\r\nConnection: close\r\n");
        len = fill_tcp_data(bufptr, len, "Content-Length: ");
        snprintf(strbuf, sizeof(strbuf), "%zu", strlen(client_postval));
        len = fill_tcp_data(bufptr, len, strbuf);
        len = fill_tcp_data(bufptr, len, "\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n");
        len = fill_tcp_data(bufptr, len, client_postval);
    }

    return len;
}

/**
 * @brief Internal result callback for the WWW client.
 *
 * This function processes the result of the HTTP request.
 *
 * @param fd The file descriptor.
 * @param statuscode The status code.
 * @param datapos The position of the data in the buffer.
 * @param len_of_data The length of the data.
 * @return 0 Always returns 0.
 */
uint8_t www_client_internal_result_callback(uint8_t fd, uint8_t statuscode, uint16_t datapos, uint16_t len_of_data) {
    if (fd != www_fd) {
        client_browser_callback(4, 0, 0);
        return 0;
    }

    if (statuscode == 0 && len_of_data > 12 && client_browser_callback) {
        uint16_t offset = TCP_SRC_PORT_H_P + ((bufptr[TCP_HEADER_LEN_P] >> 4) * 4);
        uint8_t *status_code_pos = &bufptr[datapos + 9];

        if (strncmp("200", (char *)status_code_pos, 3) == 0) {
            client_browser_callback(0, offset, len_of_data);
        } else {
            client_browser_callback(1, offset, len_of_data);
        }
    }

    return 0;
}

/**
 * @brief Sends an HTTP GET request.
 *
 * @param urlbuf URL buffer.
 * @param urlbuf_varpart Variable part of the URL.
 * @param hoststr Hostname string.
 * @param callback Callback function to handle the result.
 */
void client_http_get(char *urlbuf, char *urlbuf_varpart, char *hoststr, void (*callback)(uint8_t, uint16_t, uint16_t)) {
    client_urlbuf = urlbuf;
    client_urlbuf_var = urlbuf_varpart;
    client_hoststr = hoststr;
    browsertype = 0;
    client_browser_callback = callback;
    www_fd = client_tcp_req(www_client_internal_result_callback, www_client_internal_datafill_callback, 80);
}

/**
 * @brief Sends an HTTP POST request.
 *
 * @param urlbuf URL buffer.
 * @param hoststr Hostname string.
 * @param additionalheaderline Additional header line, set to NULL if not used.
 * @param method HTTP method, set to NULL for default POST, alternative is PUT.
 * @param postval URL-encoded string buffer for the POST data.
 * @param callback Callback function to handle the result.
 */
void client_http_post(char *urlbuf, char *hoststr, char *additionalheaderline, char *method, char *postval, void (*callback)(uint8_t, uint16_t)) {
    client_urlbuf = urlbuf;
    client_hoststr = hoststr;
    client_additionalheaderline = additionalheaderline;
    client_method = method;
    client_postval = postval;
    browsertype = 1;
    client_browser_callback = (void (*)(uint8_t, uint16_t, uint16_t))callback;
    www_fd = client_tcp_req(www_client_internal_result_callback, www_client_internal_datafill_callback, 80);
}
#endif // WWW_client

/**
 * @brief Register a callback function for receiving ping replies.
 *
 * @param callback The callback function to be called when a ping reply is received.
 */
void register_ping_rec_callback(void (*callback)(uint8_t *srcip)) {
    icmp_callback = callback;
}

#ifdef PING_client
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
#endif // PING_client

uint8_t allocateIPAddress(uint8_t *buf, uint16_t buffer_size, uint8_t *mymac, uint16_t myport, uint8_t *myip, uint8_t *mynetmask, uint8_t *gwip, uint8_t *dnsip, uint8_t *dhcpsvrip) {
    uint16_t dat_p;
    int plen = 0;
    long lastDhcpRequest = HAL_GetTick();
    uint8_t dhcpState = 0;
    bool gotIp = FALSE;
    uint8_t dhcpTries = 10;

    dhcp_start(buf, mymac, myip, mynetmask, gwip, dnsip, dhcpsvrip);

    while (!gotIp) {
        plen = enc28j60PacketReceive(buffer_size, buf);
        dat_p = packetloop_icmp_tcp(buf, plen);

        if (dat_p == 0) {
            check_for_dhcp_answer(buf, plen);
            dhcpState = dhcp_state();

            if (dhcpState != DHCP_STATE_OK) {
                if (HAL_GetTick() > (lastDhcpRequest + 10000L)) {
                    lastDhcpRequest = HAL_GetTick();
                    if (--dhcpTries <= 0) {
                        return 0;
                    }
                    dhcp_start(buf, mymac, myip, mynetmask, gwip, dnsip, dhcpsvrip);
                }
            } else if (!gotIp) {
                gotIp = TRUE;

                init_ip_arp_udp_tcp(mymac, myip, myport);
                client_set_gwip(gwip);

#ifdef DNS_client
                dnslkup_set_dnsip(dnsip);
#endif
            }
        }
    }

    return 1;
}

// Perform all processing to resolve a hostname to IP address.
// Returns 1 for successful name resolution, 0 otherwise
uint8_t resolveHostname(uint8_t *buf, uint16_t buffer_size, uint8_t *hostname) {
    uint16_t dat_p;
    int plen = 0;
    long lastDnsRequest = HAL_GetTick();
    uint8_t dns_state = DNS_STATE_INIT;
    bool gotAddress = FALSE;
    uint8_t dnsTries = 3;  // After 3 attempts fail gracefully

    while (!gotAddress) {
        plen = enc28j60PacketReceive(buffer_size, buf);
        dat_p = packetloop_icmp_tcp(buf, plen);

        if (dat_p == 0) {
            if (client_waiting_gw()) {
                continue;
            }

            if (dns_state == DNS_STATE_INIT) {
                dns_state = DNS_STATE_REQUESTED;
                lastDnsRequest = HAL_GetTick();
                dnslkup_request(buf, hostname);
                continue;
            }

            if (dns_state != DNS_STATE_ANSWER) {
                if (HAL_GetTick() > (lastDnsRequest + 60000L)) {
                    if (--dnsTries <= 0) {
                        return 0;  // Failed to resolve address
                    }
                    dns_state = DNS_STATE_INIT;
                    lastDnsRequest = HAL_GetTick();
                }
                continue;
            }
        } else {
            if (dns_state == DNS_STATE_REQUESTED && udp_client_check_for_dns_answer(buf, plen)) {
                dns_state = DNS_STATE_ANSWER;
                client_tcp_set_serverip(dnslkup_getip());
                gotAddress = TRUE;
            }
        }
    }

    return 1;
}

// Return 0 to just continue in the packet loop and return the position
// of the tcp/udp data if there is tcp/udp data part
uint16_t packetloop_icmp_tcp(uint8_t *buf, uint16_t plen) {
    uint16_t len;
#if defined(TCP_client)
    uint8_t send_fin = 0;
    uint16_t tcpstart;
    uint16_t save_len;
#endif

    // Handle empty packets and ARP requests
    if (plen == 0) {
#if defined(NTP_client) || defined(UDP_client) || defined(TCP_client) || defined(PING_client)
        if ((waitgwmac & WGW_INITIAL_ARP || waitgwmac & WGW_REFRESHING) && delaycnt == 0 && enc28j60linkup()) {
            client_arp_whohas(buf, gwip);
        }
        delaycnt++;
#if defined(TCP_client)
        if (tcp_client_state == 1 && (waitgwmac & WGW_HAVE_GW_MAC)) { // Send a SYN
            tcp_client_state = 2;
            tcpclient_src_port_l++; // Allocate a new port
            client_syn(buf, ((tcp_fd << 5) | (0x1f & tcpclient_src_port_l)), tcp_client_port_h, tcp_client_port_l);
        }
#endif
#endif
        return 0;
    }

    // Handle ARP packets
    if (eth_type_is_arp_and_my_ip(buf, plen)) {
        if (buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REQ_L_V) {
            make_arp_answer_from_request(buf);
        }
#if defined(NTP_client) || defined(UDP_client) || defined(TCP_client) || defined(PING_client)
        if (waitgwmac & WGW_ACCEPT_ARP_REPLY && buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REPLY_L_V) {
            if (client_store_gw_mac(buf)) {
                waitgwmac = WGW_HAVE_GW_MAC;
            }
        }
#endif
        return 0;
    }

    // Check if IP packets are for us
    if (!eth_type_is_ip_and_my_ip(buf, plen)) {
        return 0;
    }

    // Handle ICMP echo requests
    if (buf[IP_PROTO_P] == IP_PROTO_ICMP_V && buf[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V) {
        if (icmp_callback) {
            icmp_callback(&buf[IP_SRC_P]);
        }
        make_echo_reply_from_request(buf, plen);
        ES_PingCallback();
        return 0;
    }

    // Handle UDP packets
    if (buf[IP_PROTO_P] == IP_PROTO_UDP_V) {
#ifdef NTP_client
        if (buf[UDP_SRC_PORT_H_P] == 0 && buf[UDP_SRC_PORT_L_P] == 0x7b) {
            client_ntp_process_answer(buf, plen);
            return UDP_DATA_P;
        }
#endif
#ifdef DNS_client
        if (buf[UDP_SRC_PORT_H_P] == 0 && buf[UDP_SRC_PORT_L_P] == 53) {
            return UDP_DATA_P;
        }
#endif
        udp_packet_process(buf, plen);
        return UDP_DATA_P;
    }

    // Handle TCP packets
    if (plen >= 54 && buf[IP_PROTO_P] == IP_PROTO_TCP_V) {
#if defined(TCP_client)
        if (buf[TCP_DST_PORT_H_P] == TCPCLIENT_SRC_PORT_H) {
#if defined(WWW_client)
            bufptr = buf;
#endif
            if (!check_ip_message_is_from(buf, tcpsrvip)) {
                return 0;
            }

            if (buf[TCP_FLAGS_P] & TCP_FLAGS_RST_V) {
                if (client_tcp_result_callback) {
                    client_tcp_result_callback((buf[TCP_DST_PORT_L_P] >> 5) & 0x7, 3, 0, 0);
                }
                tcp_client_state = 5;
                return 0;
            }

            len = get_tcp_data_len(buf);

            if (tcp_client_state == 2) {
                if ((buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V) && (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V)) {
                    make_tcp_ack_from_any(buf, 0, 0);
                    buf[TCP_FLAGS_P] = TCP_FLAGS_ACK_V | TCP_FLAGS_PUSH_V;

                    if (client_tcp_datafill_callback) {
                        len = client_tcp_datafill_callback((buf[TCP_SRC_PORT_L_P] >> 5) & 0x7);
                    } else {
                        len = 0;
                    }
                    tcp_client_state = 3;
                    make_tcp_ack_with_data_noflags(buf, len);
                    return 0;
                } else {
                    tcp_client_state = 1;
                    len++;
                    if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V) {
                        len = 0;
                    }
                    make_tcp_ack_from_any(buf, len, TCP_FLAGS_RST_V);
                    return 0;
                }
            }

            if (tcp_client_state == 3 || tcp_client_state == 4) {
                tcp_client_state = 4;
                if (client_tcp_result_callback) {
                    tcpstart = TCP_DATA_START;
                    if (tcpstart > plen - 8) {
                        tcpstart = plen - 8;
                    }
                    save_len = len;
                    if (tcpstart + len > plen) {
                        save_len = plen - tcpstart;
                    }
                    send_fin = client_tcp_result_callback((buf[TCP_DST_PORT_L_P] >> 5) & 0x7, 0, tcpstart, save_len);
                }
                if (send_fin) {
                    make_tcp_ack_from_any(buf, len, TCP_FLAGS_PUSH_V | TCP_FLAGS_FIN_V);
                    tcp_client_state = 5;
                    return 0;
                }
                make_tcp_ack_from_any(buf, len, 0);
                return 0;
            }

            if (tcp_client_state == 5) {
                return 0;
            }

            if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V) {
                make_tcp_ack_from_any(buf, len + 1, TCP_FLAGS_PUSH_V | TCP_FLAGS_FIN_V);
                tcp_client_state = 5;
                return 0;
            }

            if (len > 0) {
                make_tcp_ack_from_any(buf, len, 0);
            }
            return 0;
        }
#endif

        if (buf[TCP_DST_PORT_H_P] == wwwport_h && buf[TCP_DST_PORT_L_P] == wwwport_l) {
            if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V) {
                make_tcp_synack_from_syn(buf);
                return 0;
            }
            if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V) {
                info_data_len = get_tcp_data_len(buf);
                if (info_data_len == 0) {
                    if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V) {
                        make_tcp_ack_from_any(buf, 0, 0);
                    }
                    return 0;
                }
                len = TCP_DATA_START;
                if (len > plen - 8) {
                    return 0;
                }
                return len;
            }
        }
    }
    return 0;
}
#endif

/* end of ip_arp_udp.c */
