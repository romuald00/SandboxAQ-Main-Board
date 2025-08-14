/**
 * @file
 * This file defines the API for using a space efficient ring buffer.
 *
 * Copyright Nuvation Research Corporation 2011-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_RINGBUFFER_H_
#define NUVC_RINGBUFFER_H_

#include <stddef.h>
#include <stdint.h>

#define RB_ERROR_OFFSET (0)

typedef enum {
    RB_OK = 0,
    RB_BUFFER_OVERRUN = RB_ERROR_OFFSET - 1,
    RB_BUFFER_UNDERRRUN = RB_ERROR_OFFSET - 2,
    LAST_RB_ERROR = RB_ERROR_OFFSET - 3,
} SmallRingBufferError;

typedef struct _SmallRingBufferAccessor {
    uint16_t readPointer;
    uint16_t writePointer;
    uint16_t size;
    uint8_t data[1];
} SmallRingBuffer;

#define SMALL_RINGBUFFER_TYPE(Log2Size)                                                                                \
    volatile union {                                                                                                   \
        uint8_t _buffer[(1 << Log2Size) + sizeof(SmallRingBuffer) - 1];                                                \
        SmallRingBuffer ringBuffer;                                                                                    \
    }

/**
 * Initializes a ring buffer.
 *
 * Please note that the storage size passed in for the ring buffer needs to be
 * a power of 2. This limitation might be lifted in the future, but for now it
 * allows for some good code optimizations.
 *
 * Please note that one byte in the data is used to differentiate between a
 * full and an empty ring buffer. This means that the usable space after
 * calling this function is 2^(log2size) - 1.
 *
 * @param[in,out] rb      the buffer to be initialized
 * @param[in]     size    size of the storage
 */
extern void SmallRbInit(volatile SmallRingBuffer *rb, uint16_t size);

/**
 * Reads the next set of bytes from the buffer.
 *
 * @param[in,out] rb       stores info about the ring buffer
 * @param[out]    data     will contain the read data
 * @param[in]     length   is the size of the data array
 * @return                 the number of bytes read into data
 */
extern uint16_t SmallRbRead(volatile SmallRingBuffer *rb, uint8_t *data, uint16_t length);

/**
 * Remove the first element from the ring buffer
 *
 * @param[in,out] rb       stores info about the ring buffer
 * @return                 negative number indicates error
 */
extern int16_t SmallRbDequeue(volatile SmallRingBuffer *ringBuffer);

/**
 * Reads a full string from the ring buffer.
 *
 * If there is no null-terminating character in the buffer, then this function
 * will not modify the queue and simply return zero. In other words, a return
 * value of zero indicates the the queue does not contain a full string.
 *
 * @param[in,out] ringBuffer    stores info about the ring buffer
 * @param[out]    data          will contain the read data
 * @param[in]     length        is the size of the data array
 * @return                      the number of bytes read into data
 */
extern uint16_t SmallRbReadString(volatile SmallRingBuffer *ringBuffer, uint8_t *data, uint16_t length);

/**
 * Writes data to the buffer.
 *
 * @param[in,out] ringBuffer    stores info about the ring buffer
 * @param[out]    data          the data to be written to the ring buffer
 * @param[in]     length        is the size of the data array
 * @return                      the number of bytes written to the ring buffer
 */
extern uint16_t SmallRbWrite(volatile SmallRingBuffer *ringBuffer, const uint8_t *data, uint16_t length);

/**
 * Write a single byte to the end of the ring buffer
 * @param[in,out] ringBuffer  stores info about the ring buffer
 * @param[in]     data        the data to be written
 * @return                    APP_OK on success
 */
extern int16_t SmallRbEnqueue(volatile SmallRingBuffer *ringBuffer, uint8_t data);

/**
 * Retrieves the number of bytes in a ring buffer.
 *
 * @param[in] ringBuffer    points to the buffer to query
 * @return                  the number of bytes stored in the buffer
 */
extern uint16_t SmallRbGetFillLength(volatile const SmallRingBuffer *ringBuffer);

#endif /* NUVC_RINGBUFFER_H_ */
