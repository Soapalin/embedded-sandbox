/* ###################################################################
**     Filename    : UART.c
**     Project     : Lab2
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-04-16, 10:23, # CodeGen: 0
**     Abstract    :
**         UART module
**     Settings    :
**     Contents    :
**         public methods
**
** ###################################################################*/
/*!
** @file UART.c
** @brief
**         UART module.
**         This module contains code for UART control.
*/
/*!
**  @addtogroup UART_module UART module documentation
**   @{@}
*/
/*! @file
 *
 *  @brief Routines to access the UART on the TWR-K70F120M.
 *
 *  This contains the functions for operating the UART.
 *
 *  @author Lucien Tran & Angus Ryan
 *  @date 2019-04-16
 */

#include "Cpu.h"
#include "types.h"
#include "MK70F12.h"
#include "FIFO.h"
#include "UART.h"


static TFIFO TxFIFO; /*!< private global variable of type struct TxFIFO defined */
static TFIFO RxFIFO; /*!< private global variable of type struct RxFIFO defined */

bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  if (baudRate == 0) /*!< ensure baudRate isn't 0 otherwise causing crashes during the during baud rate divisor calculations */
  {
    return false;
  }
  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK; /*!< enable the UART2 (bit 12) module (pg. 345, 346) MUST BE ENABLED BEFORE ACCESSING CONTROL REGISTER 2. */
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK; /*!<To enable pin routing for PortE (bit 13) (pg. 347) where we are referencing Pin Control Registers (PCR) 16 & 17. */

  PORTE_PCR16 = PORT_PCR_MUX(3); /*!< Setting the MUX bits (8, 9, 10) in the PORTE_PCR16/17 register to high. (pg. 316,317) */
  PORTE_PCR17 = PORT_PCR_MUX(3); /*!< Setting the MUX bits (8, 9, 10) in the PORTE_PCR16/17 register to high. (pg. 316,317) */

  uint16union_t bdsbr; /*!< defining BDSBR as a union to call upper and lower components of the value (To SET BDH & BDL). */
  uint8_t sampleRate = 16; /*!< Defining sample rate for BaudRate Calculation. */
  bdsbr.l = (moduleClk/(sampleRate*baudRate)); /*!< define BaudRate as a Union to split it UART2 Baud Rate High and Low. */

  UART2_C4 |= UART_C4_BRFA(32*(moduleClk%(sampleRate*baudRate))/(sampleRate*baudRate)); /*!< Setting BRFA using modulus to calculate the remainder (pg. 1922, 1972) */

  UART2_C2 &= ~UART_C2_TE_MASK; /*!< Disable UART transmitter */
  UART2_C2 &= ~UART_C2_RE_MASK; /*!< Disable UART receiver */

  UART2_BDH |= UART_BDH_SBR(bdsbr.s.Hi); /*!< set upper baud rate value */
  UART2_BDL = UART_BDL_SBR(bdsbr.s.Lo); /*!< set lower baud rate value */

  UART2_C2 = UART_C2_TE_MASK | UART_C2_RE_MASK; /*!< Enabling UART, Transmitter and Receiver (bit 3 & bit 2 of UART control register 2). (pg. 1911-1912) */

  FIFO_Init(&RxFIFO); /*!< Initiate FIFO Receiver */
  FIFO_Init(&TxFIFO); /*!< initiate FIFO Transmitter */

  return true; /*!< return true if it has a character otherwise false. */

}

bool UART_InChar(uint8_t* const dataPtr)
{
  return (FIFO_Get(&RxFIFO, dataPtr));  /*!< Attempt to GET data from RxFIFO if empty return false, if data return true */
}

bool UART_OutChar(const uint8_t data)
{
  return (FIFO_Put(&TxFIFO, data));  /*!< Attempt to PUT data from TxFIFO if empty return false, if data return true */
}

void UART_Poll(void)
{
  if (UART2_S1 & UART_S1_RDRF_MASK) /*!< Checking UART2 Status Register (pg. 1913) as well as the checking the 6th bit of the register (Receive Data Register Full Flag - Bit 5) to see if there are received packets */
  {
      FIFO_Put(&RxFIFO, UART2_D);  /*!< receiving data, FIFO puts value of UART2_D (pg. 1919). UART2_D Reads return the contents of the read-only receive data register and writes go to the write-only transmit data register. */
  }
  if (UART2_S1 & UART_S1_TDRE_MASK) /*!< Checking UART2 Status Register (pg. 1913) as well as the checking the 8th bit of the register (Transmit Data Register Empty Flag - Bit 7) to see if the flag has been raised */
    {
      FIFO_Get(&TxFIFO, (uint8_t *) &UART2_D); /*!< transmitting data, FIFO gets the value at the address of UART2_D (pg. 1919). UART2_D Reads return the contents of the read-only receive data register and writes go to the write-only transmit data register. */
  }
}
