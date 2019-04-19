/* ###################################################################
**     Filename    : packet.c
**     Project     : Lab2
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-04-16, 10:23, # CodeGen: 0
**     Abstract    :
**         packet module
**     Settings    :
**     Contents    :
**         public methods
**
** ###################################################################*/
/*!
** @file packet.c
** @brief
**         packet module.
**         This module contains code for packet control.
*/
/*!
**  @addtogroup packet_module packet module documentation
**   @{@}
*/
/*! @file
 *
 *  @brief Routines to access the packet on the TWR-K70F120M.
 *
 *  This contains the functions for operating the packet.
 *
 *  @author Lucien Tran & Angus Ryan
 *  @date 2019-04-16
 */

#include "UART.h"
#include "packet.h"


TPacket Packet;
static uint8_t packetComplete = 0;

bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  return UART_Init(baudRate, moduleClk);
}

bool Packet_Get(void)
{
  uint8_t byteSum; /*!< To check if the checksum is correct */
  /*!< using switch per Packet videos in Lab Videos by Peter McLean */
  switch(packetComplete)
  {
    case 0:
    {
      /*!< If UART_InChar returns true, increment packetCondition */
      if(UART_InChar(&Packet_Command))
      {
	packetComplete++;
	return false;
      }
    }
    case 1:
    {
      /*!< If UART_InChar returns true, increment packetCondition */
      if(UART_InChar(&Packet_Parameter1)) {
	packetComplete++;
	return false;
      }
    }
    case 2:
    {
      /*!< If UART_InChar returns true, increment packetCondition */
      if(UART_InChar(&Packet_Parameter2))
      {
	packetComplete++;
	return false;
      }
    }
    case 3:
    {
      /*!< If UART_InChar returns true, increment packetCondition */
      if(UART_InChar(&Packet_Parameter3))
      {
	packetComplete++;
	return false;
      }
    }
    case 4:
    {
      if(UART_InChar(&Packet_Checksum))
      {
	/*!< If UART_InChar returns true, calculate the byteSum with the previous InChar found */
	byteSum = Checksum_Calculation(Packet_Command, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
	/*!< Check if byteSum is equal to the checksum byte we just fetched */
	if(Packet_Checksum == byteSum)
	{
	  /*!< if the checksum and byteSum are equal, break from the switch statement */
	  packetComplete = 0;
	  return true;
	}
	else
	{
	  /*!< Shift the bytes by one to not lose all the data and only getting read of the first byte of invalid data */
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

bool Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
  uint8_t byteSum;

  if(!UART_OutChar(command))
  {
    return false;
  }
  if(!UART_OutChar(parameter1))
  {
    return false;
  }
  if(!UART_OutChar(parameter2))
  {
    return false;
  }
  if(!UART_OutChar(parameter3))
  {
    return false;
  }
  byteSum = Checksum_Calculation(command, parameter1, parameter2, parameter3);
  if(!UART_OutChar(byteSum))
  {
    return false;
  }
  return true;
}

uint8_t Checksum_Calculation(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
  uint8_t checksum = command^parameter1^parameter2^parameter3;
  return checksum;
}
