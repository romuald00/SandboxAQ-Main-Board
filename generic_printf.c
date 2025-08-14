/**
 * @file
 * This file provides a generic function for the printf family.
 *
 * Copyright Nuvation Research Corporation 2006-2017. All Rights Reserved.
 * www.nuvation.com
 */

#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "generic_printf.h"

/** Flag to indicate CLI_print function should pad to the right. */
#define PRINTF_PAD_RIGHT 0x01

/** Flag to indicate the CLI_printi function should pad with zeros. */
#define PRINTF_PAD_ZERO 0x02

/** Default buffer size for expressing a 32 bit integer number as a string. */
#define PRINTF_PRINTI_BUF_LEN 12

/* Prototypes */
static int PrintInteger(AppPrintFuncs *funcs,
                        void *arg,
                        int32_t value,
                        unsigned int base,
                        int sign,
                        unsigned int width,
                        int pad,
                        int letterBase);
static int PrintString(AppPrintFuncs *funcs, void *arg, const char *string, unsigned int width, int pad);

int AppPrint(AppPrintFuncs *funcs, void *arg, const char *format, va_list args) {
    unsigned int width;
    int pad;
    char scr[2];
    int length = 0;

    while (*format) {
        if (*format == '%') {
            format++;
            width = pad = 0;

            if (*format == '\0')
                break;

            if (*format == '%') {
                length += funcs->printChar(arg, *format);
            } else {
                if (*format == '-') {
                    format++;
                    pad = PRINTF_PAD_RIGHT;
                }

                while (*format == '0') {
                    format++;
                    pad |= PRINTF_PAD_ZERO;
                }

                while (*format >= '0' && *format <= '9') {
                    width = width * 10 + *format++ - '0';
                }

                if (*format == 's') {
                    char *s = (char *)va_arg(args, char *);
                    length += PrintString(funcs, arg, s ? s : "(null)", width, pad);
                } else if (*format == 'd') {
                    length += PrintInteger(funcs, arg, (int32_t)va_arg(args, int), 10, 1, width, pad, 'a');
                } else if (*format == 'x') {
                    length += PrintInteger(funcs, arg, (int32_t)va_arg(args, int), 16, 0, width, pad, 'a');
                } else if (*format == 'X') {
                    length += PrintInteger(funcs, arg, (int32_t)va_arg(args, int), 16, 0, width, pad, 'A');
                } else if (*format == 'y') {
                    length += PrintInteger(funcs, arg, (int32_t)va_arg(args, int32_t), 16, 0, width, pad, 'a');
                } else if (*format == 'Y') {
                    length += PrintInteger(funcs, arg, (int32_t)va_arg(args, int32_t), 16, 0, width, pad, 'A');
                } else if (*format == 'u') {
                    length += PrintInteger(funcs, arg, (int32_t)va_arg(args, int), 10, 0, width, pad, 'a');
                } else if (*format == 'l') {
                    if (*(format + 1) == 'd') {
                        length += PrintInteger(funcs, arg, (int32_t)va_arg(args, int32_t), 10, 1, width, pad, 'a');
                        format++;
                    } else if (*(format + 1) == 'u') {
                        length += PrintInteger(funcs, arg, (int32_t)va_arg(args, uint32_t), 10, 0, width, pad, 'a');
                        format++;
                    } else {
                        /* Legacy support for when nuvc used non-standard format specifier */
                        length += PrintInteger(funcs, arg, (int32_t)va_arg(args, uint32_t), 10, 0, width, pad, 'a');
                    }
                } else if (*format == 'c') {
                    /* Chars are converted to int then pushed onto the stack...
                     */
                    scr[0] = (char)va_arg(args, int);
                    scr[1] = '\0';
                    length += PrintString(funcs, arg, scr, width, pad);
                }
            }
        } else {
            length += funcs->printChar(arg, *format);
        }

        format++;
    }

    return length;
}

static int PrintInteger(AppPrintFuncs *funcs,
                        void *arg,
                        int32_t value,
                        unsigned int base,
                        int sign,
                        unsigned int width,
                        int pad,
                        int letterBase) {
    char printBuffer[PRINTF_PRINTI_BUF_LEN];
    char *ptr;
    uint32_t digitValue;
    bool negated;
    uint32_t unsignedValue;
    int length = 0;

    if (value == 0) {
        ptr = "0";
    } else {
        if (sign && value < 0 && base == 10) {
            negated = true;
            unsignedValue = (uint32_t)-value;
        } else {
            negated = false;
            unsignedValue = (uint32_t)value;
        }

        ptr = printBuffer + PRINTF_PRINTI_BUF_LEN - 1;
        *ptr = '\0';

        while (unsignedValue) {
            digitValue = unsignedValue % base;
            if (digitValue >= 10)
                digitValue += (uint32_t)letterBase - '0' - 10;
            *--ptr = (char)('0' + digitValue);
            unsignedValue /= base;
        }

        if (negated) {
            if (width && (pad & PRINTF_PAD_ZERO)) {
                length += funcs->printChar(arg, '-');
                width--;
            } else {
                *--ptr = '-';
            }
        }
    }

    return length + PrintString(funcs, arg, ptr, width, pad);
}

static int PrintString(AppPrintFuncs *funcs, void *arg, const char *string, unsigned int width, int pad) {
    int padchar = ' ';
    unsigned int len = strlen(string);
    int result = 0;

    if (width > 0) {
        if (len >= width)
            width = 0;
        else
            width -= len;
        if (pad & PRINTF_PAD_ZERO)
            padchar = '0';
    }

    if (!(pad & PRINTF_PAD_RIGHT))
        for (; width > 0; width--)
            result += funcs->printChar(arg, padchar);

    result += funcs->printString(arg, string, len);

    for (; width > 0; width--)
        result += funcs->printChar(arg, padchar);

    return result;
}
