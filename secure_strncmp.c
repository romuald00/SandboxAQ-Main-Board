/**
 * @file
 * This file implements library for secure string comparison.
 *
 * Copyright Nuvation Research Corporation 2018-2019. All Rights Reserved.
 * www.nuvation.com
 */

#include "secure_strncmp.h"
#include "stddef.h"
#include "stdint.h"

// For this to be constant-time, pDecoy cannot be optimized out
#pragma GCC push_options
#pragma GCC optimize("O0")
int32_t securestrncmp(const char *input, const char *reference, uint32_t size) {
    const unsigned char *pInput = (const unsigned char *)input;
    const unsigned char *pReference = (const unsigned char *)reference;
    const unsigned char *pDecoy = (const unsigned char *)reference;
    int32_t cmp = 0;

    if (input == NULL || reference == NULL) {
        return -1;
    }

    while (size) {
        cmp |= *pInput ^ *pReference;

        if (*pInput == '\0') {
            break;
        }

        if (*pReference != '\0') {
            pReference++;
        }
        if (*pReference == '\0') {
            pDecoy++;
        }

        pInput++;
        size--;
    }

    return cmp;
}
#pragma GCC pop_options
