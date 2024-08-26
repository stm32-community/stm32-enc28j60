/*
 * tcp.C
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */
#include "defines.h"
#include "tcp.h"
#include "net.h"
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"
#include "websrv_help_functions.h"

//-------------------------------------- STEP1
// Initialisation by ES_init_network() for this 2 next function
void client_tcp_set_serverip(uint8_t *ipaddr) {
    memcpy(tcpsrvip, ipaddr, 4);
}

void initialize_tcp_sequence_number() {
    seqnum = random(); // Assurez-vous d'avoir une fonction pour générer un nombre aléatoire
}

// Définition de la variable globale
tcp_connection_t tcp_connections[MAX_TCP_CONNECTIONS];

// Function to initialize all TCP connections to the CLOSED state
void init_tcp_connections() {
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        tcp_connections[i].state = TCP_STATE_CLOSED;
        tcp_connections[i].seq_num = 0;
        tcp_connections[i].ack_num = 0;
        memset(tcp_connections[i].remote_ip, 0, sizeof(tcp_connections[i].remote_ip));
        tcp_connections[i].remote_port = 0;
        tcp_connections[i].local_port = 0;
    }
}
void initiate_tcp_connection(uint8_t connection_id, uint8_t *remote_ip, uint16_t remote_port, uint16_t local_port) {
    tcp_connections[connection_id].state = TCP_STATE_SYN_SENT;
    tcp_connections[connection_id].local_port = local_port;
    tcp_connections[connection_id].remote_port = remote_port;
    memcpy(tcp_connections[connection_id].remote_ip, remote_ip, 4);
    tcp_connections[connection_id].seq_num = 1;
    tcp_connections[connection_id].ack_num = 0;

    uint8_t buf[60];
    send_tcp_syn_packet(buf, local_port, remote_port, remote_ip);
}


// Function to find a free TCP connection (one that is in the CLOSED state)
tcp_connection_t* find_free_connection() {
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (tcp_connections[i].state == TCP_STATE_CLOSED) {
            return &tcp_connections[i]; // Return the first free connection found
        }
    }
    return NULL; // No free connection found
}

// Function to find an existing TCP connection based on remote IP and port
tcp_connection_t* find_connection(uint8_t *ip, uint16_t port) {
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (tcp_connections[i].state != TCP_STATE_CLOSED &&
            memcmp(tcp_connections[i].remote_ip, ip, sizeof(tcp_connections[i].remote_ip)) == 0 &&
            tcp_connections[i].remote_port == port) {
            return &tcp_connections[i]; // Return the matching connection
        }
    }
    return NULL; // No matching connection found
}

//-------------------------------------- STEP2
void make_tcphead(uint8_t *buf, uint16_t len, uint8_t mss, uint8_t win_size) {
    uint16_t ck;

    // Set IP length
    buf[0x10] = len >> 8; // IP_TOTLEN_H_P = 0x10
    buf[0x11] = len & 0xff; // IP_TOTLEN_L_P = 0x11

    // Set TCP header length
    buf[0x2C] = (20 / 4) << 4; // TCP_HEADER_LEN_P = 0x2C, TCP_HEADER_LEN_PLAIN = 20

    // Set MSS option if present
    if (mss) {
        buf[0x32] = 0x02;
        buf[0x33] = 0x04;
        buf[0x34] = (mss >> 8) & 0xFF;
        buf[0x35] = mss & 0xFF;
    }

    // Zero out checksum fields
    buf[0x2E] = 0; // TCP_CHECKSUM_H_P = 0x2E
    buf[0x2F] = 0; // TCP_CHECKSUM_L_P = 0x2F

    // Calculate and set TCP checksum
    ck = calculate_tcp_checksum(&buf[0x1A], len - 20); // IP_SRC_IP_P = 0x1A, TCP_HEADER_LEN_PLAIN = 20
    buf[0x2E] = ck >> 8; // TCP_CHECKSUM_H_P = 0x2E
    buf[0x2F] = ck & 0xff; // TCP_CHECKSUM_L_P = 0x2F

    // Calculate and set IP checksum
    fill_tcp_ip_hdr_checksum(buf);
}

// htons function: convert host byte order to network byte order (short)
static uint16_t TCP_htons(uint16_t hostshort) {
    return ((hostshort << 8) & 0xFF00) | ((hostshort >> 8) & 0x00FF);
}

static uint16_t ip_identifier = 1;

