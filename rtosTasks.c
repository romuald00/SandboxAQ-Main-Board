/*
 * rtosTasks.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 27, 2023
 *      Author: rlegault
 */

#define CREATE_STRING_LOOKUP_TABLE
#include "saqTarget.h"
#undef CREATE_STRING_LOOKUP_TABLE

#include "cli_task.h"
#include "cmdAndCtrl.h"
#include "cmsis_os.h"
#include "debugPrint.h"
#include "resetBoard.h"
#include "stmTarget.h"
#include "watchDog.h"

extern IWDG_HandleTypeDef hiwdg;

ADC_MODE_e g_adcModeSwitch = ADC_MODE_SINGLE;

__weak void initBoardTasks(void) {
    assert(false);
}

void initRtosTasks(void) {
    watchDogTaskInit(osPriorityHigh, WATCHDOG_STACK_WORDS);
    osDelay(INIT_DELAYS);
    cncTaskInit(osPriorityNormal, CNC_STACK_WORDS);
    osDelay(INIT_DELAYS);
    cliTaskInit(osPriorityLow, CLI_STACK_WORDS);
    osDelay(INIT_DELAYS);
    resetTaskInit(osPriorityHigh, RESET_STACK_WORDS);
    osDelay(INIT_DELAYS);
#if ENABLE_WDG
    HAL_IWDG_Refresh(&hiwdg);
#endif
    osDelay(100);
    initBoardTasks();
}
