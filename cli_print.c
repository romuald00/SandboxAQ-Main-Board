/**
 * @file
 * Command Line Interface Library
 *
 * Implementation of the portable command line interface's printing helper
 * functions using a generic printf implementation.
 *
 * Copyright Nuvation Research Corporation 2006-2018. All Rights Reserved.
 * www.nuvation.com
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "asserts.h"
#include "generic_printf.h"

#include "cli/cli.h"
#include "cli/cli_print.h"

const char returnCodeName[] = "returnCode";
const char bytesRead[] = "bytesRead";
const char bytesWritten[] = "bytesWritten";
const char bytesTransferred[] = "bytesTransferred";
const char outputBytes[] = "outputBytes";

static const char nameAndNumberFormat[] = "<%s=%" PRId16 ">\n";
static const char nameIndexAndNumberFormat[] = "<%s%d=%" PRId16 ">\n";
static const char nameAnd32BitNumberFormat[] = "<%s=%" PRId32 ">\n";
static const char nameIndexAnd32BitNumberFormat[] = "<%s%d=%" PRId32 ">\n";

/* Helper functions for printf */
static int PrintCharToCli(void *arg, int ch);
static int PrintStringToCli(void *arg, const char *string, int length);
static void CliVprintf(CLI *hCli, const char *format, va_list args);

void CliPrintf(CLI *hCli, const char *format, ...) {
    va_list args;
    AppPrintFuncs funcs = {
        .printChar = PrintCharToCli,
        .printString = PrintStringToCli,
    };

    va_start(args, format);
    AppPrint(&funcs, hCli, format, args);
    va_end(args);
}

void CliPrintKeyValuePairs(CLI *hCli, CliKeyValuePairs *pairs) {
    if (pairs != NULL) {
        CliKeyValuePairs *curPair = pairs;
        while (curPair->key != NULL) {
            CliPrintNameAndString(hCli, curPair->key, curPair->value);
            curPair++;
        }
    }
}

void CliPrintNameAndNumber(CLI *hCli, const char *name, int number) {
    CliPrintf(hCli, nameAndNumberFormat, name, number);
}

void CliPrintNameIndexAndNumber(CLI *hCli, const char *name, int index, int number) {
    CliPrintf(hCli, nameIndexAndNumberFormat, name, index, number);
}

void CliPrintNameAnd32BitNumber(CLI *hCli, const char *name, uint32_t number) {
    CliPrintf(hCli, nameAnd32BitNumberFormat, name, (uint32_t)number);
}

void CliPrintNameIndexAnd32BitNumber(CLI *hCli, const char *name, int index, uint32_t number) {
    CliPrintf(hCli, nameIndexAnd32BitNumberFormat, name, index, (uint32_t)number);
}

void CliPrintNameAndString(CLI *hCli, const char *name, const char *format, ...) {
    va_list args;
    va_start(args, format);
    CliPrintf(hCli, "<%s=\"", name);
    CliVprintf(hCli, format, args);
    CliPrintf(hCli, "\">\n");
    va_end(args);
}

void CliPrintSpecialName(CLI *hCli, const char *name) {
    CliPrintf(hCli, "<%s>\n", name);
}

/* Helper functions for printf */
static int PrintCharToCli(void *arg, int ch) {
    (void)arg;
    CliWriteChar((CLI *)arg, ch);

    return 1;
}

static int PrintStringToCli(void *arg, const char *string, int length) {
    (void)arg;
    ASSERT_TRUE(length == strlen(string));
    CliWriteString((CLI *)arg, string);

    return length;
}

static void CliVprintf(CLI *hCli, const char *format, va_list args) {
    AppPrintFuncs funcs = {
        .printChar = PrintCharToCli,
        .printString = PrintStringToCli,
    };

    AppPrint(&funcs, hCli, format, args);
}
