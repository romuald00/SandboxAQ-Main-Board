/**
 * @file
 * Definitions for Watchdog task.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef __TASK_WATCHDOG_H__
#define __TASK_WATCHDOG_H__

#include "FreeRTOS.h"
#include "task.h"
#include "watchDog.h"
#include <stdint.h>

#define WDOG_TASK_NAME_LEN_MAX 20

#define FAST_IWDG_1MS (0.001)
#define STANDARD_2SEC_IWDG (2.0)

typedef struct WatchdogTaskDef {
    uint32_t task;
    TaskHandle_t taskHandle;
    char taskName[WDOG_TASK_NAME_LEN_MAX];
    uint32_t period;
    uint32_t lastCheckin;
    uint32_t highWatermark;
    uint8_t enabled;
} WatchdogTaskDef_t;

/**
 * @fn taskWatchdogThread
 *
 * @brief Check that each task is not being starved of CPU cycle and is not locked up
 *
 * @param[in] argument, not used
 **/
void taskWatchdogThread(void const *argument);

/**
 * @fn watchdogKickFromCurrentTask
 *
 * @brief Helper method, each task needs to signal the watchdog periodically.
 *
 * @Note useful for a long run function that can be called from different tasks
 *
 **/
void watchdogKickFromCurrentTask();

/**
 * @fn watchdogKickFromTask
 *
 * @brief Helper method, each task needs to signal the watchdog periodically.
 *
 * @Note quicker call then having to detect your thread.
 *
 * @param task: identify the task
 *
 **/
void watchdogKickFromTask(uint32_t task);

/**
 * @fn watchdogAssignToCurrentTask
 *
 * @brief associate the current task to the Watchdog task id
 *
 * @param task: identify the task
 *
 **/
void watchdogAssignToCurrentTask(uint32_t task);

/**
 * @fn watchdogSetTaskEnabled
 *
 * @brief enable or disable the watchdog checking of this task
 *
 * @param task: identify the task
 *
 * @param enable: 0=disable, 1=enable
 **/
void watchdogSetTaskEnabled(uint32_t task, uint8_t enabled);

/**
 * @fn watchdogExpiredHandle
 *
 * @brief handel when a task has expired.
 *
 * @note in this implementation no monitored tasks expire
 *
 * @param task: identify the task
 *
 **/
void watchdogExpiredHandle(uint32_t task);

/**
 * @fn watchdogChangeTaskPeriod
 *
 * @brief Configure the task update period before declaring it as failed.
 *
 * @param task: identify the task
 *
 * @param period_ms: new period in millisec
 *
 * @return returns the previous setting in millesec
 **/
uint32_t watchdogChangeTaskPeriod(uint32_t task, uint32_t period_ms);

/**
 * @fn watchdogGetHighWatermark
 *
 * @brief return the longest time between updates by this task.
 *
 * @param task: identify the task
 *
 * @return high water mark in milli seconds
 **/
uint32_t watchdogGetHighWatermark(uint32_t task);

/**
 * @fn watchdogGetPeriod
 *
 * @brief return the task's configure period.
 *
 * @param task: identify the task
 *
 * @return the task's period in milli seconds
 **/
uint32_t watchdogGetPeriod(uint32_t task);

/**
 * @fn watchdogGetTaskName
 *
 * @brief return the task's string name.
 *
 * @param task: identify the task
 *
 * @return the task's name
 **/
char *watchdogGetTaskName(uint32_t task);

/**
 * @fn watchdogReboot
 *
 * @brief set up watch dog to cause reset in rebootTime_ms
 *
 * @note the granularity of rebootTime_ms is actually 500ms
 *
 * @param rebootTime_ms: time to wait before causing system to reboot
 *
 **/
void watchdogReboot(int rebootTime_ms);

/**
 * @fn calculateWatchdogRegisters
 *
 * @brief calculate the prescaler and upregister values for the target frequency
 *
 * @note if targetFreq exceeds max then it will set register values returned to max
 *
 * @note assumes 32000 Hz base clock
 *
 * @param targetPeriod: time to wait before watchdog will reset without writing to its reset register
 *
 * @param *prescaler: returns calculated prescaler(PR) value to meet target period
 *
 * @param *upRegister: returns calculated reload(RL) register value to meet target period
 **/
void calculateWatchdogRegisters(float targetPeriod, uint32_t *prescaler, uint32_t *upRegister);

#endif /* __TASK_WATCHDOG_H__ */
