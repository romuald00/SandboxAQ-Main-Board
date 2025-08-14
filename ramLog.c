/**
 * @file
 * This file implements the RAM-logger library.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#include "ramLog.h"
#include "asserts.h"
#include "debugPrint.h"
#include "debugPrintSettings.h"
#include "macros.h"
#include "saqTarget.h"
#include "stmTarget.h"
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define CHARACTER_PER_ROW 16
#define SIZE_1K 1024

extern CRC_HandleTypeDef hcrc;

#if DPRINTF_USE_RAMLOG == 1
extern char _RAM_LOG_SIZE;
extern UART_HandleTypeDef RAM_LOG_USART;
static int32_t __RAM_LOG_SIZE = (int32_t)&_RAM_LOG_SIZE;
__attribute__((section(".ramlog"))) char ramLogBuff[RAM_LOG_SIZE_BYTES];

static uint32_t *ramBufPtr = (uint32_t *)&ramLogBuff[RAM_BUF_PTR];
static uint32_t *ramBufShadowPtr = (uint32_t *)&ramLogBuff[RAM_BUF_SHADOW_PTR];
static uint32_t *ramBufLen = (uint32_t *)&ramLogBuff[RAM_BUF_LEN];
static uint32_t *ramBufLenShadowPtr = (uint32_t *)&ramLogBuff[RAM_BUF_LEN_SHADOW_PTR];

#ifdef STM32H743xx
extern char _LARGE_BUFFER_SIZE;
static int32_t __LARGE_BUFFER_SIZE = (int32_t)&_LARGE_BUFFER_SIZE;
extern char largeBuffer[];
#endif

static volatile int missedLogFlag = 0;
static int ramLogInitialized = 0;

#if USE_RAM_LOG_USART == 1
static uint8_t *dmaReadPtr = (uint8_t *)&ramLogBuff[RAM_LOG_HEADER_SIZE];
static bool dmaRunning = false;
bool g_disableRamLog = true; // assume true until proven this is not a coil driver board
extern UART_HandleTypeDef RAM_LOG_USART;

void imuDmaComplete(UART_HandleTypeDef *huart);

static bool ramlogBlock = false;

void blockRamLogUpdates(void) {
    ramlogBlock = true;
}

void unblockRamLogUpdates(void) {
    ramlogBlock = false;
}



static const uint32_t bufferSzTable[CNC_ACTION_MAX] = {
        [CNC_ACTION_READ_SMALL_BUFFER] = 1*SIZE_1K,
        [CNC_ACTION_READ_MED_BUFFER] = 4*SIZE_1K,
        [CNC_ACTION_READ_LARGE_BUFFER] = SENSORLOG_RAM_SIZE_BYTES};

uint32_t sensorRamLogTxSize_Bytes(CNC_ACTION_e bufferSize) {
    assert(bufferSize < CNC_ACTION_MAX);

    return bufferSzTable[bufferSize];
}

void startDma(void) {
    if (g_disableRamLog) {
        return;
    }
    uint8_t *dmaPtr = dmaReadPtr;
    uint8_t *ramBufAddr = (uint8_t *)&ramLogBuff[*ramBufPtr];
    uint16_t len = 0;
    if (dmaReadPtr < ramBufAddr) {
        len = ramBufAddr - dmaReadPtr;
        dmaReadPtr = ramBufAddr;
    } else {
        len = (uint8_t *)&ramLogBuff[RAM_LOG_SIZE_BYTES] - dmaReadPtr;
        dmaReadPtr = (uint8_t *)&ramLogBuff[RAM_LOG_HEADER_SIZE];
    }
    HAL_UART_Transmit_DMA(&RAM_LOG_USART, dmaPtr, len);
    dmaRunning = true;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if ((g_disableRamLog == false) && (huart == &RAM_LOG_USART)) {
        uint8_t *ramBufAddr = (uint8_t *)&ramLogBuff[*ramBufPtr];
        if (dmaReadPtr == ramBufAddr) {
            dmaRunning = false;
            return;
        } else {
            startDma();
        }
    } else {
#ifdef STM32F411xE
        imuDmaComplete(huart);
#endif
    }
}
#endif

#define NUM_RETRY 10
bool SWO_EN = false;

typedef enum { IOB_OK, IOB_ERR_BUS, IOB_ERR_BUSERR, IOB_ERR_ALERT, IOB_ERR_RANGE } IOB_ERR;

int debuggerPresent(void) {
    if ((CoreDebug->DHCSR & 1) != 1)
        return IOB_ERR_BUS; // C_DEBUGEN, debugger not attached?
    if ((CoreDebug->DEMCR & (1 << 24)) == 0)
        return IOB_ERR_BUSERR; // TRCENA, trace not enabled
    if ((ITM->TCR & 0x0001) == 0)
        return IOB_ERR_ALERT; // ITMENA, ITM not enabled
    // uint8_t port = addr & 0x1f; // extract port number 0-31
    // if( (ITM->TER & (0x01 << port)) == 0 ) return IOB_ERR_RANGE; // ITM port not enabled
    return IOB_OK;
}
void ramLogInit(void) {
#ifdef STM32F411xE
    assert(bufferSzTable[CNC_ACTION_READ_LARGE_BUFFER] == SENSORLOG_RAM_SIZE_BYTES);
    if (g_boardType != BOARDTYPE_IMU_COIL) {
        g_disableRamLog = false;
    }
    // Test that the RAM LOG SIZE fits within the LARGE_BUFFER_SIZE_BYTES
    assert(LARGE_BUFFER_SIZE_BYTES >= __RAM_LOG_SIZE);
#else
    assert(LARGE_BUFFER_SIZE_BYTES == __LARGE_BUFFER_SIZE);
    g_disableRamLog = false;
#endif

    assert(__RAM_LOG_SIZE == RAM_LOG_SIZE_BYTES);
    assert(ramLogInitialized == 0);
    if (debuggerPresent() == IOB_OK) {
        SWO_EN = true;
    }
    if (!dprintfLock(DEBUG_PRINT_WAIT)) {
        return;
    }
    if (*ramBufPtr < ARRAY_SIZE(ramLogBuff) && *ramBufShadowPtr == ~*ramBufPtr) {
        if (*ramBufLenShadowPtr == ~*ramBufLen) {
            uint32_t oldLen = *ramBufLen;
            uint32_t newLen = *ramBufLen;
            uint32_t termLen = 0;
            uint32_t numCorrected = 0;
            uint32_t correctedAddr[NUM_RETRY];

            while (numCorrected < NUM_RETRY) {
                termLen =
                    strlen(&ramLogBuff[RAM_LOG_HEADER_SIZE]) + RAM_LOG_HEADER_SIZE; // start reading past the header

                if (termLen < *ramBufPtr) {
                    ramLogBuff[termLen] = 'X';
                    // Save this information because we cant safely print it yet
                    correctedAddr[numCorrected] = termLen;
                    numCorrected++;
                } else {
                    break;
                }
            }

            if (numCorrected == NUM_RETRY) {
                ramLogSetupEmpty();
                ramLogInitialized = 1;
                ramLogWrite(SWO_EN, "\r\n\r\n");
                ramLogWrite(SWO_EN, "RAM log data was corrupt\r\n");
                ramLogWrite(SWO_EN, "Starting new RAM log (%d bytes)...\r\n", *ramBufLen);
                dprintfUnlock();
                return;
            }

            if (*ramBufLen > RAM_LOG_SIZE_BYTES) {
                if (*ramBufPtr >= RAM_LOG_SIZE_BYTES - 1) {
                    *ramBufPtr = RAM_LOG_HEADER_SIZE;
                    *ramBufShadowPtr = ~*ramBufPtr;
                }
                *ramBufLen = RAM_LOG_SIZE_BYTES;
                *ramBufLenShadowPtr = ~RAM_LOG_SIZE_BYTES;
                ramLogBuff[RAM_LOG_SIZE_BYTES - 1] = '\0';
            } else if (*ramBufLen < RAM_LOG_SIZE_BYTES) {
                memset(&ramLogBuff[*ramBufPtr], 0, RAM_LOG_SIZE_BYTES - *ramBufLen);
                *ramBufLen = RAM_LOG_SIZE_BYTES;
                *ramBufLenShadowPtr = ~RAM_LOG_SIZE_BYTES;
            }

            newLen = *ramBufLen;
            ramLogInitialized = 1;
            ramLogWrite(SWO_EN, "\r\n\r\n");
            for (int i = 0; i < numCorrected; i++) {
                ramLogWrite(
                    SWO_EN, "Invalid terminator to log found at position %d. Repaired with X.\r\n", correctedAddr[i]);
            }
            ramLogWrite(SWO_EN, "RAM log old length: %d new length: %d bytes\r\n", oldLen, newLen);
            ramLogWrite(SWO_EN, "Resuming logging after reset...\r\n");
            dprintfUnlock();
            return;
        } else {
            ramLogSetupEmpty();
            uint32_t len = *ramBufLen;
            ramLogInitialized = 1;
            ramLogWrite(SWO_EN, "\r\n\r\n");
            ramLogWrite(SWO_EN, "RAM log header was corrupt\r\n");
            ramLogWrite(SWO_EN, "Starting new RAM log (%d bytes)...\r\n", len);
            dprintfUnlock();
            return;
        }
    } else {
        ramLogSetupEmpty();
        uint32_t len = *ramBufLen;
        ramLogInitialized = 1;
        ramLogWrite(SWO_EN, "\r\n\r\n");
        ramLogWrite(SWO_EN, "Starting new RAM log (%d bytes)...\r\n", len);
        dprintfUnlock();
        return;
    }
}

void ramLogWrite(int printToSWO, const char *str, ...) {
    va_list args;

    va_start(args, str);
    ramLogWriteVA(printToSWO, str, args);
    va_end(args);
}

// NOTE: the caller to this function must hold the dprintfLock
void ramLogWriteVA(int printToSWO, const char *str, va_list args) {
    // this check is needed if there is an interrupt between the debugPrint init and the ramLog init that tries to
    // print. Or if an interrupt with a print happens before initialization.
    if (!ramLogInitialized) {
        return;
    }
    uint32_t bufLeft = ARRAY_SIZE(ramLogBuff) - *ramBufPtr;
    int32_t written = 0;

    written = vsnprintf(&ramLogBuff[*ramBufPtr], bufLeft, str, args);

    if (written < 0) {
        return;
    }

    if (printToSWO && SWO_EN) {
        minimalWrite(&ramLogBuff[*ramBufPtr]);
    }
    if (!ramlogBlock) {
        if (written >= bufLeft) {
            *ramBufPtr = RAM_LOG_HEADER_SIZE;
            *ramBufShadowPtr = ~*ramBufPtr;
        } else if (written == bufLeft - 1) {
            *ramBufPtr += written;
            *ramBufShadowPtr = ~*ramBufPtr;
        } else {
            *ramBufPtr += written;
            ramLogBuff[*ramBufPtr] = ' ';
            *ramBufShadowPtr = ~*ramBufPtr;
        }
        if (!dmaRunning) {
            startDma();
        }
        // Ensure end of log is always zero for emergency print
        ramLogBuff[RAM_LOG_SIZE_BYTES - 1] = 0;
    }
}

char *ramLogRead(void) {
    return &ramLogBuff[RAM_LOG_HEADER_SIZE];
}

char *ramLogReadFrom(uint32_t offset) {
    if (RAM_LOG_HEADER_SIZE + offset < ARRAY_SIZE(ramLogBuff)) {
        return &ramLogBuff[RAM_LOG_HEADER_SIZE + offset];
    } else {
        return NULL;
    }
}

// NOTE: the caller to this function must hold the dprintfLock
void ramLogSetupEmpty(void) {
    memset(ramLogBuff, 0, ARRAY_SIZE(ramLogBuff));
    *ramBufPtr = RAM_LOG_HEADER_SIZE;
    *ramBufShadowPtr = ~*ramBufPtr;
    *ramBufLen = RAM_LOG_SIZE_BYTES - sizeof(uint32_t); // size of CRC check at end of buffer for SPI Tx
    *ramBufLenShadowPtr = ~*ramBufLen;
    dmaReadPtr = (uint8_t *)&ramLogBuff[*ramBufPtr];
}

int ramLogEmpty() {
    if (!dprintfLock(DEBUG_PRINT_WAIT)) {
        DPRINTF_NOLOCK("Error clearing RAM log\r\n");
        return -1;
    }
    if (dmaRunning) {
        HAL_UART_AbortTransmit(&RAM_LOG_USART);
        dmaRunning = false;
    }
    ramLogSetupEmpty();
    dprintfUnlock();

    DPRINTF_RAW("\r\n\r\n");
    DPRINTF_INFO("Cleared RAM log at software request\r\n");
    return 0;
}

void checkMissedLog(void) {
    if (missedLogFlag) {
        missedLogFlag = 0;
        ramLogWrite(0, "<Missed Logs - Check SWO>\r\n");
    }
}

void recordMissedLog(void) {
    missedLogFlag = 1;
}

uint32_t usableRamLogLength(void) {
    if (!dprintfLock(DEBUG_PRINT_WAIT)) {
        DPRINTF_NOLOCK("Error getting RAM log length\r\n");
        return 0;
    }
    uint32_t len = strnlen(&ramLogBuff[RAM_LOG_HEADER_SIZE], RAM_LOG_SIZE_BYTES - RAM_LOG_HEADER_SIZE);
    dprintfUnlock();
    return len;
}

int ramLogStats(int *length, int *offset) {
    if (length == NULL || offset == NULL) {
        return -1;
    }
    if (!dprintfLock(DEBUG_PRINT_WAIT)) {
        DPRINTF_NOLOCK("Error getting RAM log length\r\n");
        return -1;
    }
    *length = strnlen(&ramLogBuff[RAM_LOG_HEADER_SIZE], RAM_LOG_SIZE_BYTES - RAM_LOG_HEADER_SIZE);
    if (*length > 0) {
        *offset = (*ramBufPtr - RAM_LOG_HEADER_SIZE - 1) % *length;
    } else {
        *offset = 0;
    }
    dprintfUnlock();
    return 0;
}
#endif /* DPRINTF_USE_RAMLOG == 1 */

