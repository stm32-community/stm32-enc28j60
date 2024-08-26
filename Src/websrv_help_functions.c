/*
 * websrv_help_functions.C
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */

#include "defines.h"
#include "websrv_help_functions.h"
#include "tcp.h"
#include "net.h"
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"

#ifdef FROMDECODE_websrv_help

typedef void (*browser_callback_t)(uint8_t, uint16_t, uint16_t);
static browser_callback_t client_browser_callback = NULL;

// Function to send the logs page
uint16_t send_logs_page(uint8_t *buf) {
    const char *logs_page = "<html><head><title>Logs</title></head><body><h1>System Logs</h1><p>Log content goes here</p></body></html>";
    uint16_t len = strlen(logs_page);
    memcpy(buf + TCP_DATA_P, logs_page, len);
    return len;
}

// Function to send a test page
uint16_t send_test_page(uint8_t *buf) {
    const char *test_page = "<html><head><title>Test</title></head><body><h1>Test Page</h1><p>Test content goes here</p></body></html>";
    uint16_t len = strlen(test_page);
    memcpy(buf + TCP_DATA_P, test_page, len);
    return len;
}

// Function to send an HTTP response
void www_server_reply(uint8_t *buf, uint16_t dlen, tcp_connection_t *tcp_conn) {
    uint16_t ck;
    uint8_t i;

    // Setup the TCP header
    buf[TCP_FLAGS_P] = TCP_FLAGS_ACK_V | TCP_FLAGS_PUSH_V | TCP_FLAGS_FIN_V; // FIN, ACK, and PSH flags
    buf[TCP_SEQACK_P] = (tcp_conn->ack_num >> 24) & 0xFF;
    buf[TCP_SEQACK_P + 1] = (tcp_conn->ack_num >> 16) & 0xFF;
    buf[TCP_SEQACK_P + 2] = (tcp_conn->ack_num >> 8) & 0xFF;
    buf[TCP_SEQACK_P + 3] = tcp_conn->ack_num & 0xFF;
    tcp_conn->seq_num += dlen;
    buf[TCP_SEQ_P] = (tcp_conn->seq_num >> 24) & 0xFF;
    buf[TCP_SEQ_P + 1] = (tcp_conn->seq_num >> 16) & 0xFF;
    buf[TCP_SEQ_P + 2] = (tcp_conn->seq_num >> 8) & 0xFF;
    buf[TCP_SEQ_P + 3] = tcp_conn->seq_num & 0xFF;

    // TCP header length is 20 bytes
    buf[TCP_HEADER_LEN_P] = 0x50;

    // Set the window size
    buf[TCP_WINDOWSIZE_H_P] = 0x05;
    buf[TCP_WINDOWSIZE_L_P] = 0xB4;

    // Calculate and set the checksum
    ck = 0;
    // Pseudo header part 1: source and destination IPs
    for (i = 0; i < 4; i++) {
        ck += buf[IP_SRC_IP_P + i];
        ck += buf[IP_DST_IP_P + i];
    }
    // Pseudo header part 2: protocol and TCP length
    ck += IP_PROTO_TCP_V + (TCP_HEADER_LEN_PLAIN + dlen);
    // TCP header and data
    for (i = IP_HEADER_LEN + ETH_HEADER_LEN; i < IP_HEADER_LEN + ETH_HEADER_LEN + TCP_HEADER_LEN_PLAIN + dlen; i += 2) {
        ck += (uint16_t)((buf[i] << 8) | buf[i + 1]);
    }
    while (ck >> 16) {
        ck = (ck & 0xFFFF) + (ck >> 16);
    }
    ck = ~ck;
    buf[TCP_CHECKSUM_H_P] = ck >> 8;
    buf[TCP_CHECKSUM_L_P] = ck & 0xFF;

    // Send the packet
    enc28j60PacketSend(IP_HEADER_LEN + ETH_HEADER_LEN + TCP_HEADER_LEN_PLAIN + dlen, buf);
}

