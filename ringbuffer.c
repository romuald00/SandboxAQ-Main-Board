/**
 * @file
 * This file implements the API for using a space efficient ring buffer.
 *
 * Copyright Nuvation Research Corporation 2011-2017. All Rights Reserved.
 * www.nuvation.com
 */

#include <stdbool.h>

#include "ringbuffer.h"

/**
 * Determines whether the given ring buffer is empty.
 *
 * @param[in] ringBuffer    the buffer to be queried
 * @return                  true if the buffer is empty, false otherwise
 */
#define RingBufferIsEmpty(ringBuffer) ringBuffer->writePointer == ringBuffer->readPointer

/**
 * Determines the size of the free space in the ringbuffer.
 *
 * @param[in] ringBuffer    the buffer to be queried
 */
static uint16_t SmallRingBufferGetFreeBytes(volatile const SmallRingBuffer *const ringBuffer) {
    return (ringBuffer->size - 1 + ringBuffer->readPointer - ringBuffer->writePointer) & (ringBuffer->size - 1);
}

void SmallRbInit(volatile SmallRingBuffer *rb, uint16_t size) {
    /* Initialize the buffer as empty. */
    rb->readPointer = 0;
    rb->writePointer = 0;

    rb->size = size - sizeof(SmallRingBuffer) + 1;
}

int16_t SmallRbDequeue(volatile SmallRingBuffer *ringBuffer) {
    uint16_t rPtr = ringBuffer->readPointer;
    int16_t data;

    if (rPtr != ringBuffer->writePointer) {
        data = ringBuffer->data[rPtr];
        rPtr = (rPtr + 1) & (ringBuffer->size - 1);
    } else
        data = RB_BUFFER_UNDERRRUN;

    ringBuffer->readPointer = rPtr;

    return data;
}

uint16_t SmallRbRead(volatile SmallRingBuffer *ringBuffer, uint8_t *data, uint16_t length) {
    uint16_t numBytesRead = 0;
    uint16_t rPtr = ringBuffer->readPointer;

    while (rPtr != ringBuffer->writePointer && numBytesRead < length) {
        data[numBytesRead] = ringBuffer->data[rPtr];
        rPtr = (rPtr + 1) & (ringBuffer->size - 1);

        numBytesRead++;
    }

    ringBuffer->readPointer = rPtr;

    if (numBytesRead < length)
        data[numBytesRead] = '\0';

    return numBytesRead;
}

uint16_t SmallRbReadString(volatile SmallRingBuffer *ringBuffer, uint8_t *data, uint16_t length) {
    uint16_t numBytesRead = 0;
    uint16_t rPtr = ringBuffer->readPointer;

    while (rPtr != ringBuffer->writePointer && numBytesRead < length) {
        data[numBytesRead] = ringBuffer->data[rPtr];
        rPtr = (rPtr + 1) & (ringBuffer->size - 1);

        if (data[numBytesRead] == '\0') {
            ringBuffer->readPointer = rPtr;
            return numBytesRead;
        }

        numBytesRead++;
    }

    return 0;
}

uint16_t SmallRbWrite(volatile SmallRingBuffer *ringBuffer, const uint8_t *data, uint16_t _length) {
    uint16_t numBytesWritten = 0;
    uint16_t wPtr = ringBuffer->writePointer;
    uint16_t numLeft = SmallRingBufferGetFreeBytes(ringBuffer);
    uint16_t length = _length;

    if (numLeft < length)
        length = numLeft;

    while (numBytesWritten < length) {
        ringBuffer->data[wPtr] = data[numBytesWritten];
        wPtr = (wPtr + 1) & (ringBuffer->size - 1);

        numBytesWritten++;
    }

    ringBuffer->writePointer = wPtr;

    return numBytesWritten;
}

int16_t SmallRbEnqueue(volatile SmallRingBuffer *ringBuffer, uint8_t data) {
    int16_t retCode;
    uint16_t wPtr = ringBuffer->writePointer;
    uint16_t nwPtr = (wPtr + 1) & (ringBuffer->size - 1);

    if (nwPtr != ringBuffer->readPointer) {
        ringBuffer->data[wPtr] = data;
        wPtr = nwPtr;
        retCode = RB_OK;
    } else
        retCode = RB_BUFFER_OVERRUN;

    ringBuffer->writePointer = wPtr;

    return retCode;
}

uint16_t SmallRbGetFillLength(volatile const SmallRingBuffer *ringBuffer) {
    return ringBuffer->size - 1 - SmallRingBufferGetFreeBytes(ringBuffer);
}