// Helper function to create a new IP + TCP packet
void make_ip_tcp_new(uint8_t *buf, uint16_t len, uint8_t *dst_ip) {
    static uint16_t ip_identifier = 1;

    buf[0] = IP_V4_V | IP_HEADER_LENGTH_V;
    buf[1] = 0x00;
    buf[2] = (len >> 8) & 0xFF;
    buf[3] = len & 0xFF;
    buf[4] = (ip_identifier >> 8) & 0xFF;
    buf[5] = ip_identifier & 0xFF;
    ip_identifier++;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf[8] = 128;
    buf[9] = IP_PROTO_TCP_V;
    memcpy(&buf[12], ipaddr, 4);
    memcpy(&buf[16], dst_ip, 4);
    fill_ip_hdr_checksum(buf);
}

void create_tcp_synack_packet(uint8_t *buf, tcp_connection_t *tcp_conn) {
    memset(buf + 0x22, 0, 20); // TCP_SRC_PORT_H_P = 0x22, TCP_HEADER_LEN_PLAIN = 20
    buf[0x2F] = 0x12; // TCP_FLAGS_P = 0x2F, TCP_FLAGS_SYNACK_V

    buf[0x22] = (tcp_conn->local_port >> 8) & 0xFF; // TCP_SRC_PORT_H_P = 0x22
    buf[0x23] = tcp_conn->local_port & 0xFF; // TCP_SRC_PORT_L_P = 0x23
    buf[0x24] = (tcp_conn->remote_port >> 8) & 0xFF; // TCP_DST_PORT_H_P = 0x24
    buf[0x25] = tcp_conn->remote_port & 0xFF; // TCP_DST_PORT_L_P = 0x25

    uint32_t seq_num = tcp_conn->seq_num;
    uint32_t ack_num = tcp_conn->ack_num + 1;
    buf[0x26] = (seq_num >> 24) & 0xFF; // TCP_SEQ_P = 0x26
    buf[0x27] = (seq_num >> 16) & 0xFF;
    buf[0x28] = (seq_num >> 8) & 0xFF;
    buf[0x29] = seq_num & 0xFF;
    buf[0x2A] = (ack_num >> 24) & 0xFF; // TCP_SEQACK_P = 0x2A
    buf[0x2B] = (ack_num >> 16) & 0xFF;
    buf[0x2C] = (ack_num >> 8) & 0xFF;
    buf[0x2D] = ack_num & 0xFF;

    buf[0x30] = 0x05; // TCP_WINDOWSIZE_H_P = 0x30
    buf[0x31] = 0xB4; // TCP_WINDOWSIZE_L_P = 0x31, 1460 bytes

    // Calculate and set checksums
    fill_ip_tcp_checksum(buf, 20 + 20); // IP_HEADER_LEN = 20, TCP_HEADER_LEN_PLAIN = 20

    // Update connection state
    tcp_conn->state = TCP_STATE_SYN_RECEIVED;
    tcp_conn->seq_num += 1;
}

// Function to create and send a TCP ACK packet
void create_tcp_ack_packet(uint8_t *buf, tcp_connection_t *tcp_conn, uint8_t mss, uint8_t win_size) {
    uint16_t len = 20 + (mss ? 4 : 0); // TCP_HEADER_LEN_PLAIN = 20

    // Configure the TCP header with necessary information
    buf[0x2F] = 0x10; // TCP_FLAGS_ACK_V

    // Set source and destination ports
    buf[0x22] = tcp_conn->local_port >> 8; // TCP_SRC_PORT_H_P = 0x22
    buf[0x23] = tcp_conn->local_port & 0xFF; // TCP_SRC_PORT_L_P = 0x23
    buf[0x24] = tcp_conn->remote_port >> 8; // TCP_DST_PORT_H_P = 0x24
    buf[0x25] = tcp_conn->remote_port & 0xFF; // TCP_DST_PORT_L_P = 0x25

    // Set sequence and acknowledgment numbers
    buf[0x26] = tcp_conn->seq_num >> 24; // TCP_SEQ_H_P = 0x26
    buf[0x27] = tcp_conn->seq_num >> 16;
    buf[0x28] = tcp_conn->seq_num >> 8;
    buf[0x29] = tcp_conn->seq_num & 0xFF;

    buf[0x2A] = tcp_conn->ack_num >> 24; // TCP_ACK_H_P = 0x2A
    buf[0x2B] = tcp_conn->ack_num >> 16;
    buf[0x2C] = tcp_conn->ack_num >> 8;
    buf[0x2D] = tcp_conn->ack_num & 0xFF;

    // Set window size
    buf[0x30] = win_size >> 8; // TCP_WINDOWSIZE_H_P = 0x30
    buf[0x31] = win_size & 0xFF; // TCP_WINDOWSIZE_L_P = 0x31

    // Zero out the checksum fields
    buf[0x32] = 0; // TCP_CHECKSUM_H_P = 0x32
    buf[0x33] = 0; // TCP_CHECKSUM_L_P = 0x33

    // Fill MSS option if necessary
    if (mss) {
        buf[0x34] = mss; // TCP_OPTIONS_P = 0x34
    }

    // Calculate TCP checksum
    uint16_t checksum = calculate_tcp_checksum(buf, len);
    buf[0x32] = checksum >> 8; // TCP_CHECKSUM_H_P = 0x32
    buf[0x33] = checksum & 0xFF; // TCP_CHECKSUM_L_P = 0x33

    // Calculate IP checksum
    fill_tcp_ip_hdr_checksum(buf);

    // Send the packet
    enc28j60PacketSend(20 + len + 14, buf); // IP_HEADER_LEN = 20, ETH_HEADER_LEN = 14

    udpLog2("TCP", "ACK packet sent");
}

