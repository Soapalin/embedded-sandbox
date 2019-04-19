/* ###################################################################
**     Filename    : FIFO.c
**     Project     : Lab2
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-04-16, 10:23, # CodeGen: 0
**     Abstract    :
**         FIFO module
**     Settings    :
**     Contents    :
**         public methods
**
** ###################################################################*/
/*!
** @file FIFO.c
** @brief
**         FIFO module.
**         This module contains code for FIFO control.
*/
/*!
**  @addtogroup FIFO_module FIFO module documentation
**   @{@}
*/
/*! @file
 *
 *  @brief Routines to access the FIFO on the TWR-K70F120M.
 *
 *  This contains the functions for operating the FIFO.
 *
 *  @author Lucien Tran & Angus Ryan
 *  @date 2019-04-16
 */


#include "FIFO.h"

#define FIFO_SIZE 256 /*!<  Number of bytes in a FIFO */

bool FIFO_Init(TFIFO * const fifo) /*!<  Initiate the FIFO */
{
  fifo->Start = fifo->End = 0; /*!<  Make both head and tails of the FIFO = 0, therefore empty */
  fifo->NbBytes = 0; /*!< Number of the bytes taken by the Buffer is equal to 0 when it is empty */
  return true;
}

bool FIFO_Put(TFIFO * const fifo, const uint8_t data)
{
  if (fifo->NbBytes == 256) /*!<  If FIFO is full */
  {
    return false; /*!< FIFO is full, no space to write new  data */
  } /*!<  FIFO was full, so deleting oldest data to replace with new one - will lose data every time FIFO-Put is called and it is full to avoid CPU interrupt error */
  else
  {
    fifo->Buffer[fifo->End] = data; /*!<  Storing data in the new element of the array */
    if (fifo->End == FIFO_SIZE)
    {
      fifo->End = 0; /*!<  Check for wrap around */
    }
    else
    {
      fifo->End++; /*!< Create a potential new element in the Buffer Array to store the data */
    }
    fifo->NbBytes++; /*!< Increase the number of bytes = new data is written - to keep track of the number of bytes stored */
    return true; /*!< Successful */
  }
}

bool FIFO_Get(TFIFO * const fifo, uint8_t * const dataPtr)
{
  if (fifo->NbBytes == 0) /*!< Empty if PutPt == GetPt */
  {
    return false; /*!< return false when it is empty */
  }
  else
  {
    *dataPtr = fifo->Buffer[fifo->Start]; /*!< value at the address is equal to the character */
    if (fifo->Start == FIFO_SIZE)
    {
      fifo->Start = 0; /*!<  Check for wrap around */
    }
    else
    {
      fifo->Start++;
    }
    fifo->NbBytes--; /*!< Getting the value is getting rid of the oldest value - Number of bytes decreases */
    return true; /*!<  Successful */
  }
}


