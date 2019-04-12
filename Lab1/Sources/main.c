/* ###################################################################
**     Filename    : main.c
**     Project     : Lab1
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 2.0
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


// CPU module - contains low level hardware initialization routines
#include "Cpu.h"

//Header Files
#include "packet.h"
#include "UART.h"

#define BAUDRATE 38400 //Baud Rate specified in project
#define MODULECLK CPU_BUS_CLK_HZ //Clock Speed referenced from Cpu.H

uint8_t Lsb = 0x6D; //least significant byte of student number 7533 1D6D equates to 7533.
uint8_t Msb = 0x1D; //most significant byte of student number 7533

const uint8_t PACKET_ACK_MASK; //Packet Acknowledgment mask, referring to bit 7 of the Packet

//Function Prototypes
bool PacketHandler(void);
void StartupPackets(void);

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/
  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  bool successful = Packet_Init(BAUDRATE, MODULECLK); //initiate packet (further initiates UART) - verify successful.
  StartupPackets(); //calling Function to deal with tower startup packets.

  for(;;)
  {
    if (successful)
    {
      if (Packet_Get())
      { // When a complete packet is finally formed, handle the packet accordingly
        PacketHandler();
      }
      UART_Poll();  //Call the UART Polling function (refer to UART.c to see functionality)
    }
  }

  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/


/*! @brief Send the packets needed on startup
 *
 *
 *  @param void
 *  @return void
 *  @note Assumes that Packet_Init was called
 */
void StartupPackets(void) {
  Packet_Put(TOWER_STARTUP_COMMAND, TOWER_STARTUP_PARAMETER1, TOWER_STARTUP_PARAMETER2, TOWER_STARTUP_PARAMETER3);
  Packet_Put(TOWER_VERSION_COMMAND, TOWER_VERSION_PARAMETER1, TOWER_VERSION_PARAMETER2, TOWER_VERSION_PARAMETER3);
  Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_PARAMETER1, Lsb, Msb);
}

/*! @brief Process the packet that has been received
 *
 *
 *  @param void
 *  @return bool - TRUE if the packet has been handled properly.
 *  @note Assumes that Packet_Init and Packet_Get was called
 */
