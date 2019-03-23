/*! @file
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *  @author PMcL
 *  @date 2015-07-23
 */

#ifndef TIMER_H
#define TIMER_H

// new types
#include "types.h"

/*! @brief Initializes the Timer before first use.
 *
 *  Sets up the internal SysTick clock.
 *  Registers the user's callback function.
 *  @param userFunction is a pointer to a user callback function.
 */
void Timer_Init(void (*userFunction)(void));

/*! @brief Interrupt service routine for the timer.
 *
 *  The internal systick clock has ticked.
 *  The user callback function will be called after 1 second has elapsed.
 *  @note Assumes the Timer has been initialized.
 */
void __attribute__ ((interrupt)) Timer_TickISR(void);

#endif
