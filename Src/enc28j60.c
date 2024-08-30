/**
 * @file ENC28J60.c
 * @brief Source file containing the implementation for interfacing with the ENC28J60 Ethernet controller.
 *
 * This file includes functions for initializing the ENC28J60, handling SPI communication,
 * reading and writing registers, managing buffers, and controlling power states. It also
 * contains functions for sending and receiving Ethernet packets, as well as configuring
 * various network settings.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
 */

#include "defines.h"
#include "enc28j60.h"
#include "error_handler.h"

static uint8_t Enc28j60Bank;
static uint16_t gNextPacketPtr;
static uint8_t erxfcon;
static SPI_HandleTypeDef *hspi = NULL;



void enc28j60_set_spi(SPI_HandleTypeDef *hspi_new) {
	hspi = hspi_new;
}

void error (float error_num, char infinite);

unsigned char ENC28J60_SendByte(uint8_t tx) {
    if (hspi == NULL)
        return 0;

    uint8_t rx = 0;
    int r;

    // Use a reasonable timeout value
    const uint32_t timeout = 1000;  // 1 second timeout

    r = HAL_SPI_TransmitReceive(hspi, &tx, &rx, 1, timeout);

    if (r != HAL_OK) {
        ENC28j60_Error_Handler(SPI_ERROR);
        return 0;  // Return a default value in case of an error
    }

    return rx;
}

uint8_t enc28j60ReadOp(uint8_t op, uint8_t address) {
    uint8_t temp;

    // Enable the chip (CS low)
    enableChip;

    // Issue read command
    ENC28J60_SendByte(op | (address & ADDR_MASK));

    // Read the data
    temp = ENC28J60_SendByte(0xFF);

    // If this is a MAC or MII register, read one more byte
    if (address & 0x80)
        temp = ENC28J60_SendByte(0xFF);

    // Disable the chip (CS high)
    disableChip;

    return temp;
}

void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data) {
    // Enable the chip (CS low)
    enableChip;

    // Issue write command
    ENC28J60_SendByte(op | (address & ADDR_MASK));

    // Write the data
    ENC28J60_SendByte(data);

    // Disable the chip (CS high)
    disableChip;
}

void enc28j60PowerDown() {
    // Disable packet reception
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_RXEN);

    // Wait for any ongoing receive operation to complete
    while (enc28j60Read(ESTAT) & ESTAT_RXBUSY);

    // Wait for any ongoing transmission to complete
    while (enc28j60Read(ECON1) & ECON1_TXRTS);

    // Enter power save mode
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PWRSV);
}

void enc28j60PowerUp() {
    // Exit power save mode
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON2, ECON2_PWRSV);

    // Wait for the clock to become ready
    while (!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
}

void enc28j60ReadBuffer(uint16_t len, uint8_t* data) {
    // Enable the chip (CS low)
    enableChip;

    // Issue read buffer memory command
    ENC28J60_SendByte(ENC28J60_READ_BUF_MEM);

    // Read data from buffer
    while (len--) {
        *data++ = ENC28J60_SendByte(0x00);
    }

    // Disable the chip (CS high)
    disableChip;
}

static uint16_t enc28j60ReadBufferWord() {
    uint16_t result;
    // Read 2 bytes from the buffer and store them in result
    enc28j60ReadBuffer(2, (uint8_t*)&result);
    return result;
}

void enc28j60WriteBuffer(uint16_t len, uint8_t* data) {
    // Enable the chip (CS low)
    enableChip;

    // Issue write buffer memory command
    ENC28J60_SendByte(ENC28J60_WRITE_BUF_MEM);

    // Write data to buffer
    while (len--) {
        ENC28J60_SendByte(*data++);
    }

    // Disable the chip (CS high)
    disableChip;
}

void enc28j60SetBank(uint8_t address) {
    uint8_t bank = address & BANK_MASK;

    if (bank != Enc28j60Bank) {
        // Clear the bank select bits in ECON1
        enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_BSEL1 | ECON1_BSEL0);

        // Set the new bank
        Enc28j60Bank = bank;

        // Set the bank select bits in ECON1
        enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, bank >> 5);
    }
}

