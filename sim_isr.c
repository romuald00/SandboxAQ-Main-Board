/**
 * @file
 * Implementation of simulated ISR functions.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "sim/sim_isr.h"

#ifdef __CYGWIN__
#define MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK
#else
#define MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK_NP
#endif

static pthread_t mainThread;
static pthread_mutex_t isr_mutex;

int SimInitISRThread(void) {
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    assert(0 == pthread_mutexattr_settype(&attr, MUTEX_TYPE));
    assert(0 == pthread_mutex_init(&isr_mutex, &attr));

    mainThread = pthread_self();
    pthread_mutex_unlock(&isr_mutex);

    return 0;
}

int SimInterruptMainThread(SimISR isr, int arg) {
    pthread_mutex_lock(&isr_mutex);
    isr(arg);
    pthread_mutex_unlock(&isr_mutex);

    return 0;
}

uint16_t SimDisableInterrupts(void) {
    if (!pthread_equal(pthread_self(), mainThread))
        return 0;

    if (EDEADLK == pthread_mutex_lock(&isr_mutex))
        return 0;

    return 1;
}

void SimEnableInterrupts(uint16_t stateToRestore) {
    if (!pthread_equal(pthread_self(), mainThread))
        return;

    if (stateToRestore == 1)
        pthread_mutex_unlock(&isr_mutex);
}
