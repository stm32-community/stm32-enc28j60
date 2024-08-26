#include "LogManager.h"
#include "EtherShield.h" // Assuming this is where udplog2 is declared


#if ETHERSHIELD_DEBUG
void ethershieldDebug(char *message) {
    printf("%s\r\n", message);
    udpLog2("ETHERSHIELD_DEBUG", message);
}
#endif

void udpLog2(char* alerte, char* text) {
    static uint8_t udp[500];
    char data[450];
    snprintf(data, 450, "Project %s - %s", alerte, text);
    send_udp(udp, data, strlen(data), sport, dip, dport);
}

#define MAX_LOGS 50
#define LOG_LENGTH 80

char logs[MAX_LOGS][LOG_LENGTH];
int log_index = 0;

// Fonction pour ajouter un log
void add_log(const char *log) {
    strncpy(logs[log_index], log, LOG_LENGTH);
    log_index = (log_index + 1) % MAX_LOGS;
}

// Fonction pour récupérer les logs en format HTML
//void get_logs(char *buffer, size_t size) {
//    size_t offset = 0;
//    for (int i = 0; i < MAX_LOGS; i++) {
//        int index = (log_index + i) % MAX_LOGS;
//        offset += snprintf(buffer + offset, size - offset, "%s\n", logs[index]);
//        if (offset >= size) break;
//    }
//}

// Function to log messages based on log type
void logMessage(LogType type, const char *message) {
    switch (type) {
        case LOG_UDP:
            udplog2("Log", message);
            break;
        case LOG_WEB:
            // Implement web logging
            break;
        case LOG_UART:
            // Implement UART logging
            break;
        // Add more cases as needed
        default:
            break;
    }
}

// Function to initialize the log manager for a specific log type
void initLogManager(LogType type) {
    switch (type) {
        case LOG_UDP:
            // Initialize UDP logging if needed
            break;
        case LOG_WEB:
            // Initialize web logging if needed
            break;
        case LOG_UART:
            // Initialize UART logging if needed
            break;
        // Add more cases as needed
        default:
            break;
    }
}
