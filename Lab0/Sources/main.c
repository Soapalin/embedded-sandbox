/*!
** @file
** @version 1.0
** @brief
**         Main module.
**         This module implements a simple 12-hour clock.
**         It time-stamps button pushes and stores them in a FIFO used a packed representation.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


// CPU module - contains low level hardware initialization routines
#include "Cpu.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"

// Simple timer
#include "timer.h"

// Button functions
#include "buttons.h"

// LED functions
#include "LEDs.h"

// The packed time representation

//   15             12   11                        6    5                       0
// |----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|
// |       hours       |          minutes            |          seconds            |

typedef uint16_t PackedTime_t;
uint16_t Timer;
uint8_t Seconds,Hours,Minutes;  //GLOBAL VARIABLE

// ***
// You will need to create a FIFO object with a size suitable to store 10 time-stamps using the packed time representation.
// ***
#define TIMESTAMP_FIFO_SIZE 10 //size 10 FIFO 8-bit Time
PackedTime_t* PutPt; // Pointer of where to put next
PackedTime_t* GetPt; // Pointer of where to get next
// FIFO is empty if PutPt == GetPt
// FIFO is full if PutPt + 1 == GetPt
PackedTime_t FIFO[TIMESTAMP_FIFO_SIZE]; // The statically allocated FIFO data
void FIFO_Init(void)
{
 // Make atomic, entering critical section
 PutPt = GetPt = FIFO; // Empty when PutPt == GetPt
}

int FIFO_Get(PackedTime_t* const dataPt)
{
  if (PutPt == GetPt) // Empty if PutPt == GetPt
  {
    return 0;
  }
   // Make atomic, entering critical section
  *dataPt = *GetPt++;
  if (GetPt == &FIFO[TIMESTAMP_FIFO_SIZE]) {
    GetPt = FIFO;
    return 1;
  }

}

int FIFO_Put(const PackedTime_t timestamp)
{
 PackedTime_t* pt; // Temporary put pointer
 // Make atomic, entering critical section
 pt = PutPt + 1; // Make new potential PutPt
 if (pt == &FIFO[TIMESTAMP_FIFO_SIZE])
   pt = FIFO; // Wrap pointer if necessary
   if (pt == GetPt) // If FIFO is full, fail
   {
       FIFO_Get(pt); //get the oldest value and take it out of the FIFO
       *PutPt = timestamp; // Put data into FIFO
       PutPt = pt; // Update PutPt
       return 1; // Successful
   } // Failed, FIFO was full
   else
   {
       *PutPt = timestamp; // Put data into FIFO
       PutPt = pt; // Update PutPt
       return 1; // Successful
   }
}

PackedTime_t GetTime(void) {
  int inputSecond= Timer;
  int remainingSeconds;
  int secondsInHour = 60 * 60;
  int secondsInMinute = 60;
  Hours = (inputSecond/secondsInHour);
  remainingSeconds = inputSecond - (Hours * secondsInHour);
  Minutes = remainingSeconds/secondsInMinute;
  remainingSeconds = remainingSeconds - (Minutes*secondsInMinute);
  Seconds = remainingSeconds;
  PackedTime_t total = ((uint16_t)Hours<<12) + ((uint16_t) Minutes<<6) + Seconds;
  // DecodeTime(total);
  return total;
}

static void DecodeTime(PackedTime_t total)
{
  Seconds = total - Hours - Minutes;
  Minutes = (total - Hours)>>6;
  Hours = total>>12;
}

static void OneSecondElapsed(void)
{
  LEDs_Toggle(LED_BLUE);
  // One second has elapsed - update the time here
  Timer++;
}

static void Button1Pressed(void)
{
  LEDs_Toggle(LED_ORANGE);
  PackedTime_t eventtimestamp = GetTime(); // The button has been pressed - get time (call time function?) - return current time
  // The button has been pressed - put a time-stamp into the FIFO
  FIFO_Put(eventtimestamp);
}

static void TowerInit(void)
{
  PE_low_level_init();
  Timer_Init(OneSecondElapsed);
  Buttons_Init(Button1Pressed);
  LEDs_Init();
  __EI();
}

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/
  TowerInit();
  /* Write your code here */
  FIFO_Init();
  for (;;)
  {
  }
}

/* END main */
/*!
** @}
*/
