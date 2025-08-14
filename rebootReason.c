/*
 * rebootreason.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Sep 2, 2024
 *      Author: rlegault
 */
#define CREATE_REBOOT_REASON_STRING_LOOKUP_TABLE
#include "rebootReason.h"
#undef CREATE_REBOOT_REASON_STRING_LOOKUP_TABLE
#include "debugPrintSettings.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

extern char _REBOOT_LOG_SIZE;
static int32_t __REBOOT_LOG_SIZE = (int32_t)&_REBOOT_LOG_SIZE;

#define REBOOT_UID 0xAA55AA55

typedef struct {
    uint32_t uid;
    uint32_t intReason;
    char string[REBOOT_LOG_SIZE_BYTES - 2 * sizeof(uint32_t)];
} rebootReason_t, *rebootReason_tp;

__attribute__((section(".rebootreason"))) rebootReason_t rebootReason;

void rebootReasonClear(void) {
    assert(__REBOOT_LOG_SIZE == sizeof(rebootReason_t));
    rebootReason.uid = ~REBOOT_UID;
    rebootReason.intReason = REBOOT_UNKNOWN;
}

void rebootReasonSet(REBOOT_REASON_e reason, const char *msg) {
    if (rebootReason.uid != REBOOT_UID) {
        rebootReason.intReason = reason;
        if (msg != NULL) {
            memcpy(rebootReason.string, msg, sizeof(rebootReason.string));
        } else {
            rebootReason.string[0] = '\0';
        }
        rebootReason.uid = REBOOT_UID;
    }
}
void rebootReasonSetPrintf(REBOOT_REASON_e reason, const char *msg, ...) {
    va_list args;
    if (rebootReason.uid != REBOOT_UID) {
        rebootReason.intReason = reason;
        if (msg != NULL) {
            va_start(args, msg);
            vsnprintf(rebootReason.string, sizeof(rebootReason.string), msg, args);
            va_end(args);
        }
        rebootReason.uid = REBOOT_UID;
    }
}
bool rebootReasonGet(REBOOT_REASON_e *reason, const char **msg) {
    if (rebootReason.uid == REBOOT_UID) {
        *reason = rebootReason.intReason;
        *msg = rebootReason.string;
        return true;
    }
    return false;
}
