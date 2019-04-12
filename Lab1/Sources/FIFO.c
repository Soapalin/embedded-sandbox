/* ###################################################################
**     Filename    : FIFO.c
**     Project     : Lab1
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-03-21, 14:11, # CodeGen: 0
**     Abstract    :
**         FIFO module.
**         This module contains FIFO function.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 2.0
** @brief
**         FIFO module.
**         This module contains user's application code.
*/
/*!
**  @addtogroup main_module main module documentation
**  @{
*/


#include "FIFO.h"

// Number of bytes in a FIFO
#define FIFO_SIZE 256


bool FIFO_Init(TFIFO * const fifo) {
  // Initiate the FIFO
  fifo->Start = fifo->End = 0; //Make both head and tails of the FIFO = 0, therefore empty
  fifo->NbBytes = 0; // Number of the bytes taken by the Buffer is equal to 0 when it is empty
  return true;

}

bool FIFO_Put(TFIFO * const fifo, const uint8_t data)
{
  fifo->End++; // Create a potential new element in the Buffer Array to store the data
  if (fifo->NbBytes == 256) // If FIFO is full
    {
      return false; //FIFO is full, no space to write new  data
    } //  FIFO was full, so deleting oldest data to replace with new one - will lose data every time FIFO-Put is called and it is full to avoid CPU interrupt error
  else
  {
    fifo->Buffer[fifo->End] = data; // Storing data in the new element of the array
    if (fifo->End == FIFO_SIZE)
    {
      fifo->End = 0; // Check for wrap around
    }
    fifo->NbBytes++; // Increase the number of bytes = new data is written - to keep track of the number of bytes stored
    return true; // Successful
  }
}

bool FIFO_Get(TFIFO * const fifo, uint8_t * const dataPtr)
{
  if (fifo->NbBytes == 0) // Empty if PutPt == GetPt
  {
    return false; // return false when it is empty
  }
  else
  {
    if (fifo->Start == FIFO_SIZE)
    {
      fifo->Start = 0; // Check for wrap around
    }
    else
    {
      fifo->Start++;
    }
    *dataPtr = fifo->Buffer[fifo->Start]; //value at the address is equal to the character
    fifo->NbBytes--; // Getting the value is getting rid of the oldest value - Number of bytes decreases
    return true; // Successful
  }
}


