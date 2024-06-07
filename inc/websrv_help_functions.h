/*
 * websrv_help_functions.h
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */

#ifndef WEBSRV_HELP_FUNCTIONS_H
#define WEBSRV_HELP_FUNCTIONS_H

#include "defines.h"

// Function declarations

/**
 * Find the value for a given key in a query string.
 * @param str The input query string.
 * @param strbuf The buffer to store the value.
 * @param maxlen The maximum length of the value to store.
 * @param key The key to search for.
 * @return Length of the value found.
 */
uint8_t find_key_val(char *str, char *strbuf, uint8_t maxlen, char *key);

/**
 * Decode a URL-encoded string.
 * @param urlbuf The URL-encoded string to decode.
 */
void urldecode(char *urlbuf);

/**
 * Encode a string to URL-encoded format.
 * @param str The input string to encode.
 * @param urlbuf The buffer to store the URL-encoded string.
 */
void urlencode(char *str, char *urlbuf);

/**
 * Parse an IP address string into a byte array.
 * @param bytestr The byte array to store the parsed IP address.
 * @param str The input IP address string.
 * @return 1 if successful, 0 otherwise.
 */
uint8_t parse_ip(uint8_t *bytestr, char *str);

/**
 * Create a network string from a byte array.
 * @param resultstr The buffer to store the resulting string.
 * @param bytestr The input byte array.
 * @param len The length of the byte array.
 * @param separator The character to use as a separator.
 * @param base The numerical base for conversion (e.g., 10 for decimal).
 */
void mk_net_str(char *resultstr, uint8_t *bytestr, uint8_t len, char separator, uint8_t base);

#endif /* WEBSRV_HELP_FUNCTIONS_H */
