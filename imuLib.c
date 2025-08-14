/*
 * imuLib.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Aug 17, 2024
 *      Author: rlegault
 */

#include "debugPrint.h"
#include "saqTarget.h"
#include <stdint.h>

#define PRINT_8_BYTES 8
static char printBuffer[MAX_PRINT_BUFFER_SZ];
#define CRLF "\r\n"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
void displaySentBinaryData(uint8_t *rxBuffer, BINARY_DATA_FIELD_e dataType) {
    if (!DEBUG_IMU_VERBOSE_PRINTING) {
        return;
    }
    for (int i = 0; i < sizeof(quaternionData_t); i += PRINT_8_BYTES) {
        DPRINTF_RAW("%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                    rxBuffer[i],
                    rxBuffer[i + 1],
                    rxBuffer[i + 2],
                    rxBuffer[i + 3],
                    rxBuffer[i + 4],
                    rxBuffer[i + 5],
                    rxBuffer[i + 6],
                    rxBuffer[i + 7]);
    }

    if (dataType == DATA_TYPE_EULER_2NDBYTE) {
        eulerData_tp euler = (eulerData_tp)rxBuffer;
        float froll, fpitch, fheading;

        u32BE_to_floatLE(froll, euler->alphaRoll);
        u32BE_to_floatLE(fpitch, euler->alphaPitch);
        u32BE_to_floatLE(fheading, euler->alphaHeading);

        snprintf(printBuffer,
                 MAX_PRINT_BUFFER_SZ,
                 "roll=%.02f (%lx) pitch=%.02f heading=%.02f \r\n",
                 froll,
                 euler->alphaRoll,
                 fpitch,
                 fheading);
        DPRINTF_RAW(printBuffer);

        for (int i = 0; i < CARTESSIAN_COOR; i++) {
            float facc;
            u32BE_to_floatLE(facc, euler->accel[i]);
            snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "ACCEL_%c = %.02f ", i + 'x', facc);
            DPRINTF_RAW(printBuffer);
        }
        DPRINTF_RAW(CRLF);
        for (int i = 0; i < CARTESSIAN_COOR; i++) {
            float fgyro;
            u32BE_to_floatLE(fgyro, euler->gyro[i]);
            snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "GYRO_%c = %.02f (%lx)", i + 'x', fgyro, euler->gyro[i]);
            DPRINTF_RAW(printBuffer);
        }
        DPRINTF_RAW(CRLF);
        for (int i = 0; i < CARTESSIAN_COOR; i++) {
            uint16_t mag = htob16(euler->magnet[i]);
            snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "MAG_%c = %.02f (%x) ", i + 'x', MAGNET_VALUE(mag), mag);
            DPRINTF_RAW(printBuffer);
        }
        DPRINTF_RAW(CRLF);
        uint16_t temp = htob16(euler->temperature);
        snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "TEMPER = %.02f (%x)\r\n", (TEMPERATURE_VALUE(temp)), temp);
        DPRINTF_RAW(printBuffer);
    } else {
        quaternionData_tp quat = (quaternionData_tp)rxBuffer;
        for (int i = 0; i < QUATERNION_COOR; i++) {
            float fquat;
            u32BE_to_floatLE(fquat, quat->quat[i]);
            DPRINTF_RAW("Q%d = %.02f ", i, fquat);
        }
        DPRINTF_RAW(CRLF);

        for (int i = 0; i < CARTESSIAN_COOR; i++) {
            float facc;
            u32BE_to_floatLE(facc, quat->accel[i]);
            DPRINTF_RAW("ACCEL_%c = %.02f ", i + 'x', facc);
        }
        DPRINTF_RAW(CRLF);

        for (int i = 0; i < CARTESSIAN_COOR; i++) {
            float fgyro;
            u32BE_to_floatLE(fgyro, quat->gyro[i]);
            DPRINTF_RAW("GYRO_%c = %.02f ", i + 'x', fgyro);
        }
        DPRINTF_RAW(CRLF);
        for (int i = 0; i < CARTESSIAN_COOR; i++) {
            uint16_t mag = htob16(quat->magnet[i]);
            snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "MAG_%c = %.02f (%x) ", i + 'x', MAGNET_VALUE(mag), mag);
            DPRINTF_RAW(printBuffer);
        }
        DPRINTF_RAW(CRLF);

        uint16_t temp = htob16(quat->temperature);
        DPRINTF_RAW("TEMPER = %.02f (%x)\r\n", TEMPERATURE_VALUE(temp), temp);
    }
}
#pragma GCC diagnostic pop