// Function to create and send a TCP ACK packet with data
void create_tcp_ack_with_data_packet(uint8_t *buf, uint16_t len) {
    uint16_t total_len = 20 + 20 + len; // IP_HEADER_LEN = 20, TCP_HEADER_LEN_PLAIN = 20

    // Update the total length field in the IP header
    buf[0x10] = total_len >> 8; // IP_TOTLEN_H_P = 0x10
    buf[0x11] = total_len & 0xff; // IP_TOTLEN_L_P = 0x11

    // Update the TCP header length field
    buf[0x2E] = (20 / 4) << 4; // TCP_HEADER_LEN_P = 0x2E, TCP_HEADER_LEN_PLAIN = 20

    // Calculate and set the TCP checksum
    buf[0x32] = 0; // TCP_CHECKSUM_H_P = 0x32
    buf[0x33] = 0; // TCP_CHECKSUM_L_P = 0x33
    uint16_t tcp_ck = calculate_tcp_checksum(&buf[0x1A], 8 + 20 + len); // IP_SRC_IP_P = 0x1A, TCP_HEADER_LEN_PLAIN = 20
    buf[0x32] = tcp_ck >> 8; // TCP_CHECKSUM_H_P = 0x32
    buf[0x33] = tcp_ck & 0xff; // TCP_CHECKSUM_L_P = 0x33

    // Update the TCP flags to include ACK and PUSH
    buf[0x2F] = 0x18; // TCP_FLAGS_ACK_V = 0x10, TCP_FLAGS_PUSH_V = 0x08

    // Calculate and set the IP header checksum
    buf[0x18] = 0; // IP_CHECKSUM_H_P = 0x18
    buf[0x19] = 0; // IP_CHECKSUM_L_P = 0x19
    uint16_t ip_ck = calculate_tcp_checksum(&buf[0x0E], 20); // IP_P = 0x0E, IP_HEADER_LEN = 20
    buf[0x18] = ip_ck >> 8; // IP_CHECKSUM_H_P = 0x18
    buf[0x19] = ip_ck & 0xff; // IP_CHECKSUM_L_P = 0x19

    // Log the IP and TCP headers
    log_ip_header(buf);
    log_tcp_header(buf);

    // Send the packet
    enc28j60PacketSend(total_len + 14, buf); // ETH_HEADER_LEN = 14
}
//-------------------------------------- STEP3
// Function to calculate the length of TCP data
uint16_t get_tcp_data_len(uint8_t *buf) {
    int16_t total_len = ((int16_t)buf[0x10] << 8) | (buf[0x11] & 0xff); // IP_TOTLEN_H_P, IP_TOTLEN_L_P
    int16_t header_len = 20 + ((buf[0x2E] >> 4) * 4); // IP_HEADER_LEN, TCP_HEADER_LEN_P (0x2E)
    int16_t data_len = total_len - header_len;

    // Return the data length if it is greater than 0, otherwise return 0
    return (data_len > 0) ? (uint16_t)data_len : 0;
}

// Function to fill TCP data into the buffer
uint16_t fill_tcp_data(uint8_t *buf, uint16_t pos, const uint8_t *data, uint16_t len) {
    memcpy(&buf[0x35 + 3 + pos], data, len); // TCP_CHECKSUM_L_P (0x35)
    return pos + len; // Return the new position after the data
}


