/**
 * @file
 * Portable sprintf implementation.
 *
 * Copyright Nuvation Research Corporation 2006-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifdef USE_NUVC_SNPRINTF

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "generic_printf.h"

#define MAX_SNPRINTF_SIZE 255

typedef struct {
    char *buffer;
    uint8_t size;
    uint8_t index;
} SprintfMemory;

/* Helper functions for sprintf. */
static int PrintCharToBuffer(void *arg, int ch) {
    SprintfMemory *info = (SprintfMemory *)arg;

    if (info->index >= info->size)
        return 0;

    info->buffer[info->index] = (char)ch;
    info->index++;
    return 1;
}

static int PrintStringToBuffer(void *arg, const char *string, int length) {
    SprintfMemory *info = (SprintfMemory *)arg;

    if (length > (info->size - info->index))
        length = info->size - info->index;

    strncpy(&info->buffer[info->index], string, length);
    info->index += length;

    return length;
}

/* Public functions. */
int snprintf(char *buffer, size_t size, const char *format, ...) {
    va_list args;
    AppPrintFuncs funcs;
    SprintfMemory info;

    if (size > MAX_SNPRINTF_SIZE)
        return -1;

    funcs.printChar = PrintCharToBuffer;
    funcs.printString = PrintStringToBuffer;

    info.buffer = buffer;
    info.size = size;
    info.index = 0;

    va_start(args, format);
    int length = AppPrint(&funcs, &info, format, args);
    va_end(args);

    buffer[length] = '\0';

    return length;
}
#endif /* USE_NUVC_SNPRINTF */
