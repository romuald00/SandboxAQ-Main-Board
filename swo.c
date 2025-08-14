/**
 * @file
 * This file defines the debug print fuunctionality.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#include "swo.h"
#include "stdio.h"
#include "stmTarget.h"
extern UART_HandleTypeDef huart3;
PUTCHAR_PROTOTYPE {
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
    ITM_SendChar(ch);
    return ch;
}