// Function to build TCP data packet
uint16_t build_tcp_data(uint8_t *buf, uint16_t srcPort) {
    udpLog2("TCP_client", "Entering build_tcp_data");

    // Build Ethernet header
    memcpy(&buf[0x00], gwmacaddr, 6); // ETH_DST_MAC
    memcpy(&buf[0x06], macaddr, 6); // ETH_SRC_MAC
    buf[0x0C] = 0x08; // ETH_TYPE_H_P (ETHTYPE_IP_H_V)
    buf[0x0D] = 0x00; // ETH_TYPE_L_P (ETHTYPE_IP_L_V)

    // Build IP header
    fill_buf_p(&buf[0x0E], 9, iphdr); // IP_P
    buf[0x10] = 0x00; // IP_TOTLEN_H_P
    buf[0x11] = 40; // IP_TOTLEN_L_P (IP total length field)
    buf[0x17] = 0x06; // IP_PROTO_P (IP_PROTO_TCP_V)
    memcpy(&buf[0x1E], tcpsrvip, 4); // IP_DST_IP_P
    memcpy(&buf[0x22], ipaddr, 4); // IP_SRC_IP_P
    fill_ip_hdr_checksum(buf);

    // Build TCP header
    buf[0x24] = tcp_client_port_h; // TCP_DST_PORT_H_P
    buf[0x25] = tcp_client_port_l; // TCP_DST_PORT_L_P
    buf[0x26] = srcPort >> 8; // TCP_SRC_PORT_H_P
    buf[0x27] = srcPort & 0xff; // TCP_SRC_PORT_L_P

    // Zero out sequence number and acknowledgement number
    memset(&buf[0x28], 0, 8); // TCP_SEQ_H_P

    // Set initial sequence number
    buf[0x2A] = seqnum; // TCP_SEQ_H_P + 2
    seqnum += 3;

    // Set TCP header length and flags
    buf[0x2E] = 0x60; // TCP_HEADER_LEN_P (Header length (5 words) and reserved bits)
    buf[0x2F] = 0x08; // TCP_FLAGS_P (TCP_FLAGS_PUSH_V)

    // Set TCP window size
    buf[0x30] = 0x4; // TCP_WIN_SIZE
    buf[0x31] = 0x0; // TCP_WIN_SIZE + 1

    // Zero out TCP checksum and urgent pointer
    buf[0x34] = 0; // TCP_CHECKSUM_H_P
    buf[0x35] = 0; // TCP_CHECKSUM_L_P
    buf[0x36] = 0; // TCP_CHECKSUM_L_P + 1
    buf[0x37] = 0; // TCP_CHECKSUM_L_P + 2

    // Set MSS option
    buf[0x38] = 2; // TCP_OPTIONS_P
    buf[0x39] = 4; // TCP_OPTIONS_P + 1
    buf[0x3A] = CLIENTMSS >> 8; // TCP_OPTIONS_P + 2
    buf[0x3B] = CLIENTMSS & 0xff; // TCP_OPTIONS_P + 3

    // Calculate and set TCP checksum
    uint16_t ck = checksum(&buf[0x1E], 8 + 20 + 4, 2); // IP_SRC_IP_P + 8 + TCP_HEADER_LEN_PLAIN + 4
    buf[0x34] = ck >> 8; // TCP_CHECKSUM_H_P
    buf[0x35] = ck & 0xff; // TCP_CHECKSUM_L_P

    udpLog2("TCP_client", "Built TCP data");

    // Return the total length of the packet
    return 20 + 20 + 14 + 4; // IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + ETH_HEADER_LEN + 4
}

// Function to send TCP data
void send_tcp_data(uint8_t *buf, uint16_t dlen) {
    udpLog2("TCP_client", "Entering send_tcp_data");

    // Set TCP flags to PUSH
    buf[0x2F] = 0x08; // TCP_FLAGS_P (TCP_FLAGS_PUSH_V)

    // Calculate total length of the IP packet (IP header + TCP header + data length)
    uint16_t total_len = 20 + 20 + dlen; // IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + dlen
    buf[0x10] = total_len >> 8; // IP_TOTLEN_H_P
    buf[0x11] = total_len & 0xff; // IP_TOTLEN_L_P

    // Fill in the IP header checksum
    fill_ip_hdr_checksum(buf);

    // Set TCP checksum to zero before calculation
    buf[0x34] = 0; // TCP_CHECKSUM_H_P
    buf[0x35] = 0; // TCP_CHECKSUM_L_P

    // Calculate TCP checksum
    uint16_t ck = checksum(&buf[0x1E], 8 + 20 + dlen, 2); // IP_SRC_IP_P + 8 + TCP_HEADER_LEN_PLAIN + dlen
    buf[0x34] = ck >> 8; // TCP_CHECKSUM_H_P
    buf[0x35] = ck & 0xff; // TCP_CHECKSUM_L_P

    // Send the packet
    enc28j60PacketSend(total_len + 14, buf); // total_len + ETH_HEADER_LEN

    udpLog2("TCP_client", "Sent TCP data");
}

