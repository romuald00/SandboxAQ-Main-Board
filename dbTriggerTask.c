/*
 * dbTriggerTask.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 18, 2023
 *      Author: rlegault
 */

#include "dbTriggerTask.h"
#include "cli/cli_print.h"
#include "cmdAndCtrl.h"
#include "cmsis_os.h"
#include "dbCommTask.h"
#include "debugPrint.h"
#include "main.h"
#include "peripherals/MB_handlePwrCtrl.h"
#include "pwmPinConfig.h"
#include "taskWatchdog.h"
#include <stdlib.h>
#include <string.h>

void setDbTriggerInterval(uint32_t interval_us);

osThreadId dbTriggerTaskHandle;
void dbTriggerThread(const void *arg);

/* The event group used by all the task based tests. */

#define ALL_DB_EVENT_0 0x00FFFFFF
#define ALL_DB_EVENT_1 0x00FFFFFF

__DTCMRAM__ uint32_t dbTriggerEventGroupMask[MAX_DB_EVENT_GROUPS] = {0};
__DTCMRAM__ EventGroupHandle_t dbTriggerEventGroup[MAX_DB_EVENT_GROUPS] = {NULL};
/* The event group used by all the task based tests. */

__ITCMRAM__ inline void dbTriggerDisable(uint32_t dbId) {
    if (dbId >= MAX_CS_ID) {
        DPRINTF_ERROR("dbId %d out of range\r\n", dbId)
    }
    assert(dbId < MAX_CS_ID);
    dbTriggerEventGroupMask[GET_GROUP_EVT_IDX(dbId)] &= ~(GET_GROUP_EVT_ID(dbId));
}

__ITCMRAM__ inline void dbTriggerEnable(uint32_t dbId) {
    assert(dbId < MAX_CS_ID);
    dbTriggerEventGroupMask[GET_GROUP_EVT_IDX(dbId)] |= (GET_GROUP_EVT_ID(dbId));
}

__ITCMRAM__ inline void dbTriggerEnableAll(void) {
    if (!isPowerModeEn()) {
        // in low power mode, do nothing.
        return;
    }
    // just do half at each trigger
    static uint32_t counter = -1;
    // first time enable all
    if (counter == -1) {
        dbTriggerEventGroupMask[0] = ALL_DB_EVENT_0;
    }
    counter++;
    if (MAX_DB_TASKS_END_ID == MAX_CS_ID && DB_TASK_START_ID == 0) {
        dbTriggerEventGroupMask[0] = ALL_DB_EVENT_0;
    } else {
        for (int dbId = DB_TASK_START_ID; dbId < MAX_DB_TASKS_END_ID; dbId++) {
            dbTriggerEventGroupMask[GET_GROUP_EVT_IDX(dbId)] |= (GET_GROUP_EVT_ID(dbId));
        }
    }
}

__ITCMRAM__ inline void dbTriggerDisableAll(void) {
    dbTriggerEventGroupMask[0] = 0;
}

