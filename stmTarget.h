/**
 * @file
 * This file defines STM includes to target.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef __STM_TARGET_H__
#define __STM_TARGET_H__

#include "main.h"
#ifdef STM32H743xx
#include "stm32h7xx_hal.h"
#define hiwdg hiwdg1
// Using MDMA results in 168us packet handling vs memcpy with 84us
// In the current implementation of MDMA there appears to be a caching issue.
#elif defined(STM32F767xx)
#include "stm32f7xx_hal.h"
#define hiwdg hiwdg
#endif

// THREAD stack sizes
#define DEFAULT_TASK_STACK_SIZE 512
#define MONGOOSE_STACK_WORDS 1024
#define DBTRIGGER_STACK_WORDS 256
#define MBGATHER_STACK_WORDS 1024
#define DDSTRIGGER_STACK_WORDS 128
#define DB_COMM_STACK_WORDS 512
#define SPI_STACK_WORDS 320
#define TCPDATA_STACK_WORDS 256
#define TRIGGER_STACK_WORDS 256
#define DDSTRIG_STACK_WORDS 128
// COMMON TASK STACKS
#define CLI_STACK_WORDS 900
#define WATCHDOG_STACK_WORDS 512
#define CNC_STACK_WORDS 512
#define RESET_STACK_WORDS 256

extern RTC_HandleTypeDef hrtc;

// Main Board P6  : PIN0=3V3, dbg0 ... dbg7, 10=GND
#define DBG0_PORT DBG_TP_0_GPIO_Port
#define DBG0_PIN DBG_TP_0_Pin

#define DBG1_PORT DBG_TP_1_GPIO_Port
#define DBG1_PIN DBG_TP_1_Pin

#define DBG2_PORT DBG_TP_2_GPIO_Port
#define DBG2_PIN DBG_TP_2_Pin

#define DBG3_PORT DBG_TP_3_GPIO_Port
#define DBG3_PIN DBG_TP_3_Pin

#define DBG4_PORT DBG_TP_4_GPIO_Port
#define DBG4_PIN DBG_TP_4_Pin

#define DBG5_PORT DBG_TP_5_GPIO_Port
#define DBG5_PIN DBG_TP_5_Pin

#define DBG6_PORT DBG_TP_6_GPIO_Port
#define DBG6_PIN DBG_TP_6_Pin

#define DBG7_PORT DBG_TP_7_GPIO_Port
#define DBG7_PIN DBG_TP_7_Pin
#ifdef NUCLEO
#define CLI_UART huart3
#define CLI_UART_INSTANCE USART3
#else
#define CLI_UART huart1
#define CLI_UART_INSTANCE USART1
#endif
#define UART_DATA RDR

// Command and control message depth
#define CNC_MSG_POOL_DEPTH 56 // 2KB <- 64

#endif /* __STM_TARGET_H__ */
