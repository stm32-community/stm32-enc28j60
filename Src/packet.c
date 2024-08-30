/**
 * @file packet.c
 * @brief Source file implementing packet processing functions for handling Ethernet, TCP, UDP, ICMP, and HTTP protocols.
 *
 * This file includes functions to process incoming Ethernet packets, including handling ARP, IP, TCP, UDP,
 * and ICMP protocols. It also manages TCP connection states, processes HTTP requests, and handles UDP-based
 * commands and NTP responses.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
 */

#include "packet.h"

// Function to process all incoming packets
void handle_AllPacket(uint8_t *buf, uint16_t plen) {
    // Check if the packet is an ARP packet
    if (buf[ETH_TYPE_H_P] == ETHTYPE_ARP_H_V && buf[ETH_TYPE_L_P] == ETHTYPE_ARP_L_V) {
        // Handle ARP packet
        if ((waitgwmac & WGW_INITIAL_ARP || waitgwmac & WGW_REFRESHING) && delaycnt == 0 && enc28j60linkup()) {
            client_arp_whohas(buf, gwip); // Send ARP request for gateway IP
        }
        delaycnt++;
        if (buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REQ_L_V) {
            make_arp_answer_from_request(buf); // Respond to ARP request
        }
        if (waitgwmac & WGW_ACCEPT_ARP_REPLY && buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REPLY_L_V) {
            if (client_store_gw_mac(buf)) {
                waitgwmac = WGW_HAVE_GW_MAC; // Store gateway MAC address
            }
        }
        return;
    }

    // Check if the packet is an IP packet addressed to us
    if (!eth_type_is_ip_and_my_ip(buf, plen)) {
        return;
    }

    // Check Ethernet type
    if (buf[ETH_TYPE_H_P] == ETHTYPE_IP_H_V && buf[ETH_TYPE_L_P] == ETHTYPE_IP_L_V) {
        // It's an IP packet, check the IP protocol
        uint8_t ip_proto = buf[IP_PROTO_P];

        if (ip_proto == IP_PROTO_ICMP_V) {
            // Handle ICMP packet
            handle_icmp_packet(buf, plen);
        } else if (ip_proto == IP_PROTO_TCP_V) {
            // Handle TCP packet
            handle_tcp_packet(buf, plen);
        } else if (ip_proto == IP_PROTO_UDP_V) {
            // Handle UDP packet
            handle_udp_packet(buf, plen);
        }
    }
}

// Function to handle ICMP packets
void handle_icmp_packet(uint8_t *buf, uint16_t plen) {
    // Process ICMP packet (e.g., reply to ping)
    if (icmp_callback) {
        icmp_callback(&buf[IP_SRC_IP_P]);
    }
    make_echo_reply_from_request(buf, plen);
    pingCallback();
}

// Déclarations Globales
static uint8_t tcp_state = TCP_STATE_CLOSED;

void handle_tcp_response(uint8_t *buf) {
    uint8_t connection_id = 0; // Assuming single connection for simplicity
    tcp_connection_t *conn = &tcp_connections[connection_id];

    if (conn->state == TCP_STATE_SYN_SENT && (buf[33] & TCP_FLAGS_SYN_V) && (buf[33] & TCP_FLAGS_ACK_V)) {
        conn->ack_num = ((uint32_t)buf[24] << 24) | ((uint32_t)buf[25] << 16) | ((uint32_t)buf[26] << 8) | buf[27];
        conn->seq_num++;
        conn->state = TCP_STATE_ESTABLISHED;
        udpLog2("TCP_client", "Connection Established");

        // Send HTTP GET request
        const char *http_get = "GET / HTTP/1.1\r\nHost: 192.168.1.1\r\n\r\n";
        uint8_t buf[200];
        memcpy(buf + 38, http_get, strlen(http_get));
        send_tcp_ack_packet(buf, conn, conn->remote_ip, strlen(http_get));
    }
}

void loop() {
    uint8_t buf[60];
    // Simulate receiving a TCP response packet (this should be replaced with actual packet receiving code)
    buf[33] = TCP_FLAGS_SYN_V | TCP_FLAGS_ACK_V;
    buf[24] = 0; buf[25] = 0; buf[26] = 0; buf[27] = 2; // ACK number
    handle_tcp_response(buf);
}


