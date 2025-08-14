/**
 * @file
 * Definitions of simulated ISR functions.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_SIM_ISR_H_
#define NUVC_SIM_ISR_H_

#include <stdint.h>

/**
 * A function to be invoked from the main thread.
 *
 * This is the basis on which drivers can simulate ISRs.
 */
typedef void (*SimISR)(int arg);

/**
 * Initialize the ISR simulation layer.
 *
 * This must be called from the main thread before any interrupts are
 * simulated.
 */
extern int SimInitISRThread(void);

/**
 * Interrupts the main thread and simulates an ISR.
 *
 * @param[in] isr   points to a function to call in ISR context
 * @return          0 on success, non-zero otherwise
 */
extern int SimInterruptMainThread(SimISR isr, int arg);

/**
 * Disables simulated interrupts.
 *
 * @return state to pass into SimEnableInterrupts later
 */
extern uint16_t SimDisableInterrupts(void);

/**
 * Enables simulated interrupts.
 *
 * @param[in] stateToRestore    value returned by SimDisableInterrupts
 */
extern void SimEnableInterrupts(uint16_t stateToRestore);

#endif /* NUVC_SIM_ISR_H_ */
