/**
 * @file
 * Functions for Watchdog task.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#include "taskWatchdog.h"
#include "cmsis_os.h"
#include "debugPrint.h"
#include "rebootReason.h"
#include "saqTarget.h"
#include "string.h"
#include <assert.h>
#include <stdlib.h>

#define IWDG_BASE_CLOCK_FREQUENCY 32000
#define MAX_UPREGISTER 4095
#define MAX_PRESCALER 7

#define MAX_BUFFER 100

#define WATCHDOG_STATS_BUFFER_SZ_BYTES (60 * 50)

#define WDOG_WAKES_X_TIMES_DURATION(SLEEP_DUR, X) (SLEEP_DUR * X)

#define IWDG_RESET_TIMER_SETTING_MS (2000)
#define WATCHDOG_DELAY_MS 500

extern IWDG_HandleTypeDef hiwdg;
extern WatchdogTaskDef_t watchdogTaskDefs[];

int32_t g_rebootTime = INT32_MIN;

void watchdogReboot(int rebootTime_ms) {
    g_rebootTime = rebootTime_ms;
    DPRINTF_INFO("User requested reboot in %d msec\r\n", rebootTime_ms);
}

#if WDOG_DUMP_STATS == 1
void dumpTaskStatsToLog() {
    DPRINTF_INFO("Dumping task stats...\r\n");
    char *taskBuff = pvPortMalloc(WATCHDOG_STATS_BUFFER_SZ_BYTES); // FreeRTOS task statistics printing area
    assert(taskBuff != NULL);
    vTaskGetCustomRunTimeStats(taskBuff);
    DPRINTF_RAW(taskBuff);
    DPRINTF_RAW("\r\n");
    vPortFree(taskBuff);
}
#endif

__ITCMRAM__ void taskWatchdogThread(void const *argument) {
    uint32_t currentTick;
    uint32_t delta;
    watchdogResetPeriodSetting(STANDARD_2SEC_IWDG);
    DPRINTF_INFO("Watchdog task starting...\r\n");
    // Check that the watchdog task will fire 4 times before the IWDG 2 second timer
    // resets the system
    assert((WDOG_WAKES_X_TIMES_DURATION(WATCHDOG_DELAY_MS, 4)) <= IWDG_RESET_TIMER_SETTING_MS);

    while (1) {
        currentTick = HAL_GetTick();

        for (uint8_t i = 0; i < WDT_NUM_TASKS; i++) {
            if (watchdogTaskDefs[i].enabled) {
                delta = currentTick - watchdogTaskDefs[i].lastCheckin;
                if (delta > watchdogTaskDefs[i].period) {
                    DPRINTF_ERROR("Task %u (%s) did not check in with the watchdog within %u ms!\r\n",
                                  watchdogTaskDefs[i].task,
                                  watchdogTaskDefs[i].taskName,
                                  watchdogTaskDefs[i].period);
                    DPRINTF_ERROR("Last check in at: %u, currently: %u (delta %u)\r\n",
                                  watchdogTaskDefs[i].lastCheckin,
                                  currentTick,
                                  delta);
#if WDOG_DUMP_STATS == 1
                    dumpTaskStatsToLog();
#endif
                    rebootReasonSet(REBOOT_WDT, watchdogTaskDefs[i].taskName);
                    DPRINTF_ERROR("Resetting...\r\n");
                    watchdogExpiredHandle(i);

#if WDOG_FAST_RESET == 1
                    watchdogResetPeriodSetting(FAST_IWDG_1MS);
#endif

                    // Let the WDT reset
                    while (1) {
                        // NOP
                        static volatile int cnt = 0;
                        cnt++;
                    }
                } else if (delta > watchdogTaskDefs[i].highWatermark) {
                    watchdogTaskDefs[i].highWatermark = delta;
                }
            }
        }
#if ENABLE_WDG
        HAL_IWDG_Refresh(&hiwdg);
#endif

        osDelay(WATCHDOG_DELAY_MS); // 500 ms
        if (g_rebootTime != INT32_MIN) {
            g_rebootTime -= WATCHDOG_DELAY_MS;
            if (g_rebootTime < WATCHDOG_DELAY_MS) {
                rebootReasonSet(REBOOT_WDT, "RESET REQUEST");
                watchdogResetPeriodSetting(FAST_IWDG_1MS);
            }
        }
    }
}

void watchdogKickFromCurrentTask() {
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    // Get the watchdog task ID from our thread local storage
    uint32_t task = (uint32_t)pvTaskGetThreadLocalStoragePointer(currentTask, TLS_WATCHDOG_ID);
    if (task < WDT_NUM_TASKS && currentTask == watchdogTaskDefs[task].taskHandle) {

        watchdogTaskDefs[task].lastCheckin = HAL_GetTick();
    }
}

__ITCMRAM__ void watchdogKickFromTask(uint32_t task) {
    assert(task < WDT_NUM_TASKS);
    watchdogTaskDefs[task].lastCheckin = HAL_GetTick();
}

void watchdogAssignToCurrentTask(uint32_t task) {
    assert(task < WDT_NUM_TASKS);

    assert(watchdogTaskDefs[task].task == task);
    watchdogTaskDefs[task].taskHandle = xTaskGetCurrentTaskHandle();
    // Save the task ID in thread local storage for easy lookup later
    vTaskSetThreadLocalStoragePointer(NULL, TLS_WATCHDOG_ID, (void *)task);
}

void watchdogSetTaskEnabled(uint32_t task, uint8_t enabled) {
    assert(task < WDT_NUM_TASKS);

    watchdogTaskDefs[task].enabled = enabled;
    if (enabled) {
        watchdogTaskDefs[task].lastCheckin = HAL_GetTick();
    }
}

__weak void watchdogExpiredHandle(uint32_t task) {
}

uint32_t watchdogChangeTaskPeriod(uint32_t task, uint32_t period) {
    uint32_t oldPeriod = 0;
    assert(task < WDT_NUM_TASKS);

    oldPeriod = watchdogTaskDefs[task].period;
    watchdogTaskDefs[task].period = period;
    watchdogTaskDefs[task].lastCheckin = HAL_GetTick();
    return oldPeriod;
}

uint32_t watchdogGetHighWatermark(uint32_t task) {
    assert(task < WDT_NUM_TASKS);
    return watchdogTaskDefs[task].highWatermark;
}

uint32_t watchdogGetPeriod(uint32_t task) {
    assert(task < WDT_NUM_TASKS);
    return watchdogTaskDefs[task].period;
}

char *watchdogGetTaskName(uint32_t task) {
    assert(task < WDT_NUM_TASKS);
    return watchdogTaskDefs[task].taskName;
}

void calculateWatchdogRegisters(float targetFreq, uint32_t *prescaler, uint32_t *upRegister) {
    // Frequency = (1/CLK)*4*2^PR*(UR+1)
    float maxTarget = IWDG_BASE_CLOCK_FREQUENCY;
    maxTarget = 1 / maxTarget;
    maxTarget = maxTarget * 4 * pow(2, MAX_PRESCALER) * (MAX_UPREGISTER + 1);
    if (maxTarget < targetFreq) {
        *prescaler = MAX_PRESCALER;
        *upRegister = MAX_UPREGISTER;
        return;
    }
    float tmp = (targetFreq * IWDG_BASE_CLOCK_FREQUENCY) / (4 * (MAX_UPREGISTER + 1));
    // log2(x) = log10(x)/log10(2)
    tmp = log10(tmp) / log10(2);
    if (tmp < 0) {
        *prescaler = 0;
    } else {
        *prescaler = ceil(tmp);
    }
    *upRegister = floor((targetFreq * IWDG_BASE_CLOCK_FREQUENCY / (4 * pow(2, *prescaler))) - 1);
    float freqActual = (4 * pow(2, *prescaler) * (*upRegister + 1)) / IWDG_BASE_CLOCK_FREQUENCY;
    char buffer[MAX_BUFFER];
    snprintf(buffer,
             MAX_BUFFER,
             "IWDG Target = %f  Actual=%f  (prescaler=%ld, upReg=%ld)\r\n",
             targetFreq,
             freqActual,
             *prescaler,
             *upRegister);

    DPRINTF_INFO(buffer);
}
