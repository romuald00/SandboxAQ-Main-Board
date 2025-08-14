/*
 * largeBuffer.c
 *
 *  Copyright Nuvation Research Corporation 2018-2025. All Rights Reserved.
 *
 *  Created on: Jan 28, 2025
 *      Author: rlegault
 */

#include "largeBuffer.h"
#include "macros.h"
#include "ramLog.h"
#include "saqTarget.h"
#include <string.h>

static osMutexId lbufMutexId = NULL;

extern char _LARGE_BUFFER_SIZE;
static int32_t __LARGE_BUFFER_SIZE = (int32_t)&_LARGE_BUFFER_SIZE;
__attribute__((section(".largebuffer"))) char largeBuffer[LARGE_BUFFER_SIZE_BYTES];

void largeBufferInit(void) {
    assert(LARGE_BUFFER_SIZE == __LARGE_BUFFER_SIZE);
    assert(lbufMutexId == NULL);
    osMutexDef(lbufMutex);
    lbufMutexId = osMutexCreate(osMutex(lbufMutex));
    assert(lbufMutexId != NULL);
}

osStatus largeBufferLock(uint32_t timeout_ms) {
    assert(lbufMutexId != NULL);
    return osMutexWait(lbufMutexId, timeout_ms);
}

void largeBufferUnlock(void) {
    assert(lbufMutexId != NULL);
    osMutexRelease(lbufMutexId);
}

uint32_t largeBufferSize_Bytes(void) {
    return LARGE_BUFFER_SIZE;
}

uint8_t *largeBufferGet(void) {
    return (uint8_t *)largeBuffer;
}

uint32_t usableLargeBufferLength(void) {
    uint32_t len = strnlen(largeBuffer, largeBufferSize_Bytes());
    return len;
}

char *largeBufferReadFrom(uint32_t offset) {
    if (offset < ARRAY_SIZE(largeBuffer)) {
        return &largeBuffer[offset];
    } else {
        return NULL;
    }
}

uint32_t usableRamLogLargeBufferLength(void) {
    uint32_t len = strnlen(&largeBuffer[RAM_LOG_HEADER_SIZE], largeBufferSize_Bytes() - RAM_LOG_HEADER_SIZE);
    return len;
}

char *largeRamLogBufferReadFrom(uint32_t offset) {
    if (RAM_LOG_HEADER_SIZE + offset < ARRAY_SIZE(largeBuffer)) {
        return &largeBuffer[RAM_LOG_HEADER_SIZE + offset];
    } else {
        return NULL;
    }
}
