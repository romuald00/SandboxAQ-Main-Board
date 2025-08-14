/**
 * @file
 * Implementation of the Finite State Machine.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_FSM_H_
#define NUVC_FSM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

/**
 * Allows for easy declarations of FSM states.
 *
 * Example usage:
 * @code
 *      NEW_FSM_STATE(FsmState1);
 *      NEW_FSM_STATE(FsmState2);
 *
 *      NEW_FSM_STATE(FsmState1)
 *      {
 *          MOVE_TO_STATE(FsmState2);
 *      }
 *
 *      NEW_FSM_STATE(FsmState2)
 *      {
 *          MOVE_TO_STATE(FsmState1);
 *      }
 * @endcode
 */
#define NEW_FSM_STATE(name) static void name(FSM_STATE_MACHINE *sm)

/**
 * Allows for easy transitioning between FSM states.
 *
 * Example usage: Refer to #NEW_FSM_STATE.
 */
#define MOVE_TO_STATE(name)                                                                                            \
    do {                                                                                                               \
        sm->state = name;                                                                                              \
        return;                                                                                                        \
    } while (0)

/** Macro for defining states that should be accessible by other modules. */
#define NEW_GLOBAL_FSM_STATE(name) void name(FSM_STATE_MACHINE *sm)

/**
 * Enters a critical section.
 *
 * This function needs to be defined outside of this module either by being
 * implemented or redefined as a pre-processor macro.
 *
 * Example usage:
 * @code
 *     uint16_t prevState;
 *
 *     prevState = FSM_EnterCriticalSection();
 *     // ... awesome critical code
 *     FSM_ExitCriticalSection(prevState);
 * @endcode
 *
 * @return  a value to pass to a matching FSM_ExitCriticalSection
 */
extern uint16_t FSM_EnterCriticalSection(void);

/**
 * Exits a critical section.
 *
 * This function needs to be defined outside of this module either by being
 * implemented or redefined as a pre-processor macro.
 *
 * Example usage: Refer to FSM_EnterCriticalSection(void)
 *
 * @param[in] stateToRestore    the value returned by FSM_EnterCritical
 */
extern void FSM_ExitCriticalSection(uint16_t stateToRestore);

/**
 * An enumerated type used to identify the various finite state machine
 * callback functions.
 */
typedef enum {
    /** The callback should put the processor into sleep mode. */
    FSM_ACTION_ALL_STATES_IDLE,
    /** The callback should perform whatever periodic operation it wants
     * (e.g. feed watchdog, profiling, etc.). */
    FSM_ACTION_STATE_BEGIN_EXECUTION_CYCLE,
    /** The callback should perform whatever periodic operation it wants
     * (e.g. feed watchdog, profiling, etc.). */
    FSM_ACTION_STATE_EXECUTION_COMPLETE
} FSM_ACTION;

typedef struct _fsm_state_machine FSM_STATE_MACHINE;

typedef void (*FSM_STATE)(FSM_STATE_MACHINE *sm);
typedef void (*FSM_CALLBACK_FUNC)(FSM_ACTION);
typedef uint16_t FSM_SIGNAL_TYPE;

/** Data structure to represent a single state machine. */
struct _fsm_state_machine {
    FSM_STATE state;
    volatile void *context;
    FSM_SIGNAL_TYPE signalState;
    FSM_SIGNAL_TYPE signalMask;
    volatile FSM_SIGNAL_TYPE signalSet;
    volatile FSM_SIGNAL_TYPE signalClear;
};

/**
 * Executes a set of finite state machines.
 *
 * @param[in] fsmList
 *     A pointer to an array of pointers to state machines which should be
 *     executed.
 *
 * @param[in] fsmCount
 *     the number of FSMs in the list to be executed.
 *
 * @param[in] fsmCallback
 *     A pointer to a user defined callback function which will be called to
 *     report the execution status of the state machine framework.
 */
void FSM_Execute(FSM_STATE_MACHINE *const fsmList[], unsigned int fsmCount, FSM_CALLBACK_FUNC const fsmCallback);

#define FSM_SetSignal(fsm, mask)                                                                                       \
    do {                                                                                                               \
        uint16_t prevState = FSM_EnterCriticalSection();                                                               \
        (fsm)->signalSet |= (mask);                                                                                    \
        FSM_ExitCriticalSection(prevState);                                                                            \
    } while (0)

#define FSM_ClearSignal(fsm, mask)                                                                                     \
    do {                                                                                                               \
        uint16_t prevState = FSM_EnterCriticalSection();                                                               \
        (fsm)->signalClear |= (mask);                                                                                  \
        FSM_ExitCriticalSection(prevState);                                                                            \
    } while (0)

#define FSM_CheckSignal(fsm, mask) ((fsm)->signalState & (mask))

#define FSM_SetSignalFromISR(fsm, mask) ((fsm)->signalSet |= (mask))
#define FSM_ClearSignalFromISR(fsm, mask) ((fsm)->signalClear |= (mask))

#define FSM_SetSignalMask(fsm, mask) ((fsm)->signalMask |= (mask))
#define FSM_ClearSignalMask(fsm, mask) ((fsm)->signalMask &= ~(mask))

#ifdef __cplusplus
}
#endif

#endif /* NUVC_FSM_H_*/