// Function to build and send TCP data packet
void send_data_packet(uint8_t *buf, uint16_t srcPort, const uint8_t *data, uint16_t len) {
    // Build TCP data packet
    uint16_t pos = build_tcp_data(buf, srcPort);

    // Fill TCP data into the packet
    pos = fill_tcp_data(buf, pos, data, len);

    // Send the TCP data packet
    send_tcp_data(buf, pos - (20 + 20 + 14)); // IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + ETH_HEADER_LEN
}

// Function to send a FIN packet to close the TCP connection
void send_tcp_fin_packet(uint8_t *buf) {
    // Set TCP flags to ACK and FIN
    buf[0x2F] = 0x11; // TCP_FLAGS_P offset (TCP_FLAGS_ACK_V | TCP_FLAGS_FIN_V)

    // Set TCP checksum to zero before calculation
    buf[0x34] = 0; // TCP_CHECKSUM_H_P offset
    buf[0x35] = 0; // TCP_CHECKSUM_L_P offset

    // Calculate TCP checksum
    uint16_t tcp_ck = checksum(&buf[0x1E], 8 + 20, 2); // IP_SRC_IP_P offset + 8 + TCP_HEADER_LEN_PLAIN
    buf[0x34] = tcp_ck >> 8; // TCP_CHECKSUM_H_P offset
    buf[0x35] = tcp_ck & 0xff; // TCP_CHECKSUM_L_P offset

    // Set IP checksum to zero before calculation
    buf[0x18] = 0; // IP_CHECKSUM_H_P offset
    buf[0x19] = 0; // IP_CHECKSUM_L_P offset

    // Calculate IP checksum
    uint16_t ip_ck = checksum(&buf[0x0E], 20, 0); // IP_P offset + IP_HEADER_LEN
    buf[0x18] = ip_ck >> 8; // IP_CHECKSUM_H_P offset
    buf[0x19] = ip_ck & 0xff; // IP_CHECKSUM_L_P offset

    // Log sending FIN packet
    char log_buffer[100];
    sprintf(log_buffer, "Sending FIN packet. TCP Checksum: %04x", tcp_ck);
    udpLog2("TCP", log_buffer);

    // Send the FIN packet
    enc28j60PacketSend(20 + 20 + 14, buf); // IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + ETH_HEADER_LEN
}

//-------------------------------------- STEP4
// Function to prepare TCP data at a specified position and return the new position
uint16_t prepare_tcp_data(uint8_t *buf, uint16_t pos, const uint8_t *data, uint16_t len) {
    // Copy data to the specified position in the buffer
    memcpy(&buf[0x37 + pos], data, len); // TCP_CHECKSUM_L_P + 3 offset

    // Return the new position after adding the data
    return pos + len;
}

// Function to transmit prepared TCP data
void transmit_tcp_data(uint8_t *buf, uint16_t dlen) {
    udpLog2("TCP_client", "Entering transmit_tcp_data");

    // Set TCP flags to indicate PUSH operation
    buf[0x2F] = 0x18; // TCP_FLAGS_P offset (TCP_FLAGS_PUSH_V)

    // Calculate total length of the IP packet (IP header + TCP header + data length)
    uint16_t total_len = 20 + 20 + dlen; // IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + data length
    buf[0x10] = total_len >> 8; // IP_TOTLEN_H_P offset
    buf[0x11] = total_len & 0xff; // IP_TOTLEN_L_P offset

    // Fill in the IP header checksum
    fill_ip_hdr_checksum(buf);

    // Set TCP checksum to zero before calculation
    buf[0x34] = 0; // TCP_CHECKSUM_H_P offset
    buf[0x35] = 0; // TCP_CHECKSUM_L_P offset

    // Calculate TCP checksum
    uint16_t ck = checksum(&buf[0x1E], 8 + 20 + dlen, 2); // IP_SRC_IP_P offset + 8 + TCP_HEADER_LEN_PLAIN + data length
    buf[0x34] = ck >> 8; // TCP_CHECKSUM_H_P offset
    buf[0x35] = ck & 0xff; // TCP_CHECKSUM_L_P offset

    // Send the packet
    enc28j60PacketSend(total_len + 14, buf); // total length + ETH_HEADER_LEN

    udpLog2("TCP_client", "Sent TCP data");
}

//-------------------------------------- OTHERS