uint8_t enc28j60Read(uint8_t address) {
    // Set the bank for the given address
    enc28j60SetBank(address);

    // Read the value from the specified register
    return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}


void enc28j60WriteWord(uint8_t address, uint16_t data) {
    // Write the low byte to the specified address
    enc28j60Write(address, data & 0xFF);

    // Write the high byte to the next address
    enc28j60Write(address + 1, data >> 8);
}

// Read upper 8 bits from PHY register
uint16_t enc28j60PhyReadH(uint8_t address) {
    // Set the address and start the register read operation
    enc28j60Write(MIREGADR, address);
    enc28j60Write(MICMD, MICMD_MIIRD);

    // Wait for the read operation to complete
    HAL_Delay(15);

    // Wait until the PHY read completes
    while (enc28j60Read(MISTAT) & MISTAT_BUSY);

    // Reset the reading bit
    enc28j60Write(MICMD, 0x00);

    // Return the read value
    return enc28j60Read(MIRDH);
}

void enc28j60Write(uint8_t address, uint8_t data) {
    // Set the bank for the given address
    enc28j60SetBank(address);

    // Write the data to the specified address
    enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc28j60PhyWrite(uint8_t address, uint16_t data) {
    // Set the PHY register address
    enc28j60Write(MIREGADR, address);

    // Write the low byte of PHY data
    enc28j60Write(MIWRL, data & 0xFF);

    // Write the high byte of PHY data
    enc28j60Write(MIWRH, data >> 8);

    // Wait until the PHY write completes
    while (enc28j60Read(MISTAT) & MISTAT_BUSY) {
        HAL_Delay(15);
    }
}

static void enc28j60PhyWriteWord(uint8_t address, uint16_t data) {
    // Set the PHY register address
    enc28j60Write(MIREGADR, address);

    // Write the PHY data
    enc28j60WriteWord(MIWRL, data);

    // Wait until the PHY write completes, with a timeout
    uint32_t startTick = HAL_GetTick();
    while (enc28j60Read(MISTAT) & MISTAT_BUSY) {
        if ((HAL_GetTick() - startTick) > 100) { // 100ms timeout
            // Handle timeout (e.g., log an error, reset the PHY, etc.)
            return;
        }
        HAL_Delay(1);
    }
}

void enc28j60clkout(uint8_t clk) {
    // Set the ECOCON register with the lower 3 bits of clk
    // 0: Output clock disabled
    // 1: 6.25 MHz
    // 2: 12.5 MHz
    // 3: 25 MHz
    // 4-7: Reserved
    enc28j60Write(ECOCON, clk & 0x07);
}

void enc28j60Init(uint8_t* macaddr) {
    // Enable the chip (CS low)
    enableChip;

    // Perform system reset
    enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
    HAL_Delay(50);

    // Initialize receive buffer
    gNextPacketPtr = RXSTART_INIT;
    enc28j60WriteWord(ERXSTL, RXSTART_INIT);     // Rx start
    enc28j60WriteWord(ERXRDPTL, RXSTART_INIT);   // Set receive pointer address
    enc28j60WriteWord(ERXNDL, RXSTOP_INIT);      // Rx end
    enc28j60WriteWord(ETXSTL, TXSTART_INIT);     // Tx start
    enc28j60WriteWord(ETXNDL, TXSTOP_INIT);      // Tx end

    // Packet filter: Allow only ARP packets for broadcast, and unicast for our MAC
    erxfcon = ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_PMEN | ERXFCON_BCEN;
    enc28j60Write(ERXFCON, erxfcon);
    enc28j60WriteWord(EPMM0, 0x303f);
    enc28j60WriteWord(EPMCSL, 0xf7f9);

    // Enable MAC receive
    enc28j60Write(MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);
    // Bring MAC out of reset
    enc28j60Write(MACON2, 0x00);
    // Enable automatic padding to 60 bytes and CRC operations
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);

    // Set inter-frame gap (non-back-to-back)
    enc28j60WriteWord(MAIPGL, 0x0C12);
    // Set inter-frame gap (back-to-back)
    enc28j60Write(MABBIPG, 0x12);

    // Set the maximum packet size which the controller will accept
    enc28j60WriteWord(MAMXFLL, MAX_FRAMELEN);

    // Write MAC address (note: MAC address in ENC28J60 is byte-backward)
    enc28j60Write(MAADR5, macaddr[0]);
    enc28j60Write(MAADR4, macaddr[1]);
    enc28j60Write(MAADR3, macaddr[2]);
    enc28j60Write(MAADR2, macaddr[3]);
    enc28j60Write(MAADR1, macaddr[4]);
    enc28j60Write(MAADR0, macaddr[5]);

    // No loopback of transmitted frames
    enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);

    // Switch to bank 0
    enc28j60SetBank(ECON1);

    // Enable interrupts
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE | EIE_PKTIE);

    // Enable packet reception
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}