uint16_t www_client_internal_datafill_callback(uint8_t fd) {
    if (fd != www_fd) {
        return 0;
    }

    char strbuf[6];
    uint16_t len = 0;

    if (browsertype == 0) {
        // GET Request
        len = fill_tcp_data(bufptr, len, (uint8_t *)"GET ", strlen("GET "));
        len = fill_tcp_data(bufptr, len, (uint8_t *)client_urlbuf, strlen(client_urlbuf));
        if (client_urlbuf_var) {
            len = fill_tcp_data(bufptr, len, (uint8_t *)client_urlbuf_var, strlen(client_urlbuf_var));
        }
        len = fill_tcp_data(bufptr, len, (uint8_t *)" HTTP/1.1\r\nHost: ", strlen(" HTTP/1.1\r\nHost: "));
        len = fill_tcp_data(bufptr, len, (uint8_t *)client_hoststr, strlen(client_hoststr));
        len = fill_tcp_data(bufptr, len, (uint8_t *)"\r\nUser-Agent: " WWW_USER_AGENT "\r\nAccept: text/html\r\nConnection: close\r\n\r\n", strlen("\r\nUser-Agent: " WWW_USER_AGENT "\r\nAccept: text/html\r\nConnection: close\r\n\r\n"));
    } else {
        // POST Request
        len = fill_tcp_data(bufptr, len, (uint8_t *)(client_method ? client_method : "POST "), strlen(client_method ? client_method : "POST "));
        len = fill_tcp_data(bufptr, len, (uint8_t *)client_urlbuf, strlen(client_urlbuf));
        len = fill_tcp_data(bufptr, len, (uint8_t *)" HTTP/1.1\r\nHost: ", strlen(" HTTP/1.1\r\nHost: "));
        len = fill_tcp_data(bufptr, len, (uint8_t *)client_hoststr, strlen(client_hoststr));

        if (client_additionalheaderline) {
            len = fill_tcp_data(bufptr, len, (uint8_t *)"\r\n", strlen("\r\n"));
            len = fill_tcp_data(bufptr, len, (uint8_t *)client_additionalheaderline, strlen(client_additionalheaderline));
        }

        len = fill_tcp_data(bufptr, len, (uint8_t *)"\r\nUser-Agent: " WWW_USER_AGENT "\r\nAccept: */*\r\nConnection: close\r\n", strlen("\r\nUser-Agent: " WWW_USER_AGENT "\r\nAccept: */*\r\nConnection: close\r\n"));
        len = fill_tcp_data(bufptr, len, (uint8_t *)"Content-Length: ", strlen("Content-Length: "));
        snprintf(strbuf, sizeof(strbuf), "%zu", strlen(client_postval));
        len = fill_tcp_data(bufptr, len, (uint8_t *)strbuf, strlen(strbuf));
        len = fill_tcp_data(bufptr, len, (uint8_t *)"\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n", strlen("\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n"));
        len = fill_tcp_data(bufptr, len, (uint8_t *)client_postval, strlen(client_postval));
    }

    // Logging the final request for debugging
    udpLog2("WWW_client", "Generated HTTP request:");
    udpLog2("WWW_client", (char *)bufptr);

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
    char log_msg[50];

    // Log the entry into the function
    udpLog2("WWW_client", "Entering www_client_internal_result_callback");

    // Log the parameters received
    snprintf(log_msg, sizeof(log_msg), "FD: %d, Status: %d, DataPos: %d, Len: %d", fd, statuscode, datapos, len_of_data);
    udpLog2("WWW_client", log_msg);

    if (fd != www_fd) {
        udpLog2("WWW_client", "File descriptor mismatch");
        client_browser_callback(4, 0, 0);
        return 0;
    }

    if (statuscode == 0 && len_of_data > 12 && client_browser_callback) {
        uint16_t offset = TCP_SRC_PORT_H_P + ((bufptr[TCP_HEADER_LEN_P] >> 4) * 4);
        uint8_t *status_code_pos = &bufptr[datapos + 9];

        // Log the status code
        snprintf(log_msg, sizeof(log_msg), "Status Code: %.3s", status_code_pos);
        udpLog2("WWW_client", log_msg);

        if (strncmp("200", (char *)status_code_pos, 3) == 0) {
            udpLog2("WWW_client", "HTTP 200 OK");
            client_browser_callback(0, offset, len_of_data);
        } else {
            udpLog2("WWW_client", "HTTP Error");
            client_browser_callback(1, offset, len_of_data);
        }
    } else {
        udpLog2("WWW_client", "Invalid status or data length");
    }

    udpLog2("WWW_client", "Exiting www_client_internal_result_callback");
    return 0;
}


void client_http_get(char *urlbuf, char *urlbuf_varpart, char *hoststr, void (*callback)(uint8_t, uint16_t, uint16_t)) {
    client_urlbuf = urlbuf;
    client_urlbuf_var = urlbuf_varpart;
    client_hoststr = hoststr;
    browsertype = 0;
    client_browser_callback = callback;
    www_fd = client_tcp_req(www_client_internal_result_callback, www_client_internal_datafill_callback, 80);
}

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

