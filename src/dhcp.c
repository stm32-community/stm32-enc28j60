/*
 * ENC28J60.c
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file contains definitions and macros for interfacing with the ENC28J60 Ethernet controller.
 * It includes control register definitions, chip enable/disable macros, and configurations for delays and
 * chip select (CS) handling.
 */

#include "dhcp.h"
#include "net.h"

#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTREPLY 2
#define DHCPDISCOVER 0x01
#define DHCPOFFER 0x02
#define DHCPREQUEST 0x03
#define DHCPACK 0x05
// #define DHCPNACK // Uncomment if DHCPNACK is needed

// Source port high byte must be a0 or higher:
#define DHCPCLIENT_SRC_PORT_H 0xe0
#define DHCP_SRC_PORT 67
#define DHCP_DEST_PORT 68

// DHCP data structure (size 236 bytes)
typedef struct dhcpData {
    uint8_t op;              // 0-3: op, htype, hlen, hops
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;            // 4-7: xid
    uint16_t secs;           // 8-9: secs
    uint16_t flags;          // 10-11: flags
    uint8_t ciaddr[4];       // 12-15: ciaddr
    uint8_t yiaddr[4];       // 16-19: yiaddr
    uint8_t siaddr[4];       // 20-23: siaddr
    uint8_t giaddr[4];       // 24-27: giaddr
    uint8_t chaddr[16];      // 28-43: chaddr
    uint8_t sname[64];       // 44-107: sname
    uint8_t file[128];       // 108-235: file
} __attribute__((packed)) dhcpData;

// Static variables for DHCP
static uint8_t dhcptid_l = 0;               // Counter for transaction ID
static uint8_t dhcpState = DHCP_STATE_INIT; // DHCP state

// Pointers to values we have set or need to set
static uint8_t *macaddr;    // MAC address pointer
uint8_t *dhcpip;            // DHCP IP address pointer
uint8_t *dhcpmask;          // DHCP subnet mask pointer
uint8_t *dhcpserver;        // DHCP server address pointer
uint8_t *dnsserver;         // DNS server address pointer
uint8_t *gwaddr;            // Gateway address pointer

// Hostname definitions
#define HOSTNAME_LEN DHCP_HOSTNAME_LEN
#define HOSTNAME_SIZE (HOSTNAME_LEN + 3)    // Length + 3 for additional characters

static char hostname[HOSTNAME_SIZE];        // Hostname buffer

// DHCP response variables
static uint8_t haveDhcpAnswer = 0;          // Flag for DHCP answer
static uint8_t dhcp_ansError = 0;           // DHCP answer error flag
uint32_t currentXid = 0;                    // Current transaction ID
uint16_t currentSecs = 0;                   // Current seconds since DHCP start
static uint32_t leaseStart = 0;             // Lease start time
static uint32_t leaseTime = 0;              // Lease duration
static uint8_t *bufPtr;                     // Buffer pointer

static void addToBuf(uint8_t b) {
    *bufPtr++ = b;
}

uint8_t dhcp_state(void) {
    // Check lease and request renew if currently OK and time
    // leaseStart - start time in millis
    // leaseTime - length of lease in millis
    if (dhcpState == DHCP_STATE_OK && (leaseStart + leaseTime) <= HAL_GetTick()) {
        // Calling app needs to detect this and initiate renewal
        dhcpState = DHCP_STATE_RENEW;
    }
    return dhcpState;
}

// Start request sequence, send DHCPDISCOVER
// Wait for DHCPOFFER
// Send DHCPREQUEST
// Wait for DHCPACK
// All configured
void dhcp_start(uint8_t *buf, uint8_t *macaddrin, uint8_t *ipaddrin, uint8_t *maskin,
                uint8_t *gwipin, uint8_t *dhcpsvrin, uint8_t *dnssvrin) {
    macaddr = macaddrin;
    dhcpip = ipaddrin;
    dhcpmask = maskin;
    gwaddr = gwipin;
    dhcpserver = dhcpsvrin;
    dnsserver = dnssvrin;

    // Initialize random seed and transaction ID
    srand(0x13);
    currentXid = 0x00654321 + rand();
    currentSecs = 0;

    // Clear IP, mask, gateway, DHCP server, and DNS server addresses
    memset(dhcpip, 0, 4);
    memset(dhcpmask, 0, 4);
    memset(gwaddr, 0, 4);
    memset(dhcpserver, 0, 4);
    memset(dnsserver, 0, 4);

    // Set a unique hostname, use DHCP_HOSTNAME- plus last octet of MAC address
    strncpy(hostname, DHCP_HOSTNAME, HOSTNAME_LEN);
    hostname[HOSTNAME_LEN] = '-';
    hostname[HOSTNAME_LEN + 1] = 'A' + (macaddr[5] >> 4);
    hostname[HOSTNAME_LEN + 2] = 'A' + (macaddr[5] & 0x0F);
    hostname[HOSTNAME_LEN + 3] = '\0';

    // Enable reception of broadcast packets
    enc28j60EnableBroadcast();

    // Send DHCPDISCOVER packet
    dhcp_send(buf, DHCPDISCOVER);
    dhcpState = DHCP_STATE_DISCOVER;
}

