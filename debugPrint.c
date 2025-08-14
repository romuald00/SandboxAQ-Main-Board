/**
 * @file
 * This file implements the debug printing library.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#include "debugPrint.h"
#include "debugPrintSettings.h"
#include "generic_printf.h"
#include "stmTarget.h"
#include <string.h>

extern bool SWO_EN;

#if DPRINTF_RTOS == 1
// Only include cmsis_os if its running an OS
#include "cmsis_os.h"

osMutexId dprintfMutexHandle;
osMutexDef(dprintfMutex);

static int32_t debugPrintInit = 0;
// Initialize debug printing
void debugPrintingInit(void) {
    if (!debugPrintInit) {
        // Create a mutex so prints can be atomic
        dprintfMutexHandle = osMutexCreate(osMutex(dprintfMutex));
        debugPrintInit = 1;
    }
}

int32_t debugPrintingIsInit() {
    return debugPrintInit;
}

bool dprintfLock(int timeout) {
    return osMutexWait(dprintfMutexHandle, timeout) == osOK;
}

void dprintfUnlock(void) {
    osMutexRelease(dprintfMutexHandle);
}

#endif

#ifndef DPRINTF_USE_RTC
#define DPRINTF_USE_RTC 0
#endif

#if DPRINTF_USE_RTC == 1
extern RTC_HandleTypeDef htrc;
#include "realTimeClock.h"

#endif

#define PUT_CHAR_PROTO(x) ITM_SendChar(x)

// Prints the characters from printf over SWO
int _write(int file, char *ptr, int len) {
    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++) {
        PUT_CHAR_PROTO(*ptr++);
    }

    return len;
}

// A minimal function to print a string. To be used in critical situations
void minimalWrite(const char *ptr) {
    if (ptr != NULL) {
        while (*ptr != '\0') {
            PUT_CHAR_PROTO(*ptr++);
        }
    }
}

/* Helper functions for printf */
static int printCharToDebug(void *arg, int ch) {
    (void)arg;
    PUT_CHAR_PROTO(ch);
    return 1;
}

static int printStringToDebug(void *arg, const char *string, int length) {
    (void)arg;
    (void)length;
    minimalWrite(string);
    return length;
}

// A minimal function to print a string. To be used in critical situations
void safePrintVA(const char *str, va_list args) {
    AppPrintFuncs funcs = {
        .printChar = printCharToDebug,
        .printString = printStringToDebug,
    };

    AppPrint(&funcs, NULL, str, args);
}

// A minimal function to print a string. To be used in critical situations
void safePrint(const char *str, ...) {
    va_list args;
    va_start(args, str);
    safePrintVA(str, args);
    va_end(args);
}

#if DPRINTF_USE_RAMLOG == 1
static void printTime() {
#if DPRINTF_USE_RTC == 1
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    if (getTime(&time, &date) != HAL_OK) {
        ramLogWrite(SWO_EN, "--/-- --:--:-- %u\t", HAL_GetTick());
        return;
    }
    ramLogWrite(SWO_EN,
                "%02u/%02u %02u:%02u:%02u %u\t",
                date.Month,
                date.Date,
                time.Hours,
                time.Minutes,
                time.Seconds,
                HAL_GetTick());
#else
    ramLogWrite(SWO_EN, "%u\t", HAL_GetTick());
#endif
}
#endif

#if DPRINTF_RTOS == 1

#if DEBUG_PRINT_TASK_NAME == 1
const char *getcurrentTaskName();
#endif

void dprintfType(const char *type, const char *str, ...) {
    va_list args;

    if (osMutexWait(dprintfMutexHandle, DEBUG_PRINT_WAIT) == osOK) {
#if DPRINTF_USE_RAMLOG == 1
        checkMissedLog();
        printTime();
        ramLogWrite(SWO_EN, "%8s: ", type);
#if DEBUG_PRINT_TASK_NAME == 1
        ramLogWrite(SWO_EN, "%16s: ", getcurrentTaskName());
#endif
        va_start(args, str);
        ramLogWriteVA(SWO_EN, str, args);
        va_end(args);
#else
        printf("%8s: ", type);
#if DEBUG_PRINT_TASK_NAME == 1
        printf(SWO_EN, "%16s: ", getcurrentTaskName());
#endif
        va_start(args, str);
        printf(str, args);
        va_end(args);
#endif
        osMutexRelease(dprintfMutexHandle);
    }
}

void dprintfRaw(const char *str, ...) {
    va_list args;

    if (osMutexWait(dprintfMutexHandle, DEBUG_PRINT_WAIT) == osOK) {
#if DPRINTF_USE_RAMLOG == 1
        checkMissedLog();
        va_start(args, str);
        ramLogWriteVA(1, str, args);
        va_end(args);
#else
        va_start(args, str);
        printf(str, args);
        va_end(args);
#endif
        osMutexRelease(dprintfMutexHandle);
    }
}

void dprintfRawVA(const char *str, va_list args) {
    if (osMutexWait(dprintfMutexHandle, DEBUG_PRINT_WAIT) == osOK) {
#if DPRINTF_USE_RAMLOG == 1
        checkMissedLog();
        ramLogWriteVA(1, str, args);
#else
        printf(str, args);
#endif
        osMutexRelease(dprintfMutexHandle);
    }
}

void dprintfNolock(const char *str) {
    if (osMutexWait(dprintfMutexHandle, 0) == osOK) {
#if DPRINTF_USE_RAMLOG == 1
        ramLogWrite(1, str);
#else
        minimalWrite(str);
#endif
        osMutexRelease(dprintfMutexHandle);
    } else {
        minimalWrite(str);
#if DPRINTF_USE_RAMLOG == 1
        recordMissedLog();
#endif
    }
}

void dprintfNolockArg(const char *str, ...) {
    va_list args;

    if (osMutexWait(dprintfMutexHandle, 0) == osOK) {
#if DPRINTF_USE_RAMLOG == 1
        va_start(args, str);
        ramLogWriteVA(SWO_EN, str, args);
        va_end(args);
#else
        va_start(args, str);
        safePrintVA(str, args);
        va_end(args);
#endif
        osMutexRelease(dprintfMutexHandle);
    } else {
        va_start(args, str);
        safePrintVA(str, args);
        va_end(args);
#if DPRINTF_USE_RAMLOG == 1
        recordMissedLog();
#endif
    }
}

#endif