// Function to handle TCP packets
void handle_tcp_packet(uint8_t *buf, uint16_t plen) {
    uint8_t flags = buf[TCP_FLAGS_P];
    uint32_t seq_num = (buf[TCP_SEQ_H_P] << 24) | (buf[TCP_SEQ_H_P + 1] << 16) | (buf[TCP_SEQ_H_P + 2] << 8) | buf[TCP_SEQ_H_P + 3];
    uint32_t ack_num = (buf[TCP_ACK_H_P] << 24) | (buf[TCP_ACK_H_P + 1] << 16) | (buf[TCP_ACK_H_P + 2] << 8) | buf[TCP_ACK_H_P + 3];
    uint8_t src_ip[4] = { buf[IP_SRC_IP_P], buf[IP_SRC_IP_P + 1], buf[IP_SRC_IP_P + 2], buf[IP_SRC_IP_P + 3] };
    uint16_t src_port = (buf[TCP_SRC_PORT_H_P] << 8) | buf[TCP_SRC_PORT_L_P];

    char log_buffer[100];
    snprintf(log_buffer, sizeof(log_buffer), "TCP packet received: SEQ=%u, ACK=%u, FLAGS=%02X", seq_num, ack_num, flags);
    udpLog2("TCP", log_buffer);

    tcp_connection_t *tcp_conn = find_connection(src_ip, src_port);

    if (tcp_conn == NULL) {
        if (flags & TCP_FLAGS_SYN_V) {
            tcp_conn = find_free_connection();
            if (tcp_conn != NULL) {
                tcp_conn->state = TCP_STATE_SYN_RECEIVED;
                tcp_conn->seq_num = seq_num;
                tcp_conn->ack_num = ack_num;
                memcpy(tcp_conn->remote_ip, src_ip, 4);
                tcp_conn->remote_port = src_port;
                create_tcp_synack_packet(buf, tcp_conn);
                udpLog2("TCP", "SYN received, SYN-ACK sent");
            } else {
                udpLog2("TCP", "No free connection slot available");
            }
        }
        return;
    }

    switch (tcp_conn->state) {
        case TCP_STATE_SYN_RECEIVED:
            if (flags & TCP_FLAGS_ACK_V) {
                tcp_conn->state = TCP_STATE_ESTABLISHED;
                udpLog2("TCP", "Connection established");
            }
            break;

        case TCP_STATE_ESTABLISHED:
            if (flags & TCP_FLAGS_FIN_V) {
                tcp_conn->state = TCP_STATE_CLOSED;
                create_tcp_ack_packet(buf, tcp_conn, 0, 0);
                udpLog2("TCP", "FIN received, ACK sent, connection closed");
            } else if (flags & TCP_FLAGS_ACK_V) {
                if (flags & TCP_FLAGS_PUSH_V) {
                    handle_http_request(buf, TCP_DATA_P, plen - TCP_DATA_P, tcp_conn);
                } else {
                    create_tcp_ack_packet(buf, tcp_conn, 0, 0);
                    udpLog2("TCP", "ACK received, no data to process");
                }
            }
            break;

        default:
            udpLog2("TCP", "Unknown state, resetting connection");
            tcp_conn->state = TCP_STATE_CLOSED;
            create_tcp_ack_packet(buf, tcp_conn, 0, TCP_FLAGS_RST_V);
            break;
    }
}


// Function to handle UDP packets
void handle_udp_packet(uint8_t *buf, uint16_t plen) {
    if (buf[UDP_SRC_PORT_H_P] == 0 && buf[UDP_SRC_PORT_L_P] == 0x7b) {
        client_ntp_process_answer(buf, plen);
        return;
    }

    if (buf[UDP_SRC_PORT_H_P] == 0 && buf[UDP_SRC_PORT_L_P] == 53) {
        return;
    }

    uint16_t data_pos = UDP_DATA_P;
    uint8_t *data = &buf[data_pos];
    uint16_t data_len = plen - data_pos;

    char msg[data_len + 1];
    memcpy(msg, data, data_len);
    msg[data_len] = '\0';

    // Vérifier si le message reçu correspond à une commande dans la table des commandes
    for (CommandMapping *cmd = commandTable; cmd->command != NULL; cmd++) {
        if (strncmp(msg, cmd->command, strlen(cmd->command)) == 0) {
            udpLog2("handle_udp_packet", cmd->command); // Log de la commande reconnue
            if (cmd->function_with_arg != NULL) {
                cmd->function_with_arg(msg); // Passer la commande entière pour traitement
            } else if (cmd->function != NULL) {
                cmd->function();
            }
            return;
        }
    }

    udpLog2("Error", "Unexpected UDP message");
}


// Function to handle HTTP requests
void handle_http_request(uint8_t *buf, uint16_t data_offset, uint16_t data_len, tcp_connection_t *tcp_conn) {
    // Extract the HTTP request from the TCP payload
    char *http_request = (char *)(buf + data_offset);
    udpLog2("HTTP", "HTTP request received");

    // Log the received HTTP request
    char request_log[100];
    snprintf(request_log, sizeof(request_log), "HTTP Request: %.50s", http_request);
    udpLog2("HTTP", request_log);

    // Check if the HTTP request is a GET request for "/logs"
    if (strncmp(http_request, "GET /logs", 9) == 0) {
        uint16_t response_len = send_logs_page(buf);
        www_server_reply(buf, response_len, tcp_conn);
        udpLog2("HTTP", "Logs page sent");
    } else if (strncmp(http_request, "GET /test", 9) == 0) {
        uint16_t response_len = send_test_page(buf);
        www_server_reply(buf, response_len, tcp_conn);
        udpLog2("HTTP", "Test page sent");
    } else {
        // Handle other HTTP requests or send a 404 Not Found response
        const char *not_found_response = "HTTP/1.1 404 Not Found\r\n"
                                         "Content-Type: text/html\r\n"
                                         "Content-Length: 13\r\n"
                                         "Connection: close\r\n"
                                         "\r\n"
                                         "404 Not Found";
        uint16_t response_len = strlen(not_found_response);
        memcpy(buf + TCP_DATA_P, not_found_response, response_len);
        www_server_reply(buf, response_len, tcp_conn);
        udpLog2("HTTP", "404 Not Found response sent");
    }
}