void dhcp_request_ip(uint8_t *buf) {
    // Send DHCPREQUEST packet
    dhcp_send(buf, DHCPREQUEST);

    // Update the DHCP state to REQUEST
    dhcpState = DHCP_STATE_REQUEST;
}

// Main DHCP message sending function, either DHCPDISCOVER or DHCPREQUEST
void dhcp_send(uint8_t *buf, uint8_t requestType) {
    haveDhcpAnswer = 0;
    dhcp_ansError = 0;
    dhcptid_l++;  // Increment for next request, finally wrap

    // Clear the buffer
    memset(buf, 0, 400);

    // Prepare the UDP packet
    send_udp_prepare(buf, (DHCPCLIENT_SRC_PORT_H << 8) | (dhcptid_l & 0xff), dhcpip, DHCP_DEST_PORT);

    // Set Ethernet source and destination MAC addresses
    memcpy(buf + ETH_SRC_MAC, macaddr, 6);
    memset(buf + ETH_DST_MAC, 0xFF, 6);

    // Set IP total length, protocol, and destination
    buf[IP_TOTLEN_L_P] = 0x82;
    buf[IP_PROTO_P] = IP_PROTO_UDP_V;
    memset(buf + IP_DST_P, 0xFF, 4);

    // Set UDP source and destination ports
    buf[UDP_DST_PORT_L_P] = DHCP_SRC_PORT;
    buf[UDP_SRC_PORT_H_P] = 0;
    buf[UDP_SRC_PORT_L_P] = DHCP_DEST_PORT;

    // Build DHCP packet from buf[UDP_DATA_P]
    dhcpData *dhcpPtr = (dhcpData *)&buf[UDP_DATA_P];

    // 0-3: op, htype, hlen, hops
    dhcpPtr->op = DHCP_BOOTREQUEST;
    dhcpPtr->htype = 1;
    dhcpPtr->hlen = 6;
    dhcpPtr->hops = 0;

    // 4-7: xid
    memcpy(&dhcpPtr->xid, &currentXid, sizeof(currentXid));

    // 8-9: secs
    memcpy(&dhcpPtr->secs, &currentSecs, sizeof(currentSecs));

    // 16-19: yiaddr
    memset(dhcpPtr->yiaddr, 0, 4);

    // 28-43: chaddr (16)
    memcpy(dhcpPtr->chaddr, macaddr, 6);

    // Options defined as option, length, value
    bufPtr = buf + UDP_DATA_P + sizeof(dhcpData);

    // Magic cookie: 99, 130, 83, 99
    const uint8_t magicCookie[] = {99, 130, 83, 99};
    memcpy(bufPtr, magicCookie, sizeof(magicCookie));
    bufPtr += sizeof(magicCookie);

    // Option 1 - DHCP message type
    addToBuf(53);           // Option: DHCP Message Type
    addToBuf(1);            // Length: 1
    addToBuf(requestType);  // Value: DHCPDISCOVER or DHCPREQUEST

    // Client Identifier Option: client MAC address
    addToBuf(61);    // Option: Client Identifier
    addToBuf(7);     // Length: 7
    addToBuf(0x01);  // Hardware type: Ethernet
    for (int i = 0; i < 6; i++) addToBuf(macaddr[i]);

    // Host name Option
    addToBuf(12);             // Option: Host Name
    addToBuf(HOSTNAME_SIZE);  // Length
    for (int i = 0; i < HOSTNAME_SIZE; i++) addToBuf(hostname[i]);

    if (requestType == DHCPREQUEST) {
        // Request IP address
        addToBuf(50);  // Option: Requested IP Address
        addToBuf(4);   // Length: 4
        for (int i = 0; i < 4; i++) addToBuf(dhcpip[i]);

        // Request using server IP address
        addToBuf(54);  // Option: Server Identifier
        addToBuf(4);   // Length: 4
        for (int i = 0; i < 4; i++) addToBuf(dhcpserver[i]);
    }

    // Additional information in parameter request list
    addToBuf(55);  // Option: Parameter Request List
    addToBuf(3);   // Length: 3
    addToBuf(1);   // Parameter: Subnet Mask
    addToBuf(3);   // Parameter: Router
    addToBuf(6);   // Parameter: DNS Server

    // End option
    addToBuf(255);

    // Transmit the UDP packet
    send_udp_transmit(buf, bufPtr - buf - UDP_DATA_P);
}

