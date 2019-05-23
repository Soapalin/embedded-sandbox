/* ###################################################################
**     Filename    : main.c
**     Project     : Lab3
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-04-16, 10:23, # CodeGen: 0
**     Abstract    :
**         main module
**     Settings    :
**     Contents    :
**         public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 4.0
** @brief
**         main module.
**         This module is the main.
*/
/*!
**  @addtogroup main_module main module documentation
**  @{
*/
/*! @file
 *
 *  @brief Main on the TWR-K70F120M.
 *
 *  Is the Main Module.
 *
 *  @author Lucien Tran & Angus Ryan
 *  @date 2019-04-16
 */

/*!< CPU module - contains low level hardware initialization routines */
#include "Cpu.h"
#include "Events.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"

/*!< Header Files */
#include "types.h"
#include "Flash.h"
#include "packet.h"
#include "UART.h"
#include "LEDs.h"
#include "RTC.h"
#include "PIT.h"
#include "FTM.h"
#include "accel.h"



/****************************************************************************************************************
 * Private function declaration
 ***************************************************************************************************************/
bool PacketHandler(void);
bool StartupPackets(void);
bool VersionPackets(void);
bool TowerNumberPackets(void);
bool TowerModePackets(void);
bool ProgramBytePackets(void);
bool ReadBytePackets(void);
bool TowerInit(void);
bool ClockInit(void);
void RTCCallback(void);
bool TowerTimePackets(void);
void PITCallback(void);
void FTM0Callback(void *argument);
bool ProtocolPackets(void);
void AccelDataCallback(void* arg);
void AccelReadCallback(void* arg);


/****************************************************************************************************************
 * Global variables and macro definitions
 ***************************************************************************************************************/
const uint32_t BAUDRATE = 115200; /*!< Baud Rate specified in project */
const uint32_t MODULECLK = CPU_BUS_CLK_HZ; /*!< Clock Speed referenced from Cpu.H */
const uint16_t STUDENT_ID = 0x1D6D; /*!< Student Number: 7533 */
const uint32_t PIT_Period = 641025640; /*!< 1/1056Hz = 641025640 ns*/

const uint8_t PACKET_ACK_MASK; /*!< Packet Acknowledgment mask, referring to bit 7 of the Packet */
static volatile uint16union_t *TowerNumber; /*!< declaring static TowerNumber Pointer */
static volatile uint16union_t *TowerMode; /*!< declaring static TowerMode Pointer */

TFTMChannel FTMPacket =
{
  0, /*!< Channel being used */
  CPU_MCGFF_CLK_HZ_CONFIG_0, /*!< delay count: fixed frequency clock, mentioned in Timing and Generation Docs */
  TIMER_FUNCTION_OUTPUT_COMPARE, /*!< Brief specific: we are using OutputCompare*/
  TIMER_OUTPUT_LOW, /*!< Choose one functionality of output compare: low */
  FTM0Callback, /*!< Setting User Callback Function */
  (void*) 0, /*!< User callback arguments being passed */
};




/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
   bool successful = false;/*!</local variable to confirm that all initialisation has been successful. */
  __DI(); /*!< disable interrupts (referenced in Interrupts Notes as good practice before enabling (5.11)) */
  PE_low_level_init();
  if (TowerInit()) /*!< run Tower Initialisation. */
  {
    StartupPackets(); /*!< Sends Packets from Device to PC on Startup. */
    successful = true;
    FTM_Set(&FTMPacket); /*!< configure FTM0 functionality, passing in the declared struct address containing values at top of file */
    LEDs_On(LED_ORANGE); /*!<  turn on LEDs once Packet, UART and Tower Initiated. */
  }
  __EI(); /*!< enabling interrupts (referenced in Lab 3 Hints (L3.4) ) */
  for(;;)
  {
    if (successful)
    {
      if (Packet_Get())
      {
        FTM_StartTimer(&FTMPacket); /*!< Start timer, calling interrupt User function (FTM0Callback) once completed.  */
 	LEDs_On(LED_BLUE);
 	PacketHandler(); /*!<  When a complete packet is finally formed, handle the packet accordingly */
      }
//      UART_Poll(); /*!< Call the UART Polling function (refer to UART.c to see functionality) */
    }

  }
  /*** End of Processor Expert internal initialization.                    ***/
  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/




/*! @brief saves in Flash the TowerNumber and the TowerMode
 *
 *
 *  @return bool - TRUE if packet has been sent successfully
 *  @note Assumes that Packet_Init was called
 */