// Function to create and send a TCP SYN packet
void client_syn(uint8_t *buf, uint8_t srcport, uint8_t dstport_h, uint8_t dstport_l) {
    udpLog2("TCP_client", "Entering client_syn");

    // Copy MAC addresses to the buffer
    memcpy(&buf[0x00], gwmacaddr, 6);  // ETH_DST_MAC offset
    memcpy(&buf[0x06], macaddr, 6);    // ETH_SRC_MAC offset

    // Set Ethernet type to IP
    buf[0x0C] = 0x08;  // ETH_TYPE_H_P offset (ETHTYPE_IP_H_V)
    buf[0x0D] = 0x00;  // ETH_TYPE_L_P offset (ETHTYPE_IP_L_V)

    // Fill IP header
    fill_buf_p(&buf[0x0E], 9, iphdr);  // IP_P offset

    // Set IP total length for SYN packet
    buf[0x10] = 0x00;  // IP_TOTLEN_H_P offset
    buf[0x11] = 44;    // IP_TOTLEN_L_P offset (20 bytes IP header + 24 bytes TCP header)
    buf[0x17] = 0x06;  // IP_PROTO_P offset (IP_PROTO_TCP_V)

    // Set source and destination IP addresses
    memcpy(&buf[0x1A], tcpsrvip, 4);   // IP_DST_IP_P offset
    memcpy(&buf[0x1E], ipaddr, 4);     // IP_SRC_IP_P offset

    // Calculate and set IP header checksum
    fill_ip_hdr_checksum(buf);

    // Set TCP source and destination ports
    buf[0x22] = dstport_h;             // TCP_DST_PORT_H_P offset
    buf[0x23] = dstport_l;             // TCP_DST_PORT_L_P offset
    buf[0x20] = TCPCLIENT_SRC_PORT_H;  // TCP_SRC_PORT_H_P offset
    buf[0x21] = srcport;               // TCP_SRC_PORT_L_P offset

    // Zero out sequence number and acknowledgment number
    memset(&buf[0x26], 0, 4);          // TCP_SEQ_H_P offset
    memset(&buf[0x2A], 0, 4);          // TCP_ACK_H_P offset

    // Set initial sequence number
    buf[0x28] = seqnum >> 8;           // TCP_SEQ_H_P offset + 2
    buf[0x29] = seqnum & 0xff;         // TCP_SEQ_H_P offset + 3
    seqnum += 3;                       // Increment sequence number for next use

    // Set TCP header length and flags
    buf[0x2E] = 0x60;                  // TCP_HEADER_LEN_P offset (24 bytes TCP header)
    buf[0x2F] = 0x02;                  // TCP_FLAGS_P offset (TCP_FLAGS_SYN_V)

    // Set TCP window size
    buf[0x30] = 0x04;                  // TCP_WIN_SIZE offset
    buf[0x31] = 0x00;                  // TCP_WIN_SIZE offset + 1

    // Zero out TCP checksum
    buf[0x34] = 0x00;                  // TCP_CHECKSUM_H_P offset
    buf[0x35] = 0x00;                  // TCP_CHECKSUM_L_P offset

    // Set urgent pointer
    buf[0x36] = 0x00;                  // TCP_CHECKSUM_L_P offset + 1
    buf[0x37] = 0x00;                  // TCP_CHECKSUM_L_P offset + 2

    // Set TCP options for MSS (Maximum Segment Size)
    buf[0x38] = 0x02;                  // TCP_OPTIONS_P offset (Option kind = MSS)
    buf[0x39] = 0x04;                  // TCP_OPTIONS_P offset + 1 (Option length)
    buf[0x3A] = (CLIENTMSS >> 8);      // TCP_OPTIONS_P offset + 2 (MSS value high byte)
    buf[0x3B] = CLIENTMSS & 0xff;      // TCP_OPTIONS_P offset + 3 (MSS value low byte)

    // Calculate and set TCP checksum
    uint16_t ck = checksum(&buf[0x1E], 8 + 20 + 4, 2);  // IP_SRC_IP_P offset + 8 + TCP_HEADER_LEN_PLAIN + 4
    buf[0x34] = ck >> 8;                // TCP_CHECKSUM_H_P offset
    buf[0x35] = ck & 0xff;              // TCP_CHECKSUM_L_P offset

    // Send the SYN packet
    enc28j60PacketSend(20 + 20 + 14 + 4, buf);  // IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + ETH_HEADER_LEN + 4

    udpLog2("TCP_client", "Sent TCP Syn");
}

