/**
 * @file
 * Implementation of the Finite State Machine.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#include <stdbool.h>

#include "nuv_fsm.h"

void FSM_Execute(FSM_STATE_MACHINE *const fsmList[], unsigned int fsmCount, FSM_CALLBACK_FUNC const fsmCallback) {
    FSM_STATE_MACHINE *fsm;
    unsigned int i;
    bool noStateMachinesExecuted;

    /* Loop over the list forever. */
    for (;;) {
        /* Assume no state machines require execution for this pass. */
        noStateMachinesExecuted = true;

        /* Call the state begin execution cycle callback. */
        if (fsmCallback != NULL)
            fsmCallback(FSM_ACTION_STATE_BEGIN_EXECUTION_CYCLE);

        /* Loop through all the state machines... */
        for (i = 0; i < fsmCount; i++) {
            /* Get an aliased pointer (for optimization). */
            fsm = fsmList[i];

            /* Enter critical section by disabling interrupts. */
            uint16_t prevState = FSM_EnterCriticalSection();

            /* Update the signal state by clearing any signals which require
             * clearing and setting any signals which require setting. */
            fsm->signalState = (fsm->signalState & ~fsm->signalClear) | fsm->signalSet;

            /* Both the signal set and clear flags are auto-clearing. */
            fsm->signalSet = fsm->signalClear = 0;

            /* Leave critical section by re-enabling interrupts. */
            FSM_ExitCriticalSection(prevState);

            /* Only process the state machine if a signal for which a mask is
             * set is asserted. */
            if ((fsm->signalMask & fsm->signalState) && fsm->state) {
                /* Invoke the state machine's current state function passing in
                 * the state machine's execution context. */
                fsm->state(fsm);

                /* Call the state execution complete callback. */
                if (fsmCallback != NULL)
                    fsmCallback(FSM_ACTION_STATE_EXECUTION_COMPLETE);

                /* Remember that a state machine was executed. */
                noStateMachinesExecuted = false;
            }
        }

        /* No state machines required execution in this pass through the list.
         */
        if (noStateMachinesExecuted && fsmCallback != NULL)
            fsmCallback(FSM_ACTION_ALL_STATES_IDLE);
    }
}