bool TowerInit(void)
{

  TAccelSetup accelSetup =
  {
    MODULECLK,
    (void* )&AccelDataCallback,
    NULL,
    (void* )&AccelReadCallback,
    NULL
  };
  Flash_Init();
  /*!<  Allocate var for both Tower Number and Mode, if succcessful, FlashWrite16 them with the right values */
  bool towerModeInit = Flash_AllocateVar( (volatile void **) &TowerMode, sizeof(*TowerMode));
  bool towerNumberInit = Flash_AllocateVar((volatile void **) &TowerNumber, sizeof(*TowerNumber));
  if (towerModeInit && towerNumberInit && LEDs_Init())
  {
    if (TowerMode->l == 0xffff) /* when unprogrammed, value = 0xffff, announces in hint*/
    {
      Flash_Write16((volatile uint16_t *) TowerMode, 0x1); /*!< Parsing through the function: typecast volatile uint16_t pointer from uint16union_t pointer, and default towerMode = 1 */
    }
    if (TowerNumber->l == 0xffff) /* when unprogrammed, value = 0xffff, announces in hint*/
    {
      Flash_Write16((volatile uint16_t *) TowerNumber, STUDENT_ID); /*Like above, but with towerNumber set to our student ID = 7533*/
    }
    if (RTC_Init((void*) RTCCallback , NULL) && PIT_Init(MODULECLK, (void*) &PITCallback, NULL) && (Packet_Init(BAUDRATE, MODULECLK)) && FTM_Init()) { /*!< Initiate all required modules */
      Accel_Init(&accelSetup);
      return true; /* !< If successful initiation then return true */
    }
  }
  return false;
}

/*! @brief Send the packets needed on startup

 *  @return bool - TRUE if packet has been sent successfully
 *  @note Assumes that Packet_Init was called
 */
bool StartupPackets(void)
{
  if (Packet_Put(TOWER_STARTUP_COMMAND, TOWER_STARTUP_PARAMETER1, TOWER_STARTUP_PARAMETER2, TOWER_STARTUP_PARAMETER3))
  {
    if (Packet_Put(TOWER_VERSION_COMMAND, TOWER_VERSION_PARAMETER1, TOWER_VERSION_PARAMETER2, TOWER_VERSION_PARAMETER3))
    {
      if (Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_PARAMETER1, TowerNumber->s.Lo, TowerNumber->s.Hi))
      {
        if (Packet_Put(TOWER_MODE_COMMAND,TOWER_MODE_GET, TowerMode->s.Lo, TowerMode->s.Hi))
        {
          if (CurrentMode == ACCEL_POLL)
          {
            return Packet_Put(PROTOCOL_COMMAND, PROTOCOL_PARAMETER1, PROTOCOL_ASYNC, PROTOCOL_PARAMETER3);
          }
          else if (CurrentMode == ACCEL_INT)
          {
            return Packet_Put(PROTOCOL_COMMAND, PROTOCOL_PARAMETER1, PROTOCOL_SYNC, PROTOCOL_PARAMETER3);
          }
        }
      }
    }
  }
}

/*! @brief Send the version packet to the PC
 *
 *  @return bool - TRUE if packet has been sent successfully
 *  @note Assumes that Packet_Init was called
 */
bool VersionPackets(void)
{
  return Packet_Put(TOWER_VERSION_COMMAND,TOWER_VERSION_PARAMETER1,TOWER_VERSION_PARAMETER2, TOWER_VERSION_PARAMETER3);
}

/*! @brief Send the tower number packet to the PC
 *
 *  @return bool - TRUE if packet has been sent successfully
 *  @note Assumes that Packet_Init was called
 */
bool TowerNumberPackets(void)
{
  if (Packet_Parameter1 == (uint8_t) 1)
  {
    // if Parameter1 = 1 - get the tower number and send it to PC
    return Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_GET, TowerNumber->s.Lo, TowerNumber->s.Hi);
  }
  else if (Packet_Parameter1 == (uint8_t) 2) // if Parameter1 =2 - write new TowerNumber to Flash and send it to interface
  {
    uint16union_t newTowerNumber; /*! < create a union variable to combine the two Parameters*/
    newTowerNumber.s.Lo = Packet_Parameter2;
    newTowerNumber.s.Hi = Packet_Parameter3;
    Flash_Write16((volatile uint16_t *) TowerNumber, newTowerNumber.l);
    return Packet_Put(TOWER_NUMBER_COMMAND, TOWER_NUMBER_SET, TowerNumber->s.Lo, TowerNumber->s.Hi);
  }
}