void dumpMemory(CLI *hCli, uint8_t *ptr, uint32_t count) {
    unsigned char buf[CHARACTER_PER_ROW];
    for (uint32_t i = 0; i < count; i++) {
        if ((i != 0) && ((i % CHARACTER_PER_ROW) == 0)) {
            // put some spacing between the hex table and the ascii table
            CliPrintf(hCli, "    ");
            for (int j = 0; j < CHARACTER_PER_ROW; j++) {
                if (isprint(buf[j])) {
                    CliPrintf(hCli, "%c", buf[j]);
                } else {
                    CliPrintf(hCli, ".");
                }
            }
            CliPrintf(hCli, "\r\n");
        }
        buf[i % CHARACTER_PER_ROW] = ptr[i];
        CliPrintf(hCli, "%02x ", ptr[i]);
    }
    // align the ascii character section
    for (int j = 0; j < (CHARACTER_PER_ROW - (count % CHARACTER_PER_ROW)); j++) {
        CliPrintf(hCli, "   "); // normally 2 characters and a space is used for each element
    }
    // put some spacing between the hex table and the ascii table
    CliPrintf(hCli, "    ");
    for (int j = 0; j < (count % CHARACTER_PER_ROW); j++) {
        if (isprint(buf[j])) {
            CliPrintf(hCli, "%c", buf[j]);
        } else {
            CliPrintf(hCli, ".");
        }
    }
    CliPrintf(hCli, "\r\n");
}
