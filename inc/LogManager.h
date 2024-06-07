#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include "defines.h"

// Enumeration for log types
typedef enum {
    LOG_UDP,
    LOG_WEB,
    LOG_UART
    // Add more log types as needed
} LogType;

// Function prototypes
void logMessage(LogType type, const char *message);
void initLogManager(LogType type);

#endif /* LOGMANAGER_H */
