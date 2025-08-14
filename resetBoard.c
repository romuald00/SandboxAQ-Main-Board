/*
 * resetBoard.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 20, 2023
 *      Author: jzhang
 */

#include "resetBoard.h"
#include "cmdAndCtrl.h"
#include "debugPrint.h"
#include "registerParams.h"
#include "taskWatchdog.h"

osThreadId resetTaskHandle;
bool resetFlag = false;
uint32_t resetDelay = 0;

#define RESTART_NOTIFY_TIMEOUT_MS 200

RETURN_CODE updateResetFlag(registerInfo_tp info) {
    registerWriteForce(info);
    resetFlag = info->u.dataUint != 0 ? true : false;
    xTaskNotifyGive(resetTaskHandle);
    return RETURN_OK;
}

RETURN_CODE updateResetDelay(registerInfo_tp info) {
    registerWriteForce(info);
    resetDelay = info->u.dataUint;
    return RETURN_OK;
}

void resetTaskInit(int priority, int stackSize) {
    osThreadDef(resetTask, startResetTask, priority, 0, stackSize);
    resetTaskHandle = osThreadCreate(osThread(resetTask), NULL);
    assert(resetTaskHandle != NULL);
}

void startResetTask() {
    DPRINTF_INFO("Starting reset task\n\r");
    watchdogAssignToCurrentTask(WDT_TASK_RESET);
    watchdogSetTaskEnabled(WDT_TASK_RESET, 1);

    while (1) {
        uint32_t notify = ulTaskNotifyTake(true, RESTART_NOTIFY_TIMEOUT_MS);
        if (notify == TASK_NOTIFY_OK && resetFlag != 0) {
            DPRINTF_INFO("Changing WDT period to %d\n\r", resetDelay);
            watchdogChangeTaskPeriod(WDT_TASK_RESET, resetDelay);
        }
        // when resetFlag becomes non-zero, stop kicking watchdog
        if (resetFlag == 0) {
            watchdogKickFromTask(WDT_TASK_RESET);
        }
    }
}
