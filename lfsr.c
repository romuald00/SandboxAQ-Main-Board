/**
 * @file
 * API implementation for a 16 bit galois LFSR.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lfsr.h"

uint16_t Lfsr16(uint8_t *data, size_t size, uint16_t lfsr) {
    uint16_t i;

    if (data == NULL) {
        return lfsr;
    }

    if (lfsr == 0) {
        lfsr = UINT16_C(0xACE1);
    }

    for (i = 0; i < size; i += sizeof(uint16_t)) {
        data[i] = (uint8_t)lfsr;

        /* Account for the possibility of odd buffer sizes */
        if (size >= i + sizeof(uint16_t)) {
            data[i + 1] = (uint8_t)(lfsr >> 8);
        }

        lfsr = (lfsr >> 1) ^ (-(lfsr & UINT16_C(1)) & UINT16_C(0xB400));
    }

    return lfsr;
}