// Examine packet, if DHCP then process, otherwise exit.
// Perform lease expiry checks and initiate renewal.
// Process the answer from the DHCP server:
// Return 1 on successful processing of the answer.
// We also set the variable haveDhcpAnswer to 1 if we process an answer.
// Either DHCPOFFER, DHCPACK, or DHCPNACK.
// Return 0 for nothing processed, 1 for something processed.
uint8_t check_for_dhcp_answer(uint8_t *buf, uint16_t plen) {
    // Map struct onto payload
    dhcpData *dhcpPtr = (dhcpData *)&buf[UDP_DATA_P];

    // Check if the packet is a DHCP reply with the correct transaction ID
    if (plen >= 70 &&
        buf[UDP_SRC_PORT_L_P] == DHCP_SRC_PORT &&
        dhcpPtr->op == DHCP_BOOTREPLY &&
        !memcmp(&dhcpPtr->xid, &currentXid, sizeof(currentXid))) {

        // Check for DHCP message type option
        int optionIndex = UDP_DATA_P + sizeof(dhcpData) + 4;
        if (buf[optionIndex] == 53) {  // Option 53 is DHCP message type
            uint8_t messageType = buf[optionIndex + 2];
            switch (messageType) {
                case DHCPOFFER:
                    return have_dhcpoffer(buf, plen);
                case DHCPACK:
                    return have_dhcpack(buf, plen);
                // You can add more cases here if needed, e.g., DHCPNACK
            }
        }
    }

    return 0;  // No DHCP answer processed
}

uint8_t have_dhcpoffer(uint8_t *buf, uint16_t plen) {
    // Map struct onto payload
    dhcpData *dhcpPtr = (dhcpData *)&buf[UDP_DATA_P];

    // Offered IP address is in yiaddr
    memcpy(dhcpip, dhcpPtr->yiaddr, 4);

    // Scan through variable length option list identifying options we want
    uint8_t *ptr = (uint8_t *)(dhcpPtr + 1) + 4;  // Move past fixed DHCP data and magic cookie

    while (ptr < buf + plen) {
        uint8_t option = *ptr++;
        uint8_t optionLen = *ptr++;

        switch (option) {
            case 1:  // Subnet Mask
                memcpy(dhcpmask, ptr, 4);
                break;
            case 3:  // Router/Gateway
                memcpy(gwaddr, ptr, 4);
                break;
            case 6:  // DNS Server
                memcpy(dnsserver, ptr, 4);
                break;
            case 51:  // IP Address Lease Time
                leaseTime = 0;
                for (uint8_t i = 0; i < 4; i++) {
                    leaseTime = (leaseTime << 8) + ptr[i];
                }
                leaseTime *= 1000;  // Convert to milliseconds
                break;
            case 54:  // DHCP Server Identifier
                memcpy(dhcpserver, ptr, 4);
                break;
            default:
                break;
        }
        ptr += optionLen;
    }

    dhcp_request_ip(buf);
    return 1;
}

uint8_t have_dhcpack(uint8_t *buf, uint16_t plen) {
    // Set DHCP state to OK
    dhcpState = DHCP_STATE_OK;

    // Record the lease start time
    leaseStart = HAL_GetTick();

    // Turn off broadcast. Application can re-enable if needed
    enc28j60DisableBroadcast();

    // Return 2 to indicate DHCPACK processed
    return 2;
}
/* end of dhcp.c */
