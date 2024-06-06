/*
 * UDPCommandHandler.h
 *
 * Created on: Jun 4, 2024
 * Author: dtneo
 *
 * This header file declares the necessary structures and functions for handling
 * UDP commands received by the STM32 microcontroller. It includes function
 * prototypes for various home automation tasks such as sending the STM32 state,
 * sending the RTC date and time, turning on a light, activating a relay, and
 * sending the temperature.
 */

#ifndef INC_UDPCOMMANDHANDLER_H_
#define INC_UDPCOMMANDHANDLER_H_

#include "defines.h"

// Define a function pointer type for command functions
typedef void (*CommandFunction)();

// Structure to map commands to their corresponding functions
typedef struct {
    const char *command;    // Command string
    CommandFunction function; // Function to execute for the command
} CommandMapping;

// Declare the command table as an external variable
extern CommandMapping commandTable[];

// Declare command functions here
void sendSTM32State();       // Function to send the STM32 state
void sendRTCDateTime();      // Function to send the RTC date and time
void turnOnLight();          // Function to turn on a light
void activateRelay();        // Function to activate a relay
void sendTemperature();      // Function to send the temperature

#endif /* INC_UDPCOMMANDHANDLER_H_ */
