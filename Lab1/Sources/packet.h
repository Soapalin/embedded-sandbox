/*! @file
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *  @author PMcL
 *  @date 2015-07-23
 */

#ifndef PACKET_H
#define PACKET_H

// New types
#include "types.h"

// Packet structure
extern uint8_t 	Packet_Command,		/*!< The packet's command */
		Packet_Parameter1, 	/*!< The packet's 1st parameter */
		Packet_Parameter2, 	/*!< The packet's 2nd parameter */
		Packet_Parameter3,	/*!< The packet's 3rd parameter */
		Packet_Checksum;	/*!< The packet's checksum */

// Acknowledgment bit mask
extern const uint8_t PACKET_ACK_MASK;

extern uint8_t packetComplete;

//All macro for packets and commands are found in Tower Serial Communication Protocol.pdf

/***********************************************************************************************************
 ************************************** TOWER TO PC COMMANDS ***********************************************
 **********************************************************************************************************/

// What the tower should send to the PC upon startup
#define TOWER_STARTUP_COMMAND 0x04
#define TOWER_STARTUP_PARAMETER1 0
#define TOWER_STARTUP_PARAMETER2 0
#define TOWER_STARTUP_PARAMETER3 0

// What the tower should send to the PC when fetching the tower version
#define TOWER_VERSION_COMMAND 0x09
#define TOWER_VERSION_PARAMETER1 'v' // v for version
#define TOWER_VERSION_PARAMETER2 1 // major (version)
#define TOWER_VERSION_PARAMETER3 0


//The first two bytes that the tower should send to the PC upon startup
#define TOWER_NUMBER_COMMAND 0x0b
#define TOWER_NUMBER_PARAMETER1 1

//Command with acknowledgement bit 7
#define TOWER_STARTUP_ACK 0x84
#define TOWER_VERSION_ACK 0x89
#define TOWER_NUMBER_ACK 0x8b



/************************************************************************************************************
 * ************************************** PC TO TOWER COMMANDS **********************************************
 ***********************************************************************************************************/
// Special - Get the startup value from tower upon startup
#define GET_TOWER_STARTUP 0x04

// Special - Get the tower version
#define GET_TOWER_VERSION 0x09

// Get or set the tower number from the PC
#define TOWER_NUMBER_COMMAND 0x0b
#define TOWER_NUMBER_SET 2 // PARAMETER1 to be set to 2 to set the tower number
#define TOWER_NUMBER_GET 1 // PARAMETER1 has to be set to 1 to get the tower number

/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz.
 *  @return bool - TRUE if the packet module was successfully initialized.
 */
bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk);

/*! @brief Attempts to get a packet from the received data.
 *
 *  @return bool - TRUE if a valid packet was received.
 */
bool Packet_Get(void);

/*! @brief Builds a packet and places it in the transmit FIFO buffer.
 *
 *  @return bool - TRUE if a valid packet was sent.
 */
bool Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3);

#endif
