/*
 * @file tcp.h
 * @brief Header file
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */

#ifndef TCP_H
#define TCP_H

#include "main.h"
//#include "defines.h"
#include "net.h"

// Callback function types for handling TCP results and data fill
typedef uint8_t (*tcp_result_callback_t)(uint8_t fd, uint8_t statuscode, uint16_t data_start_pos_in_buf, uint16_t len_of_data);
typedef uint16_t (*tcp_datafill_callback_t)(uint8_t fd);

// Static variables to store the callbacks
extern tcp_result_callback_t client_tcp_result_callback;
extern tcp_datafill_callback_t client_tcp_datafill_callback;

//-------------------------------------- STEP1
void client_tcp_set_serverip(uint8_t *ipaddr);
void initialize_tcp_sequence_number();
void init_tcp_connections();
tcp_connection_t* find_free_connection();
tcp_connection_t* find_connection(uint8_t *ip, uint16_t port);
//-------------------------------------- STEP2
void tcp_open_connection(uint8_t *src_ip, uint16_t src_port, uint8_t *dst_ip, uint16_t dst_port);
void make_tcphead(uint8_t *buf, uint16_t len, uint8_t mss, uint8_t win_size);
static uint16_t TCP_htons(uint16_t hostshort);
void make_ip_tcp_new(uint8_t *buf, uint16_t len, uint8_t *dst_ip);
void create_tcp_synack_packet(uint8_t *buf, tcp_connection_t *tcp_conn);
void create_tcp_ack_packet(uint8_t *buf, tcp_connection_t *tcp_conn, uint8_t mss, uint8_t win_size);
void create_tcp_ack_with_data_packet(uint8_t *buf,uint16_t len);
//void tcp_close_connection(tcp_connection_t *conn);
//-------------------------------------- STEP3
uint16_t get_tcp_data_len(uint8_t *buf);
uint16_t fill_tcp_data(uint8_t *buf, uint16_t pos, const uint8_t *data, uint16_t len);
uint16_t build_tcp_data(uint8_t *buf, uint16_t srcPort);
void send_tcp_data(uint8_t *buf, uint16_t dlen);
void send_data_packet(uint8_t *buf, uint16_t srcPort, const uint8_t *data, uint16_t len);
void send_tcp_fin_packet(uint8_t *buf);
void tcp_send_packet(uint32_t seq_num, uint32_t ack_num);
//-------------------------------------- STEP4
uint16_t prepare_tcp_data(uint8_t *buf, uint16_t pos, const uint8_t *data, uint16_t len);
void transmit_tcp_data(uint8_t *buf, uint16_t dlen);
//-------------------------------------- STEP6
void tcp_handle_retransmissions();
void tcp_state_machine(uint8_t *buf, uint16_t len);
//-------------------------------------- OTHERS
void client_syn(uint8_t *buf, uint8_t srcport, uint8_t dstport_h, uint8_t dstport_l);
void make_tcp_ack_with_data_noflags(uint8_t *buf,uint16_t dlen);

uint8_t client_tcp_req(uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),uint16_t (*datafill_callback)(uint8_t fd),uint16_t port);

void fill_ip_tcp_checksum(uint8_t *buf, uint16_t len);
void fill_tcp_ip_hdr_checksum(uint8_t *buf);
uint16_t calculate_tcp_checksum(uint8_t *buf, uint16_t len);
void send_tcp_syn_packet(uint8_t *buf, uint16_t srcPort, uint16_t dstPort, uint8_t *dst_ip);

void log_ip_header(uint8_t *buf);
void log_tcp_header(uint8_t *buf);

#endif
