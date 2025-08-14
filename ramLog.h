/**
 * @file
 *  This file defines the RAM-logger library.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef __RAM_LOG_H__
#define __RAM_LOG_H__

#include "cli/cli_print.h"
#include "saqTarget.h"
#include "stdint.h"
#include <stdarg.h>

#define SENSOR_BOARD_RAM_LOG_SZ_BYTES (20 * 1024)

typedef enum RamLogHeader {
    RAM_BUF_PTR = 0,
    RAM_BUF_SHADOW_PTR = 4,
    RAM_BUF_LEN = 8,
    RAM_BUF_LEN_SHADOW_PTR = 12,
    RAM_LOG_HEADER_SIZE = 16 // MUST BE LAST
} RamLogHeader_e;

extern char ramLogBuff[];

/**
 * @fn
 *
 * Initialize the ram log that is not reset on boot
 *
 */
void ramLogInit(void);

/**
 * @fn
 *
 * Write the printf style message to the ram log
 *
 * @param[in]    printToSWO  identify if the string should be
 *               printed out the SWO debug channel
 *
 * @param[in]    str printf formated string
 *
 * @param[in]    ... string variable args
 */
void ramLogWrite(int printToSWO, const char *str, ...);

/**
 * @fn
 *
 * Write the printf style message to the ram log
 *
 * @note         the caller to this function must hold the dprintfMutex
 *
 * @param[in]    printToSWO  identify if the string should be
 *               printed out the SWO debug channel
 *
 * @param[in]    str printf formated string
 *
 * @param[in]    args string variable args
 *
 */
void ramLogWriteVA(int printToSWO, const char *str, va_list args);

/**
 * @fn
 *
 * Get pointer to start of ramlog messages
 *
 * @note         the caller to this function must hold the dprintfMutex
 *
 * @return       string to read
 *
 */
char *ramLogRead(void);

/**
 * @fn
 *
 * Get next string message within ramlog at offset
 *
 * @note         the caller to this function must hold the dprintfMutex
 *
 * @param[in]    offset return pointer at offset in ram log buffer
 *
 * @return       string to read
 */
char *ramLogReadFrom(uint32_t offset);

/**
 * @fn
 *
 * Return the size of buffer that is printable
 *
 *
 * @return number of bytes that can be printed
 *
 */
uint32_t usableRamLogLength(void);

/**
 * @fn
 *
 * @param[out]   length Number of bytes that can be printed
 *
 * @param[out]   offset pointer to start of ram log.
 *
 * @param[in]    args string variable args
 *
 * @return 0=successful, else -1 if it failed
 *
 */
int ramLogStats(int *length, int *offset);

/**
 * @fn
 *
 * initialize the internal ram log pointers
 *
 */
void ramLogSetupEmpty(void);

/**
 * @fn
 *
 * Empty the ram log.
 *
 */
int ramLogEmpty(void);

/**
 * @fn
 *
 * Write out on SWO that logs were missed
 *
 * @note         the caller to this function must hold the dprintfMutex
 *
 */
void checkMissedLog(void);

/**
 * @fn
 *
 * Raise the flag that identifies that logs were missed
 *
 */
void recordMissedLog(void);

/**
 * @fn
 *
 * Prevent ram logs from being updated
 *
 *
 */
void blockRamLogUpdates(void);

/**
 * @fn
 *
 * Unblock Ram log updates
 *
 */
void unblockRamLogUpdates(void);

/**
 * @fn
 *
 * Translate bufferSizeId to a byte value and return the value
 *
 * @param bufferSizeId  identifies the spi transfer size of the ramlog
 *
 * @return amount of bytes to Transmit
 */
uint32_t sensorRamLogTxSize_Bytes(CNC_ACTION_e bufferSizeId);

/**
 * @fn
 *
 * Write the memory starting at pointer to hCli for count values
 *
 * @notes it prints both hex and ascii
 *
 * @param hCli, pointer to the CLI uart
 *
 * @param ptr  memory location to start reading from
 *
 * @param count number of memory bytes to read
 */
void dumpMemory(CLI *hCli, uint8_t *ptr, uint32_t count);

#endif /* __RAM_LOG_H__ */