__ITCMRAM__ void timerTriggerDbFromISR(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(dbTriggerEventGroup[0], dbTriggerEventGroupMask[0], &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void dbTriggerThreadInit(int priority, int stackSize) {
    assert(MAX_CS_ID <= GROUP_EVT_CTRL_BITS * 2);
    dbTriggerEventGroup[0] = xEventGroupCreate();

    assert(dbTriggerEventGroup[0] != NULL);

    osThreadDef(dbTriggerTask, dbTriggerThread, priority, 0, stackSize);
    dbTriggerTaskHandle = osThreadCreate(osThread(dbTriggerTask), NULL);
    assert(dbTriggerTaskHandle != NULL);

    // read the default register value and then write it to the timer
    registerInfo_t regInfo = {.mbId = DB_SPI_INTERVAL_US, .type = DATA_UINT};
    registerRead(&regInfo);
    setDbTriggerInterval(regInfo.u.dataUint);
    HAL_TIM_Base_Start_IT(pwmMap[TIM_DB_TRIGGER_MSG].htim);
}

void setDbTriggerInterval(uint32_t interval_us) {

    double pscD =
        (pwmMap[TIM_DB_TRIGGER_MSG].clockFrequency / 1000000.0) * interval_us / (pwmMap[TIM_DB_TRIGGER_MSG].maxArr);
    uint32_t psc = floor(pscD);
    uint32_t arr = (pwmMap[TIM_DB_TRIGGER_MSG].clockFrequency / 1000000) * interval_us / (psc + 1) - 1;

    pwmMap[TIM_DB_TRIGGER_MSG].htim->Init.Prescaler = psc;
    pwmMap[TIM_DB_TRIGGER_MSG].htim->Init.Period = arr;
    pwmMap[TIM_DB_TRIGGER_MSG].htim->Instance->PSC = psc;
    pwmMap[TIM_DB_TRIGGER_MSG].htim->Instance->ARR = arr;

    DPRINTF_INFO("SPI DB INTERVAL %u us TIM5 PSC=%u, ARR=%u\r\n", interval_us, psc, arr);
}

void dbTriggerThread(const void *arg) {
    watchdogAssignToCurrentTask(WDT_TASK_DBTRIGGER);
    watchdogSetTaskEnabled(WDT_TASK_DBTRIGGER, 1);
    registerInfo_t regInfo = {.mbId = DB_RETRY_INTERVAL_S, .type = DATA_UINT};
    registerRead(&regInfo);
    uint32_t wdogPeriod = regInfo.u.dataUint * 2000;
    uint32_t taskPeriod = regInfo.u.dataUint * 1000;

    if (taskPeriod > wdogPeriod) {
        taskPeriod = 0xFFFFFFFF / 2;
        wdogPeriod = 0xFFFFFFFF;
    }
    uint32_t lastTaskPeriod = taskPeriod;
    watchdogChangeTaskPeriod(WDT_TASK_DBTRIGGER, wdogPeriod);
    int startupCnt = 0;
    while (1) {
        // read register
        // sleep on value osDelay is in msec to *1000
        registerRead(&regInfo);

        if (taskPeriod > wdogPeriod) {
            taskPeriod = 0xFFFFFFFF / 2;
            wdogPeriod = 0xFFFFFFFF;
        }
        if (taskPeriod != lastTaskPeriod) {
            watchdogChangeTaskPeriod(WDT_TASK_DBTRIGGER, wdogPeriod);
            lastTaskPeriod = taskPeriod;
        }
        watchdogKickFromTask(WDT_TASK_DBTRIGGER);
        if (startupCnt < 2) {
            osDelay(10);
            startupCnt++;
        } else if (startupCnt < 4) {
            osDelay(100);
            startupCnt++;
        } else if (startupCnt < 10) {
            osDelay(1000);
            startupCnt++;
        } else {
            osDelay(taskPeriod);
        }
        dbCommTaskEnableAll();
    }
}
#define CMD_ARG_IDX 1
#define SUBCMD_ARG_IDX 2
#define MAXCMD_ARG 3
int16_t dbTriggerCliCmd(CLI *hCli, int argc, char *argv[]) {
    uint16_t success = 0;
    if (argc > CMD_ARG_IDX) {
        if (argc == SUBCMD_ARG_IDX && strcmp(argv[CMD_ARG_IDX], "interval") == 0) {
            registerInfo_t regInfo = {.mbId = DB_SPI_INTERVAL_US, .type = DATA_UINT};
            RETURN_CODE rc = registerRead(&regInfo);
            if (rc != RETURN_OK) {
                return success;
            }
            CliPrintf(hCli, "DB Trigger Interval %u usec\r\n", regInfo.u.dataUint);
            success = 1;
        } else if (argc == MAXCMD_ARG && strcmp(argv[CMD_ARG_IDX], "interval") == 0) {
            uint32_t spi_interval_us = atoi(argv[SUBCMD_ARG_IDX]);
            registerInfo_t regInfo = {.mbId = DB_SPI_INTERVAL_US, .type = DATA_UINT, .u.dataUint = spi_interval_us};
            RETURN_CODE rc = registerWrite(&regInfo);
            if (rc != RETURN_OK) {
                return success;
            }
            CliPrintf(hCli, "Success\r\n");
            success = 1;
        } else if (argc == 2 && strcmp(argv[CMD_ARG_IDX], "mask") == 0) {
            CliPrintf(hCli, "HEX 23-0 %06x\r\n", dbTriggerEventGroupMask[DB_EVENT_GROUPS_0_23] & 0x00FFFFF);
            success = 1;
        }
    }
    return success;
}

RETURN_CODE dbTriggerMaskRead(const registerInfo_tp regInfo) {
    regInfo->u.dataUint = dbTriggerEventGroupMask[0];
    return RETURN_OK;
}

// Enable or disable each dbProc based on its bit in the mask
// This will update the dbTriggerEventGroupMask
RETURN_CODE dbTriggerMaskWrite(const registerInfo_tp regInfo) {
    for (int evtId = 0; evtId < GROUP_EVT_CTRL_BITS; evtId++) {
        uint32_t dbId = evtId;

        uint32_t evtMask = GET_GROUP_EVT_ID(dbId);
        bool enable = evtMask & regInfo->u.dataUint;
        dbCommTaskEnable(dbId, enable);
    }
    return RETURN_OK;
}
