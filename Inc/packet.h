/**
 * @file packet.h
 * @brief Header file containing function prototypes for processing Ethernet packets, including TCP, UDP, ICMP, and HTTP protocols.
 *
 * This file declares functions for handling various types of network packets, including Ethernet, TCP, UDP,
 * ICMP, and HTTP. It provides the necessary prototypes for managing packet processing, handling protocol-specific
 * tasks, and maintaining network communication states.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
 */

#ifndef PACKET_H
#define PACKET_H

#include "main.h"
#include "defines.h"
#include "net.h"



void handle_AllPacket(uint8_t *buf, uint16_t len);

void handle_icmp_packet(uint8_t *buf, uint16_t len);
void handle_tcp_response(uint8_t *buf);
void loop();
void handle_tcp_packet(uint8_t *buf, uint16_t len);
void handle_udp_packet(uint8_t *buf, uint16_t len);
void handle_http_request(uint8_t *buf, uint16_t data_offset, uint16_t data_len, tcp_connection_t *tcp_conn);
#endif