uint8_t enc28j60getrev(void) {
    uint8_t rev = enc28j60Read(EREVID);
    // Microchip forgot to step the number on the silicon when they released revision B7.
    // 6 is now rev B7. If rev is greater than 5, increment it.
    if (rev > 5) rev++;
    return rev;
}

void enc28j60EnableBroadcast(void) {
    erxfcon |= ERXFCON_BCEN;
    enc28j60Write(ERXFCON, erxfcon);
}

void enc28j60DisableBroadcast(void) {
    erxfcon &= ~ERXFCON_BCEN;
    enc28j60Write(ERXFCON, erxfcon);
}

void enc28j60EnableMulticast(void) {
    erxfcon |= ERXFCON_MCEN;
    enc28j60Write(ERXFCON, erxfcon);
}

void enc28j60DisableMulticast(void) {
    erxfcon &= ~ERXFCON_MCEN;
    enc28j60Write(ERXFCON, erxfcon);
}

uint8_t enc28j60linkup(void) {
    // Check bit 3 in the upper PHSTAT2 register (bit 10 overall)
    return (enc28j60PhyReadH(PHSTAT2) & 0x04) ? 1 : 0;
}

void enc28j60PacketSend(uint16_t len, uint8_t* packet) {
    // Check no transmit in progress
    while (enc28j60ReadOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_TXRTS) {
        // Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
        if (enc28j60Read(EIR) & EIR_TXERIF) {
            enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
            enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
        }
    }

    // Set the write pointer to start of the transmit buffer area
    enc28j60WriteWord(EWRPTL, TXSTART_INIT);

    // Set the TXND pointer to correspond to the packet size given
    enc28j60WriteWord(ETXNDL, TXSTART_INIT + len);

    // Write per-packet control byte (0x00 means use macon3 settings)
    enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

    // Copy the packet into the transmit buffer
    enc28j60WriteBuffer(len, packet);

    // Send the contents of the transmit buffer onto the network
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

uint8_t enc28j60hasRxPkt(void) {
    return enc28j60Read(EPKTCNT) > 0;
}

uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t* packet) {
    uint16_t rxstat;
    uint16_t len;

    // Check if a packet has been received and buffered
    if (enc28j60Read(EPKTCNT) == 0) {
        return 0;
    }

    // Set the read pointer to the start of the received packet
    enc28j60WriteWord(ERDPTL, gNextPacketPtr);

    // Read the next packet pointer
    gNextPacketPtr = enc28j60ReadBufferWord();

    // Read the packet length (excluding the CRC length)
    len = enc28j60ReadBufferWord() - 4;

    // Read the receive status
    rxstat = enc28j60ReadBufferWord();

    // Limit retrieve length to maxlen - 1
    if (len > maxlen - 1) {
        len = maxlen - 1;
    }

    // Check CRC and symbol errors
    if ((rxstat & 0x80) == 0) {
        len = 0;
    } else {
        // Copy the packet from the receive buffer
        enc28j60ReadBuffer(len, packet);
    }

    // Move the RX read pointer to the start of the next received packet
    if ((gNextPacketPtr - 1 < RXSTART_INIT) || (gNextPacketPtr - 1 > RXSTOP_INIT)) {
        enc28j60WriteWord(ERXRDPTL, RXSTOP_INIT);
    } else {
        enc28j60WriteWord(ERXRDPTL, gNextPacketPtr - 1);
    }

    // Decrement the packet counter to indicate we are done with this packet
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);

    return len;
}
/* end of enc28j60.c */
