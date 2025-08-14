/*
 * cli_uart.c
 *  This file implements the CLI-UART interfacing functionality.
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 26, 2023
 *      Author: jzhang
 */

#include "cli/cli_uart.h"
#include "cli/cli_print.h"
#include "cli_task.h"
#include "cmsis_os.h"
#include "macros.h"
#include "ringbuffer.h"
#include "stmTarget.h"
#ifdef STM32F411xE
#include "imuComms.h"
#endif
#include <string.h>

UART_HandleTypeDef *channels[1] = {&CLI_UART};

extern osMessageQId cliQueueId;
extern CLI hCli;

#ifdef STM32F411xE
#include "imuComms.h"
extern imu_t hImu[IMU_PER_BOARD];
#endif

int CliWriteOnUart(int channel, const void *data, size_t size) {
    int retval = -1;
    uint8_t buff[MIN_CLI_BUFFER_SIZE];
    size_t remaining = size;
    size_t length;

    do {
        length = remaining < MIN_CLI_BUFFER_SIZE ? remaining : MIN_CLI_BUFFER_SIZE;

        memcpy(buff, data + size - remaining, length);

        retval = HAL_UART_Transmit(channels[channel], buff, length, 0xffff);

        if (retval != HAL_OK) {
            return -1;
        }

        remaining -= length;
    } while (remaining > 0);

    return retval;
}

int CliWriteByteOnUart(int channel, uint_least8_t data) {
    int retval = -1;

    retval = HAL_UART_Transmit(channels[channel], &data, 1, 0xffff);

    if (retval == HAL_OK) {
        return 1;
    }

    return retval;
}

#ifdef STM32F411xE
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    uint8_t rbyte;
    rbyte = (uint8_t)(huart->Instance->UART_DATA & (uint8_t)0xff);
    if (g_boardType == BOARDTYPE_IMU_COIL) {
        if (huart->Instance == IMU0_UART_INSTANCE) {
            imuCommsCharacterReceivedCallback(&hImu[IMU0_IDX], rbyte);
            return;
        } else if (huart->Instance == IMU1_UART_INSTANCE) {
            if (IMU_PER_BOARD > IMU1_IDX) {
                imuCommsCharacterReceivedCallback(&hImu[IMU1_IDX], rbyte);
            }
            return;
        }
    }
    // Use the returns above to quickly exit this ISR.
    // The CLI uart is the least active so check it last.
    if (huart->Instance == CLI_UART_INSTANCE) {
        if (CLI_CHECK_BOOL(&hCli, COMMAND_IS_EXECUTING)) {
            CliCharacterReceivedCallback(&hCli, rbyte);
            return;
        }
        // Put the data in the queue
        osMessagePut(cliQueueId, rbyte, 0);
    }
}
#else
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    uint8_t rbyte;

    if (huart->Instance == CLI_UART_INSTANCE) {
        rbyte = (uint8_t)(huart->Instance->UART_DATA & (uint8_t)0xff);
        if (CLI_CHECK_BOOL(&hCli, COMMAND_IS_EXECUTING)) {
            CliCharacterReceivedCallback(&hCli, rbyte);
            return;
        }
        // Put the data in the queue
        osMessagePut(cliQueueId, rbyte, 0);
    }
}
#endif