// search for a string of the form key=value in
// a string that looks like q?xyz=abc&uvw=defgh HTTP/1.1\r\n
//
// The returned value is stored in strbuf. You must allocate
// enough storage for strbuf, maxlen is the size of strbuf.
// I.e the value it is declated with: strbuf[5]-> maxlen=5
uint8_t find_key_val(char *str, char *strbuf, uint8_t maxlen, char *key) {
    uint8_t found = 0;
    uint8_t i = 0;
    char *kp = key;

    // Find the key in the string
    while (*str && *str != ' ' && *str != '\n' && !found) {
        if (*str == *kp) {
            kp++;
            if (*kp == '\0' && *(str + 1) == '=') {
                found = 1;
                str += 2; // Move past '='
            }
        } else {
            kp = key;
        }
        str++;
    }

    // Copy the value to the buffer if key is found
    if (found) {
        while (*str && *str != ' ' && *str != '\n' && *str != '&' && i < maxlen - 1) {
            strbuf[i++] = *str++;
        }
        strbuf[i] = '\0';
    }

    // Return the length of the value
    return i;
}

// Convert a single hex digit character to its integer value
unsigned char h2int(char c) {
    if (c >= '0' && c <= '9') {
        return (unsigned char)(c - '0');
    } else if (c >= 'a' && c <= 'f') {
        return (unsigned char)(c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
        return (unsigned char)(c - 'A' + 10);
    } else {
        return 0;
    }
}


// Decode a URL string, e.g., "hello%20joe" or "hello+joe" becomes "hello joe"
void urldecode(char *urlbuf) {
    char c;
    char *dst = urlbuf;

    while ((c = *urlbuf)) {
        if (c == '+') {
            c = ' ';
        } else if (c == '%') {
            urlbuf++;
            char high = *urlbuf++;
            char low = *urlbuf;
            c = (h2int(high) << 4) | h2int(low);
        }
        *dst++ = c;
        urlbuf++;
    }
    *dst = '\0';
}
#endif //  FROMDECODE_websrv_help

#ifdef URLENCODE_websrv_help
// Convert a single character to a 2-digit hex string
// A terminating '\0' is added
void int2h(char c, char *hstr) {
    // Convert the lower 4 bits
    hstr[1] = (c & 0xF) + '0';
    if ((c & 0xF) > 9) {
        hstr[1] = (c & 0xF) - 10 + 'a';
    }

    // Convert the upper 4 bits
    c = (c >> 4) & 0xF;
    hstr[0] = c + '0';
    if (c > 9) {
        hstr[0] = c - 10 + 'a';
    }

    // Null-terminate the string
    hstr[2] = '\0';
}

// There must be enough space in urlbuf. In the worst case, that is
// 3 times the length of str.
void urlencode(char *str, char *urlbuf) {
    char c;
    while ((c = *str)) {
        if (c == ' ' || isalnum((unsigned char)c)) {
            if (c == ' ') {
                c = '+';
            }
            *urlbuf++ = c;
        } else {
            *urlbuf++ = '%';
            int2h(c, urlbuf);
            urlbuf += 2;
        }
        str++;
    }
    *urlbuf = '\0';
}
#endif // URLENCODE_websrv_help
// Parse a string and extract the IP to bytestr
uint8_t parse_ip(uint8_t *bytestr, char *str) {
    char *sptr = NULL;
    uint8_t i = 0;

    // Initialize bytestr to zero
    for (i = 0; i < 4; i++) {
        bytestr[i] = 0;
    }

    i = 0;
    while (*str && i < 4) {
        if (sptr == NULL && isdigit((unsigned char)*str)) {
            sptr = str;
        }
        if (*str == '.') {
            *str = '\0';
            bytestr[i] = (uint8_t)(atoi(sptr) & 0xff);
            i++;
            sptr = NULL;
        }
        str++;
    }

    if (i == 3 && sptr != NULL) {
        bytestr[i] = (uint8_t)(atoi(sptr) & 0xff);
        return 0;
    }
    return 1;
}

// Convert a byte string to a human-readable display string
// (base is 10 for IP and 16 for MAC address), len is 4 for IP address and 6 for MAC.
void mk_net_str(char *resultstr, uint8_t *bytestr, uint8_t len, char separator, uint8_t base) {
    uint8_t i, j = 0;

    for (i = 0; i < len; i++) {
        if (base == 10) {
            j += sprintf(&resultstr[j], "%d", bytestr[i]);
        } else if (base == 16) {
            j += sprintf(&resultstr[j], "%02x", bytestr[i]);
        }
        resultstr[j++] = separator;
    }
    resultstr[j - 1] = '\0';  // Replace the last separator with null terminator
}

// end of websrv_help_functions.c
