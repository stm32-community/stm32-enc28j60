/*
 * @file logmanager.c
 * @brief Source file
 *
 * This header file defines the interface for the Log Manager module, which handles logging
 * operations for different types of logs such as UDP, Web, and UART. It includes an enumeration
 * for log types, function prototypes for logging messages, initializing the log manager, and
 * adding logs to the system.
 */

#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include "main.h"

// Enumeration for log types
typedef enum {
    LOG_UDP,
    LOG_WEB,
    LOG_UART
    // Add more log types as needed
} LogType;

// Function prototypes
void ethershieldDebug(char *message);
void udpLog2(char* alerte, char* text);
void add_log(const char *log);
void logMessage(LogType type, const char *message);
void initLogManager(LogType type);

#endif /* LOGMANAGER_H */