/*! @brief Send the tower mode packet to the PC
 *
 *  @return bool - TRUE if packet has been sent successfully
 *  @note Assumes that Packet_Init was called
 */
bool TowerModePackets(void)
{
  if (Packet_Parameter1 == 1) // if paramater1 = 1 - get the towermode and send it to PC
  {
    return Packet_Put(TOWER_MODE_COMMAND,TOWER_MODE_GET, TowerMode->s.Lo, TowerMode->s.Hi);
  }
  else if (Packet_Parameter1 == 2) // if parameter1 = 2 - set the towermode, write to Flash and send it back to PC
  {
    uint16union_t newTowerMode; /* !< Create a union variable to combine parameter2 and 3*/
    newTowerMode.s.Lo = Packet_Parameter2;
    newTowerMode.s.Hi = Packet_Parameter3;
    Flash_Write16((volatile uint16_t *) TowerMode, newTowerMode.l);
    return Packet_Put(TOWER_MODE_COMMAND,TOWER_MODE_SET, TowerMode->s.Lo, TowerMode->s.Hi);
  }
  return false;
}

/*! @brief Handles the packet to program bytes in FLASH
 *
 *  @return bool - TRUE if packet has been sent and handled successfully
 *  @note Assumes that Packet_Init was called
 */
bool ProgramBytePackets(void)
{
  if (Packet_Parameter1 == 8)
  {
    return Flash_Erase(); /*! < if Parameter1  = 8 - erase the whole sector */
  }
  else if (Packet_Parameter1 > 8)
  {
    return false; //data sent is obsolete
  }
  else /*!< if offset (Parameter1) is between 0 and 7 inclusive, check the offset */
  {
    volatile uint8_t *address = (uint8_t *)(FLASH_DATA_START + Packet_Parameter1);
    return Flash_Write8(address, Packet_Parameter3); //Write in the Flash
  }
  return false;
}

/*! @brief Handles the packet to read bytes from FLASH
 *
 *  @return bool - TRUE if packet has been sent and handled successfully
 *  @note Assumes that Packet_Init was called
 */
bool ReadBytePackets(void)
{
  uint8_t readByte = _FB(FLASH_DATA_START + Packet_Parameter1); /* !< fetching the Byte at offset Parameter1 and send it to PCc*/
  return Packet_Put(FLASH_READ_COMMAND, Packet_Parameter1, 0x0, readByte);
}

/*! @brief Handles the packet RTC time - sends back ther packet to PC if setting time is successful
 *
 *  @return bool - TRUE if packet has been sent and handled successfully
 *  @note Assumes that Packet_Init and RTC_Init was called
 */
