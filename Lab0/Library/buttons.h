/*! @file
 *
 *  @brief HAL for pushbutton switches on the TWR-K70.
 *
  *  This contains the functions for the pushbutton switches on the TWR-K70.
*
 *  @author PMcL
 *  @date 2015-10-25
 */

#ifndef BUTTONS_H
#define BUTTONS_H

// New types
#include "types.h"

/*! @brief Initializes the pushbutton switches.
 *
 *  @param buttonSetup is a pointer to a switch setup structure.
 *  @param userFunction is a pointer to a user callback function.
 */
bool Buttons_Init(void (*userFunction)(void));

/*! @brief Interrupt service routine for Port D buttons.
 *
 *  A button in Port D has been pressed.
 *  The user callback function will be called.
 *  @note Assumes the button has been initialized.
 */
void __attribute__ ((interrupt)) PortD_ISR(void);

/*! @brief Interrupt service routine for Port E buttons.
 *
 *  A button in Port E has been pressed.
 *  The user callback function will be called.
 *  @note Assumes the button has been initialized.
 */
void __attribute__ ((interrupt)) PortE_ISR(void);

#endif
