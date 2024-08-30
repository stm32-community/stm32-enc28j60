/**
 * @file ip_arp_udp_tcp.c
 * @brief Source file containing the implementation of IP, ARP, UDP, and TCP protocols for network communication.
 *
 * This file includes functions for handling IP, ARP, UDP, and TCP packets, managing network communication,
 * and processing checksums. It also provides implementations for ARP requests, UDP packet transmission,
 * and basic network initialization.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
 */


#include "defines.h"
#include "net.h"
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"
#include "websrv_help_functions.h"

// The checksum function calculates checksums for IP, UDP, and TCP packets.
// For IP and ICMP, the checksum is calculated over the IP header only.
// For UDP and TCP, the checksum includes a pseudo-header using the source and destination IP fields.
uint16_t checksum(uint8_t *buf, uint16_t len, uint8_t type) {
    uint32_t sum = 0;
    // Ajout des valeurs spécifiques au protocole pour UDP et TCP
    if (type == 1) {
        sum += IP_PROTO_UDP_V; // Protocole UDP
        sum += len - 8;        // Longueur UDP (données + en-tête)
    } else if (type == 2) {
        sum += IP_PROTO_TCP_V; // Protocole TCP
        sum += len - 8;        // Longueur TCP (données + en-tête)
    }

    // Sommation des mots de 16 bits
    while (len > 1) {
        sum += 0xFFFF & (*buf << 8 | *(buf + 1));
        buf += 2;
        len -= 2;
    }

    // Si un octet reste, l'ajouter (complété par zéro)
    if (len) {
        sum += (0xFF & *buf) << 8;
    }

    // Réduction de la somme de 32 bits à 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Retourner le complément à un de la somme
    return (uint16_t)(sum ^ 0xFFFF);
}
// This function initializes the web server.
// You must call this function once before you use any of the other functions.
void init_ip_arp_udp_tcp(uint8_t *mymac, uint8_t *myip, uint16_t port) {
    wwwport_h = (port >> 8) & 0xFF;
    wwwport_l = port & 0xFF;
    memcpy(ipaddr, myip, 4);
    memcpy(macaddr, mymac, 6);
}

// Check if an IP message is from a specified IP address
uint8_t check_ip_message_is_from(uint8_t *buf, uint8_t *ip) {
    return memcmp(&buf[IP_SRC_IP_P], ip, 4) == 0;
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
    return memcmp(&buf[IP_DST_IP_P], ipaddr, 4) == 0;
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

// Function to calculate IP header checksum
void fill_ip_hdr_checksum(uint8_t *buf) {
    uint16_t ck;

    // Zero out IP checksum field
    buf[0x18] = 0;
    buf[0x19] = 0;

    // Calculate IP header checksum
    ck = calculate_tcp_checksum(&buf[0x0E], 20); // IP_P = 0x0E, IP_HEADER_LEN = 20

    // Set IP header checksum
    buf[0x18] = ck >> 8;
    buf[0x19] = ck & 0xFF;
}


// Check if it's an ARP reply
uint8_t eth_type_is_arp_reply(uint8_t *buf) {
    return buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REPLY_L_V;
}

// Check if it's an ARP request
uint8_t eth_type_is_arp_req(uint8_t *buf) {
    return buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REQ_L_V;
}

// Make a return IP header from a received IP packet
void make_ip(uint8_t *buf) {
    memcpy(&buf[IP_DST_IP_P], &buf[IP_SRC_IP_P], 4);
    memcpy(&buf[IP_SRC_IP_P], ipaddr, 4);
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
    uint16_t ck = checksum(&buf[IP_SRC_IP_P], 16 + datalen, 1);
    buf[UDP_CHECKSUM_H_P] = ck >> 8;
    buf[UDP_CHECKSUM_L_P] = ck & 0xff;

    enc28j60PacketSend(UDP_HEADER_LEN + IP_HEADER_LEN + ETH_HEADER_LEN + datalen, buf);
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

#if defined(NTP_client) || defined(WOL_client) || defined(UDP_client) || defined(TCP_client) || defined(PING_client)
// Fills the buffer with a string of specified length
void fill_buf_p(uint8_t *buf, uint16_t len, const char *s) {
    memcpy(buf, s, len);
}

// Prepare a spontaneous UDP packet to a server
void send_udp_prepare(uint8_t *buf, uint16_t sport, uint8_t *dip, uint16_t dport) {
    memcpy(&buf[ETH_DST_MAC], gwmacaddr, 6);
    memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
    buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
    buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
    fill_buf_p(&buf[IP_P], 9, iphdr);

    buf[IP_TOTLEN_H_P] = 0;
    buf[IP_PROTO_P] = IP_PROTO_UDP_V;
    memcpy(&buf[IP_DST_IP_P], dip, 4);
    memcpy(&buf[IP_SRC_IP_P], ipaddr, 4);

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

    uint16_t ck = checksum(&buf[IP_SRC_IP_P], 16 + datalen, 1);
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

#if defined (NTP_client) || defined (UDP_client) || defined (TCP_client)
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
#endif // defined (NTP_client) || defined (UDP_client) || defined (TCP_client)

#endif

/* end of ip_arp_udp.c */
