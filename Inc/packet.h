/*
 * packet.h
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
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
