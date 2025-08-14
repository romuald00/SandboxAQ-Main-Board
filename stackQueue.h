/*
 * @file
 * @brief Create a stack queue that will hold memory locations. These memory locations
 * are pop off the stack when needed and pushed back onto the stack when not in use.
 *
 * @func elementGenerator that is passed into stackQInit will be used to generate pointers
 * to the reuseable memory locations.
 *
 * @func stackQPop to acquire the next unused memory location. Must use StackQPush to release
 * the memory location
 *
 * @func stackQPush to release the memory location for reuse.
 *
 *  Created on: Oct 29, 2023
 *      Author: rlegault
 */

#ifndef NUVC_INC_STACKQUEUE_H_
#define NUVC_INC_STACKQUEUE_H_

typedef struct {
    int nxtAvailableIdx;
    void **stackQ;
} stackQ_t, *stackQ_tp;

/**
 * @brief Create stackQ_t for storage of elements created by
 * elementGenerator. Each element of the store is a pointer to
 * a memory area created by elementGenerator
 *
 * @param count int: number of elements to store in the stackq
 * @param elementGenerator void*(fn): each call to this function must
 * return an unique memory location.
 *
 *
 * @return Initialized stackQ_t if successful. Otherwise, NULL
 */

stackQ_tp stackQInit(int count, void *(*elementGenerator)(void));

/**
 * @brief Return the next unused memory location for use.
 *
 * @param dev_p stackQ_tp: initialized stack structure returned by init.
 *
 * @return unique memory location or null if they are all in use.
 */

void *stackQPop(stackQ_tp dev_p);

/**
 * @brief Put the ele back onto dev_p for reuse.
 *
 * @param dev_p stackQ_tp: initialized stack structure returned by init.
 * @param ele void*: value that was returned by stackQPop
 *
 * @return unique memory location or null if they are all in use.
 */

void stackQPush(stackQ_tp dev_p, void *ele);

#endif /* NUVC_INC_STACKQUEUE_H_ */