bool TowerTimePackets(void)
{
  /*!< Checking if input is valid, if not, return false */
  if (Packet_Parameter1 <= 23)
  {
    if (Packet_Parameter2 <= 59)
    {
      if (Packet_Parameter3 <= 59)
      {
	/*!< sets the time with packet parameters given by PC */
        RTC_Set(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
        /*!< returns the original packet to the PC if successful */
        return Packet_Put(SET_TIME_COMMAND, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
      }
    }
  }
  return false;
}


/*! @brief Process the packet that has been received
 *
 *  @return bool - TRUE if the packet has been handled properly.
 *  @note Assumes that Packet_Init and Packet_Get was called
 */
bool PacketHandler(void)
{
  /*!<  Packet Handler used if Packet Get is successful*/
  bool actionSuccess;  /*!<  Acknowledge is false as long as the package isn't acknowledge or if it's not required */
  switch (Packet_Command & ~PACKET_ACK_MASK)
  {
    case TOWER_STARTUP_COMMAND:
      actionSuccess = StartupPackets();
      break;

    case TOWER_VERSION_COMMAND:
      actionSuccess = VersionPackets();
      break;

    case TOWER_NUMBER_COMMAND:
      actionSuccess = TowerNumberPackets();
      break;

    case TOWER_MODE_COMMAND:
      actionSuccess = TowerModePackets();
      break;

    case FLASH_PROGRAM_COMMAND:
      actionSuccess = ProgramBytePackets();
      break;

    case FLASH_READ_COMMAND:
      actionSuccess = ReadBytePackets();
      break;

    case SET_TIME_COMMAND:
      actionSuccess = TowerTimePackets();
      break;

    case PROTOCOL_COMMAND:
      actionSuccess = ProtocolPackets();
      break;

  }

  if (Packet_Command & PACKET_ACK_MASK) /*!< if ACK bit is set, need to send back ACK packet if done successfully and NAK packet with bit7 cleared */
  {
    if (actionSuccess)
    {
      Packet_Put(Packet_Command, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    }
    else
    {
      Packet_Put((Packet_Command |=PACKET_ACK_MASK),Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    }
  }
}

/*! @brief Triggered during interrupt, turns yellow LED on/off and send a packet containing time every sec and handles the polling of the accelerometer
 *
 *  @return void
 *  @note Assumes that RTC_Init called
 */
void RTCCallback(void)
{
  LEDs_Toggle(LED_YELLOW);
  uint8_t hours, minutes, seconds;
  RTC_Get(&hours, &minutes, &seconds);
  Packet_Put(SET_TIME_COMMAND, hours, minutes, seconds);
  if (CurrentMode == ACCEL_POLL) /*!< using RTC for ACCEL_POLL as the freq wanted is 1Hz and P = 1/Hz = 1 s*/
  {
    uint8_t data[3];
    Accel_ReadXYZ(data);
    if (data) /*!< If data is not null, values has changed from before so send new packets */
    {
      Packet_Put(ACCEL_COMMAND, data[0], data[1], data[2]);
      LEDs_Toggle(LED_GREEN);
    }
  }
}

/*! @brief Triggered during interrupt, toggles green LED
 *
 *  @return void
 *  @note Assumes that PIT_Init called
 */
void PITCallback(void)
{
  LEDs_Toggle(LED_GREEN);
}

/*! @brief Triggered during interrupt, turns blue LED off
 *
 *
 *  @param arguments
 *  @return void
 *  @note Assumes that FTM_Init & FTM_Set and FTM_StartTimer called
 */
void FTM0Callback(void* arguments)
{
  LEDs_Off(LED_BLUE);
}

/*! @brief Handles the protocol packets
 *
 *  @return bool - TRUE if packet has been sent and handled successfully
 *  @note Assumes that Packet_Init and Accel_Init was called
 */
bool ProtocolPackets(void)
{
  if (Packet_Parameter1 == PROTOCOL_GET) /*!< Get the Protocol Mode */
  {
    if (CurrentMode == ACCEL_POLL)
    {
      Packet_Put(PROTOCOL_COMMAND, PROTOCOL_PARAMETER1, PROTOCOL_ASYNC, PROTOCOL_PARAMETER3); /*!< Send the current mode to the PC*/
    }
    else if (CurrentMode == ACCEL_INT)
    {
      Packet_Put(PROTOCOL_COMMAND, PROTOCOL_PARAMETER1, PROTOCOL_SYNC, PROTOCOL_PARAMETER3); /*!< Send the current mode to the PC*/
    }
  }
  else if (Packet_Parameter1 == PROTOCOL_SET) /*!< Set the Protocol Mode */
  {
    if (Packet_Parameter2 == PROTOCOL_ASYNC) /*!< If parameter2 == 0, set the mode to poll - asynchronous */
    {
      Accel_SetMode(ACCEL_POLL);
      Packet_Put(PROTOCOL_COMMAND, PROTOCOL_PARAMETER1, PROTOCOL_ASYNC, PROTOCOL_PARAMETER3);
    }
    else if (Packet_Parameter2 == PROTOCOL_SYNC) /*!< If parameter2 == 1, set the mode to interrupt - synchronous  */
    {
      Accel_SetMode(ACCEL_INT);
      Packet_Put(PROTOCOL_COMMAND, PROTOCOL_PARAMETER1, PROTOCOL_SYNC, PROTOCOL_PARAMETER3);
    }
  }
}

/*! @brief triggered during accel interrupt

 *  @note Assumes that Tower_Init was called successfully
 */
void AccelDataCallback(void* arg)
{
  Packet_Put(ACCEL_COMMAND, TempArray[0], TempArray[1], TempArray[2]); /*!< Send the packet when the data is ready */
}

/*! @brief triggered during i2c interrupt

 *  @note Assumes that Tower_Init was called successfully
 */
void AccelReadCallback(void* arg)
{
  Accel_ReadXYZ(TempArray); /*!< Store the values in TempArray so it can be accessed by AccelReadCallback*/
  LEDs_Toggle(LED_GREEN); /*!< Toggle the Green LED at a rate of 1.56hz*/
}

/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
/*!
* @}
*/
