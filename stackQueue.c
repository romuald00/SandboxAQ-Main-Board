/*
 * stackQueue.c
 * Keeps a push pop queue with elements that are constantly being reused.
 *
 * User pops an item to be used.
 * User push the item back when they are finished with it.
 *  Created on: Oct 28, 2023
 *      Author: rlegault
 */
#include "stackQueue.h"
#include "debugPrint.h"
#include "stmTarget.h"
#include <stdlib.h>

#define TEST_FOR_DOUBLE_INSERTION 1

stackQ_tp stackQInit(int count, void *(*elementGenerator)(void)) {

    stackQ_tp stackQ_p = malloc(sizeof(stackQ_t));
    assert(stackQ_p != NULL);

    stackQ_p->nxtAvailableIdx = 0;

    stackQ_p->stackQ = malloc((count + 1) * sizeof(void *));
    assert(stackQ_p->stackQ != NULL);

    stackQ_p->stackQ[count] = NULL; // identifies the end of the stack.
    for (int i = 0; i < count; i++) {
        stackQ_p->stackQ[i] = elementGenerator();
    }
    DPRINTF_STACKQ("created stackQ at %p with %d elements\r\n", stackQ_p, count);
    return stackQ_p;
}

void *stackQPop(stackQ_tp dev_p) {
    assert(dev_p != NULL);
    __disable_irq();
    void *ele = dev_p->stackQ[dev_p->nxtAvailableIdx];
    if (ele != NULL) {
        dev_p->nxtAvailableIdx++;
    }
    __enable_irq();
    return ele;
}

void stackQPush(stackQ_tp dev_p, void *ele) {
    assert(dev_p != NULL);
    assert(ele != NULL);
#if TEST_FOR_DOUBLE_INSERTION
    void *test;
    int nxt = dev_p->nxtAvailableIdx;
    do {
        test = dev_p->stackQ[nxt];
        if (test == ele) {
            DPRINTF_ERROR("Duplicate found of ele %p at idx %d\r\n", ele, nxt);
        }
        nxt++;
        assert(test == ele);
    } while (test != NULL);
#endif
    __disable_irq();
    dev_p->nxtAvailableIdx--;
    assert(dev_p->nxtAvailableIdx >= 0);
    dev_p->stackQ[dev_p->nxtAvailableIdx] = ele;
    __enable_irq();
    return;
}
