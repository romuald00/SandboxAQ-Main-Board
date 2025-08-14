/*
 * largeBuffer.h
 *
 *  Copyright Nuvation Research Corporation 2018-2025. All Rights Reserved.
 *
 *  Created on: Jan 28, 2025
 *      Author: rlegault
 */

#ifndef LIB_INC_LARGEBUFFER_H_
#define LIB_INC_LARGEBUFFER_H_

#include "cmsis_os.h"

#define LARGE_BUFFER_SIZE (20 * 1024 - 128)

/**
 * @fn largeBufferInit
 *
 * @brief create mutex semaphore
 *
 **/
void largeBufferInit(void);

/**
 * @fn largeBufferLock
 *
 * @brief get large buffer mutex semaphore
 *
 * @param[in] timeout_ms: how long to wait for buffer to become available
 *
 * @return osStatus: osOK if semaphore taken
 **/
osStatus largeBufferLock(uint32_t timeout_ms);

/**
 * @fn largeBufferUnlock
 *
 * @brief release large buffer mutex semaphore
 *
 **/
void largeBufferUnlock(void);

/**
 * @fn largeBufferSize_Bytes
 *
 * @brief get size of large buffer size
 *
 * @return size in bytes of large buffer
 **/
uint32_t largeBufferSize_Bytes(void);

/**
 * @fn largeBufferGet
 *
 * @brief get pointer to start of large buffer
 *
 * @return pointer to start of large buffer
 **/
uint8_t *largeBufferGet(void);

/**
 * @fn usableLargeBufferLength
 *
 * @brief get the length of string within large buffer
 *
 * @return string length within buffer
 **/
uint32_t usableLargeBufferLength(void);

/**
 * @fn largeBufferReadFrom
 *
 * @brief Start reading large buffer from the offset
 *
 * @param offset - offset into large buffer to get pointer for
 *
 * @return pointer to start of large buffer from the offset.
 **/
char *largeBufferReadFrom(uint32_t offset);

/**
 * @fn usableRamLogLargeBufferLength
 *
 * @brief get the length of string within large buffer
 *
 * @note that it removes the size of the ram log init header information
 *
 * @return string length within buffer
 **/
uint32_t usableRamLogLargeBufferLength(void);

/**
 * @fn largeRamLogBufferReadFrom
 *
 * @brief Start reading large buffer from the offset
 *
 * @note that the logbuffer first 16 bytes are log init information
 *       that is skipped over when doing the calculation.
 *
 * @param offset - offset into large buffer to get pointer for
 *
 * @return pointer to start of large buffer from the offset.
 **/
char *largeRamLogBufferReadFrom(uint32_t offset);
#endif /* LIB_INC_LARGEBUFFER_H_ */
