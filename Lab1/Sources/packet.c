/* ###################################################################
**     Filename    : packet.c
**     Project     : Lab1
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-03-27, 16:27, # CodeGen: 0
**     Abstract    :
**         Packet module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file packet.c
** @version 1.0
** @brief
**         Main module.
**         This module contains user's application code.
*/
/*!
**  @addtogroup main_module main module documentation
**  @{
*/
/* MODULE */
#include "UART.h"
#include "packet.h"

uint8_t 	Packet_Command,		/*!< The packet's command */
		Packet_Parameter1, 	/*!< The packet's 1st parameter */
		Packet_Parameter2, 	/*!< The packet's 2nd parameter */
		Packet_Parameter3,	/*!< The packet's 3rd parameter */
		Packet_Checksum;	/*!< The packet's checksum */
/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz.
 *  @return bool - TRUE if the packet module was successfully initialized.
 */
bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk) {
  return UART_Init(baudRate, moduleClk);
}

/*! @brief Attempts to get a packet from the received data.
 *
 *  @return bool - TRUE if a valid packet was received.
 */
bool Packet_Get(void) {
  static uint8_t packetComplete;
  uint8_t byteSum; // To check if the checksum is correct
  // using switch per Packet videos in Lab Videos by Peter McLean
  switch(packetComplete) {
    case 0: {
      //If UART_InChar returns true, increment packetCondition
      if(UART_InChar(&Packet_Command)) {
	packetComplete++;
	return false;
      }
    }
    case 1: {
      //If UART_InChar returns true, increment packetCondition
      if(UART_InChar(&Packet_Parameter1)) {
	packetComplete++;
	return false;
      }
    }
    case 2: {
      //If UART_InChar returns true, increment packetCondition
      if(UART_InChar(&Packet_Parameter2)) {
	packetComplete++;
	return false;
      }
    }
    case 3: {
      //If UART_InChar returns true, increment packetCondition
      if(UART_InChar(&Packet_Parameter3)) {
	packetComplete++;
	return false;

      }
    }
    case 4: {
      if(UART_InChar(&Packet_Checksum)) {
	//If UART_InChar returns true, calculate the byteSum with the previous InChar found
	byteSum = Packet_Command^Packet_Parameter1^Packet_Parameter2^Packet_Parameter3;
	//Check if byteSum is equal to the checksum byte we just fetched
	if(Packet_Checksum == byteSum) {
	    //if the checksum and byteSum are equal, break from the switch statement
	    packetComplete = 0;
	    return true;
	}
	else {
	    // Shift the bytes by one to not lose all the data and only getting read of the first byte of invalid data
	    Packet_Command = Packet_Parameter1;
	    Packet_Parameter1 = Packet_Parameter2;
	    Packet_Parameter2 = Packet_Parameter3;
	    packetComplete--;
	    return false;
	}
      }
    }
  }

}


/*! @brief Builds a packet and places it in the transmit FIFO buffer.
 *
 *  @return bool - TRUE if a valid packet was sent.
 */
bool Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3) {
  uint8_t byteSum;

  if(!UART_OutChar(command)) {
      return false;
  }
  if(!UART_OutChar(parameter1)) {
      return false;
  }
  if(!UART_OutChar(parameter2)) {
      return false;
  }
  if(!UART_OutChar(parameter3)) {
      return false;
  }
  byteSum = command^parameter1^parameter2^parameter3;
  if(!UART_OutChar(byteSum)) {
      return false;
  }
  else {
      return true;
  }
}
