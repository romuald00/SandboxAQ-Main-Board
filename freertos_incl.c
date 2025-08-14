/*
 * freertos_incl.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 19, 2023
 *      Author: rlegault
 */

// This file is include in freertos.c across all processors
#include "cmsis_os.h"
#include "debugPrint.h"
#include "stmTarget.h"
#include <stdio.h>
#include <string.h>

#define TASKNAME_LEN 12
char _taskName[TASKNAME_LEN];
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName) {
    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
    called if a stack overflow is detected. */
    strlcpy(_taskName, (char *)pcTaskName, TASKNAME_LEN);
    DPRINTF_ERROR("Stack overflow !! Task %s\r\n", pcTaskName);
    assert(0);
}

static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
    /* place for user code */
}

static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize) {
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
    *ppxTimerTaskStackBuffer = &xTimerStack[0];
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
    /* place for user code */
}

/* USER CODE BEGIN Application */
// This is an extended version of vTaskGetRunTimeStats() which provides more information on task priorities and states
__ITCMRAM__ void vTaskGetCustomRunTimeStats(char *pcWriteBuffer) {
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t ulTotalTime, ulStatsAsPercentage;

    /* Make sure the write buffer does not contain a string. */
    *pcWriteBuffer = 0x00;

    /* Take a snapshot of the number of tasks in case it changes while this
    function is executing. */
    uxArraySize = uxTaskGetNumberOfTasks();

    /* Allocate an array index for each task.  NOTE!  If
    configSUPPORT_DYNAMIC_ALLOCATION is set to 0 then pvPortMalloc() will
    equate to NULL. */
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if (pxTaskStatusArray != NULL) {
        /* Generate the (binary) data. */
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalTime);

        /* For percentage calculations. */
        ulTotalTime /= 100UL;
        sprintf(pcWriteBuffer,
                "%-16s %-10s %-7s %-2s %-2s %-2s %-5s\r\n",
                "Task Name",
                "Run Time",
                "Run Perc",
                "PRIO",
                "BPRIO",
                "STATE",
                "STACK_HIGH");
        pcWriteBuffer += strlen(pcWriteBuffer);
        /* Avoid divide by zero errors. */
        if (ulTotalTime > 0) {
            /* Create a human readable table from the binary data. */
            for (x = 0; x < uxArraySize; x++) {
                /* What percentage of the total run time has the task used?
                This will always be rounded down to the nearest integer.
                ulTotalRunTimeDiv100 has already been divided by 100. */
                ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter / ulTotalTime;

                if (ulStatsAsPercentage > 0UL) {
                    /* sizeof( int ) == sizeof( long ) so a smaller
                    printf() library can be used. */
                    sprintf(pcWriteBuffer,
                            "%-16s %10u %3u/100 %2u %2u %2u %u\r\n",
                            pxTaskStatusArray[x].pcTaskName,
                            (unsigned int)pxTaskStatusArray[x].ulRunTimeCounter,
                            (unsigned int)ulStatsAsPercentage,
                            (unsigned int)pxTaskStatusArray[x].uxCurrentPriority,
                            (unsigned int)pxTaskStatusArray[x].uxBasePriority,
                            (unsigned int)pxTaskStatusArray[x].eCurrentState,
                            (unsigned int)pxTaskStatusArray[x].usStackHighWaterMark);
                } else {
                    /* If the percentage is zero here then the task has
                    consumed less than 1% of the total run time. */
                    sprintf(pcWriteBuffer,
                            "%-16s %10u  <1/100 %2u %2u %2u %u\r\n",
                            pxTaskStatusArray[x].pcTaskName,
                            (unsigned int)pxTaskStatusArray[x].ulRunTimeCounter,
                            (unsigned int)pxTaskStatusArray[x].uxCurrentPriority,
                            (unsigned int)pxTaskStatusArray[x].uxBasePriority,
                            (unsigned int)pxTaskStatusArray[x].eCurrentState,
                            (unsigned int)pxTaskStatusArray[x].usStackHighWaterMark);
                }

                pcWriteBuffer += strlen(pcWriteBuffer);
            }
        } else {
            // mtCOVERAGE_TEST_MARKER();
        }

        /* Free the array again.  NOTE!  If configSUPPORT_DYNAMIC_ALLOCATION
        is 0 then vPortFree() will be #defined to nothing. */
        vPortFree(pxTaskStatusArray);
    } else {
        // mtCOVERAGE_TEST_MARKER();
    }
}

__ITCMRAM__ const char *getTaskName(osThreadId threadId) {
    if (threadId != NULL) {
        TaskStatus_t xTaskDetails;
        vTaskGetInfo(threadId, &xTaskDetails, pdFALSE, eRunning);
        return xTaskDetails.pcTaskName;
    }
    return "None";
}

/* Determine whether we are in thread mode or handler mode. */
__ITCMRAM__ static int inHandlerMode(void) {
    return __get_IPSR() != 0;
}

__ITCMRAM__ const char *getcurrentTaskName(void) {
    volatile uint32_t cnt = 0;
    if (inHandlerMode()) {
        cnt++;
        return "ISR";
    }
    return getTaskName(osThreadGetId());
}
/* USER CODE END Application */