// Function to create and send a TCP ACK packet with data, without setting TCP flags
void make_tcp_ack_with_data_noflags(uint8_t *buf, uint16_t dlen) {
    uint16_t total_len = 20 + 20 + dlen;  // IP_HEADER_LEN + TCP_HEADER_LEN_PLAIN + data length

    // Set the total length field in the IP header
    buf[0x10] = total_len >> 8;       // IP_TOTLEN_H_P offset
    buf[0x11] = total_len & 0xff;     // IP_TOTLEN_L_P offset

    // Calculate and set the IP header checksum
    fill_ip_hdr_checksum(buf);

    // Zero out the TCP checksum field
    buf[0x34] = 0;                    // TCP_CHECKSUM_H_P offset
    buf[0x35] = 0;                    // TCP_CHECKSUM_L_P offset

    // Calculate the TCP checksum
    uint16_t tcp_ck = checksum(&buf[0x1E], 8 + 20 + dlen, 2);  // IP_SRC_IP_P offset + 8 + TCP_HEADER_LEN_PLAIN + data length
    buf[0x34] = tcp_ck >> 8;          // TCP_CHECKSUM_H_P offset
    buf[0x35] = tcp_ck & 0xff;        // TCP_CHECKSUM_L_P offset

    // Send the packet
    enc28j60PacketSend(total_len + 14, buf);  // total length + ETH_HEADER_LEN

    udpLog2("TCP_client", "Sent TCP ACK with data (no flags)");
}


// Function to initiate a TCP request
uint8_t client_tcp_req(
    uint8_t (*result_callback)(uint8_t fd, uint8_t statuscode, uint16_t data_start_pos_in_buf, uint16_t len_of_data),
    uint16_t (*datafill_callback)(uint8_t fd),
    uint16_t port) {

    udpLog2("TCP_client", "Entering client_tcp_req");

    // Set the callback functions
    client_tcp_result_callback = result_callback;
    client_tcp_datafill_callback = datafill_callback;

    // Set the destination port
    tcp_client_port_h = (port >> 8) & 0xff;
    tcp_client_port_l = port & 0xff;

    // Initialize TCP client state
    tcp_client_state = 1;

    // Increment and wrap around the file descriptor
    if (++tcp_fd > 7) {
        tcp_fd = 0;
    }

    udpLog2("TCP_client", "TCP Request initialized");

    return tcp_fd;
}


void fill_ip_tcp_checksum(uint8_t *buf, uint16_t len) {
    // IP header length
    uint8_t ip_header_len = 0x14;  // 20 bytes

    // IP checksum offsets
    uint8_t ip_checksum_h_p = 0x18;
    uint8_t ip_checksum_l_p = 0x19;

    // TCP checksum offsets
    uint8_t tcp_checksum_h_p = 0x2E;
    uint8_t tcp_checksum_l_p = 0x2F;

    // Source IP and destination IP offsets
    uint8_t ip_src_ip_p = 0x1A;
    uint8_t ip_dst_ip_p = 0x1E;

    // Zero out IP checksum field
    buf[ip_checksum_h_p] = 0;
    buf[ip_checksum_l_p] = 0;

    // Calculate IP header checksum
    uint16_t ip_ck = calculate_tcp_checksum(&buf[0x0E], ip_header_len);  // Starting at IP header

    // Set IP header checksum
    buf[ip_checksum_h_p] = ip_ck >> 8;
    buf[ip_checksum_l_p] = ip_ck & 0xFF;

    // Zero out TCP checksum field
    buf[tcp_checksum_h_p] = 0;
    buf[tcp_checksum_l_p] = 0;

    // Calculate TCP checksum
    uint16_t tcp_ck = calculate_tcp_checksum(&buf[ip_src_ip_p], len);  // Starting at source IP

    // Set TCP checksum
    buf[tcp_checksum_h_p] = tcp_ck >> 8;
    buf[tcp_checksum_l_p] = tcp_ck & 0xFF;
}

void fill_tcp_ip_hdr_checksum(uint8_t *buf) {
    uint16_t ck;

    // Zero out IP checksum field
    buf[0x18] = 0;
    buf[0x19] = 0;

    // Calculate IP header checksum
    ck = calculate_tcp_checksum(&buf[0x0E], 20); // IP_P = 0x0E, IP_HEADER_LEN = 20

    // Set IP header checksum
    buf[0x18] = ck >> 8;
    buf[0x19] = ck & 0xff;
}

uint16_t calculate_tcp_checksum(uint8_t *buf, uint16_t len) {
    uint32_t sum = 0;

    // Sum up 2-byte values from the buffer
    for (int i = 0; i < len; i += 2) {
        sum += (buf[i] << 8) | buf[i + 1];
    }

    // Add carry bits to the lower 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // One's complement
    return ~sum;
}