bool PacketHandler(void) { // Packet Handler used after Packet Get
  bool acknowledged= false;  // Acknowledge is false as long as the package isn't acknowledge or if it's not required
  switch(Packet_Command) {
    // All macro are defined in packet.h
    case GET_TOWER_STARTUP: { // when command = 0x04 and is looking for startup values
      // Sending the three packets to PC - startup value, tower version and tower number
      Packet_Put(TOWER_STARTUP_COMMAND, TOWER_STARTUP_PARAMETER1, TOWER_STARTUP_PARAMETER2, TOWER_STARTUP_PARAMETER3);
      Packet_Put(TOWER_VERSION_COMMAND, TOWER_VERSION_PARAMETER1, TOWER_VERSION_PARAMETER2, TOWER_VERSION_PARAMETER3);
      Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_PARAMETER1, Lsb, Msb);
      break;
    }
    case GET_TOWER_VERSION: { // when command = 0x09 and is looking for the tower version
      // send the tower version to PC with Packet_Put - V1.0
      Packet_Put(TOWER_VERSION_COMMAND,TOWER_VERSION_PARAMETER1,TOWER_VERSION_PARAMETER2, TOWER_VERSION_PARAMETER3);
      break;

    }
    case TOWER_NUMBER_COMMAND: { // when command = 0x0B and is looking for the tower number
      if(Packet_Parameter1 == (uint8_t) 1) {
	  // if Parameter1 = 1 - get the tower number and send it to PC
	  Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_GET, Lsb, Msb);
      }
      else if(Packet_Parameter1 == (uint8_t) 2) {
	  Lsb = Packet_Parameter2; // assign the new parameter 2 - least significant byte for tower number
	  Msb = Packet_Parameter3; // assign the new parameter 3 - most significant byte for tower number
	  // If Parameter1 = 2 - set the tower number and send it to PC
	  Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_SET, Lsb, Msb);
      }
      break;
    }
    //Packet Acknowledgment
    case TOWER_STARTUP_ACK: { // when command = 0x84 - get the startup value with ACK bit 7
      if(Packet_Put(TOWER_STARTUP_COMMAND, TOWER_STARTUP_PARAMETER1, TOWER_STARTUP_PARAMETER2, TOWER_STARTUP_PARAMETER3)){
	  if(Packet_Put(TOWER_VERSION_COMMAND, TOWER_VERSION_PARAMETER1, TOWER_VERSION_PARAMETER2, TOWER_VERSION_PARAMETER3)) {
	      if(Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_PARAMETER1, Lsb, Msb)) {
		  //After sending startup packets, send the acknowledgement packet if successful with ACK bit 7
		  Packet_Put(TOWER_STARTUP_ACK, TOWER_STARTUP_PARAMETER1, TOWER_STARTUP_PARAMETER2, TOWER_STARTUP_PARAMETER3);
		  acknowledged = true;
	      }
	  }
      }
      break;
    }
    case TOWER_VERSION_ACK: // Get the version of the tower with ACK required
      {
      if(Packet_Put(TOWER_VERSION_COMMAND,TOWER_VERSION_PARAMETER1,TOWER_VERSION_PARAMETER2, TOWER_VERSION_PARAMETER3))
      {
	  // After sending the version of the tower successfully, send the packet back with ACK bit to show success
	  Packet_Put(TOWER_VERSION_ACK,TOWER_VERSION_PARAMETER1,TOWER_VERSION_PARAMETER2, TOWER_VERSION_PARAMETER3);
	  acknowledged = true;
      }
      break;
    }
    case TOWER_NUMBER_ACK: // get the tower number with ACK required
      {
      if(Packet_Parameter1 == (uint8_t) 1) { // If the command is GET tower number
	  if(Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_GET, Lsb, Msb)) {
	      // Fetch the tower number and send it to PC. After success, send ACK packet
	      Packet_Put(TOWER_NUMBER_ACK, TOWER_NUMBER_GET, Lsb, Msb);
	      acknowledged = true;
	  }
      }
      else if(Packet_Parameter1 == (uint8_t) 2) { // If the command is SET tower number
	  //Set the tower number with the new parameters
	  Lsb = Packet_Parameter2;
	  Msb = Packet_Parameter3;
	  if(Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_SET, Lsb, Msb)) {
	      // after sending the new tower number, send the ACK packet
	      Packet_Put(TOWER_NUMBER_ACK, TOWER_NUMBER_SET, Lsb, Msb);
	      acknowledged = true;
	  }
      }
      break;
    }
  }

  if(acknowledged == false)
  { //If the command couldn't be carried out, send the command with bit 7 cleared
    switch(Packet_Command)
    {
      case TOWER_VERSION_ACK:
      { // GET tower version packet command could not be carried out, so send the packet with the ACK bit 7 cleared (0)
	Packet_Put(TOWER_VERSION_COMMAND,TOWER_VERSION_PARAMETER1,TOWER_VERSION_PARAMETER2, TOWER_VERSION_PARAMETER3);
	break;
      }
      case TOWER_NUMBER_ACK:
      {
	// GET tower number packet command could not be carried out, so send the packet with the ACK bit 7 cleared (0)
	if(Packet_Parameter1 == (uint8_t) 1)
	{
	  Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_GET, Lsb, Msb);
	}
	// SET tower number packet command could not be carried out, so send the packet with the ACK bit 7 cleared (0)
        else if(Packet_Parameter1 == (uint8_t) 2)
        {
	  Lsb = Packet_Parameter2;
	  Msb = Packet_Parameter3;
          Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_SET, Lsb, Msb);
	}
        break;
      }
      case TOWER_STARTUP_ACK:
      { // GET tower startup packet command could not be carried out, so send the packet with the ACK bit 7 cleared (0)
	Packet_Put(TOWER_STARTUP_COMMAND, TOWER_STARTUP_PARAMETER1, TOWER_STARTUP_PARAMETER2, TOWER_STARTUP_PARAMETER3);
        break;
      }
    }
    return true;
  }
  else
  {
    return true;
  }
}

/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
