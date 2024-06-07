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

#ifdef FROMDECODE_websrv_help
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