void log_ip_header(uint8_t *buf) {
    char log_buffer[100];
    udpLog2("IP Header", "-----------------");
    sprintf(log_buffer, "IP Version/IHL: 0x%02x", buf[IP_P]);
    udpLog2("IP Header", log_buffer);
    sprintf(log_buffer, "Type of Service: 0x%02x", buf[IP_TOS_P]);
    udpLog2("IP Header", log_buffer);
    sprintf(log_buffer, "Total Length: %d", (buf[IP_TOTLEN_H_P] << 8) | buf[IP_TOTLEN_L_P]);
    udpLog2("IP Header", log_buffer);
    sprintf(log_buffer, "ID: 0x%04x", (buf[IP_ID_H_P] << 8) | buf[IP_ID_L_P]);
    udpLog2("IP Header", log_buffer);
    sprintf(log_buffer, "Flags/Fragment Offset: 0x%04x", (buf[IP_FLAGS_H_P] << 8) | buf[IP_FLAGS_L_P]);
    udpLog2("IP Header", log_buffer);
    sprintf(log_buffer, "TTL: %d", buf[IP_TTL_P]);
    udpLog2("IP Header", log_buffer);
    sprintf(log_buffer, "Protocol: %d", buf[IP_PROTO_P]);
    udpLog2("IP Header", log_buffer);
    sprintf(log_buffer, "Header Checksum: 0x%04x", (buf[IP_CHECKSUM_H_P] << 8) | buf[IP_CHECKSUM_L_P]);
    udpLog2("IP Header", log_buffer);
    sprintf(log_buffer, "Source IP: %d.%d.%d.%d", buf[IP_SRC_IP_P], buf[IP_SRC_IP_P + 1], buf[IP_SRC_IP_P + 2], buf[IP_SRC_IP_P + 3]);
    udpLog2("IP Header", log_buffer);
    sprintf(log_buffer, "Destination IP: %d.%d.%d.%d", buf[IP_DST_IP_P], buf[IP_DST_IP_P + 1], buf[IP_DST_IP_P + 2], buf[IP_DST_IP_P + 3]);
    udpLog2("IP Header", log_buffer);
}


void log_tcp_header(uint8_t *buf) {
    char log_buffer[100];
    udpLog2("TCP Header", "-----------------");
    sprintf(log_buffer, "Source Port: %d", (buf[TCP_SRC_PORT_H_P] << 8) | buf[TCP_SRC_PORT_L_P]);
    udpLog2("TCP Header", log_buffer);
    sprintf(log_buffer, "Destination Port: %d", (buf[TCP_DST_PORT_H_P] << 8) | buf[TCP_DST_PORT_L_P]);
    udpLog2("TCP Header", log_buffer);
    sprintf(log_buffer, "Sequence Number: 0x%08x", (buf[TCP_SEQ_H_P] << 24) | (buf[TCP_SEQ_H_P + 1] << 16) | (buf[TCP_SEQ_H_P + 2] << 8) | buf[TCP_SEQ_H_P + 3]);
    udpLog2("TCP Header", log_buffer);
    sprintf(log_buffer, "Acknowledgment Number: 0x%08x", (buf[TCP_ACK_H_P] << 24) | (buf[TCP_ACK_H_P + 1] << 16) | (buf[TCP_ACK_H_P + 2] << 8) | buf[TCP_ACK_H_P + 3]);
    udpLog2("TCP Header", log_buffer);
    sprintf(log_buffer, "Data Offset: %d", (buf[TCP_HEADER_LEN_P] >> 4) * 4);
    udpLog2("TCP Header", log_buffer);
    sprintf(log_buffer, "Flags: 0x%02x", buf[TCP_FLAGS_P]);
    udpLog2("TCP Header", log_buffer);
    sprintf(log_buffer, "Window Size: %d", (buf[TCP_WINDOWSIZE_H_P] << 8) | buf[TCP_WINDOWSIZE_L_P]);
    udpLog2("TCP Header", log_buffer);
    sprintf(log_buffer, "Checksum: 0x%04x", (buf[TCP_CHECKSUM_H_P] << 8) | buf[TCP_CHECKSUM_L_P]);
    udpLog2("TCP Header", log_buffer);
    sprintf(log_buffer, "Urgent Pointer: %d", (buf[TCP_URGENT_PTR_H_P] << 8) | buf[TCP_URGENT_PTR_L_P]);
    udpLog2("TCP Header", log_buffer);
}
