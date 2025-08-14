/**
 * @file
 * Definitions for Watchdog task.
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef __WATCH_DOG_H__
#define __WATCH_DOG_H__

#include "cmsis_os.h"
#include "stmTarget.h"

#define WDOG_FAST_RESET 1
#define WDOG_DUMP_STATS 0 // on main board buffer to write task data into is too small.
#define ENABLE_WDG 1

// NOTE: If you update this table, you must also update "watchdogTaskDefs" in watchDog.c
typedef enum WatchdogTask {
    WDT_TASK_CLI = 0,
    WDT_TASK_CNC,
    WDT_TASK_CTRLCOMM_SPI1,
    WDT_TASK_CTRLCOMM_SPI2,
    WDT_TASK_CTRLCOMM_SPI3,
    WDT_TASK_CTRLCOMM_SPI4,
    WDT_TASK_DBTRIGGER,
    WDT_TASK_DBCOMM_0,
    WDT_TASK_DBCOMM_1,
    WDT_TASK_DBCOMM_2,
    WDT_TASK_DBCOMM_3,
    WDT_TASK_DBCOMM_4,
    WDT_TASK_DBCOMM_5,
    WDT_TASK_DBCOMM_6,
    WDT_TASK_DBCOMM_7,
    WDT_TASK_DBCOMM_8,
    WDT_TASK_DBCOMM_9,
    WDT_TASK_DBCOMM_10,
    WDT_TASK_DBCOMM_11,
    WDT_TASK_DBCOMM_12,
    WDT_TASK_DBCOMM_13,
    WDT_TASK_DBCOMM_14,
    WDT_TASK_DBCOMM_15,
    WDT_TASK_DBCOMM_16,
    WDT_TASK_DBCOMM_17,
    WDT_TASK_DBCOMM_18,
    WDT_TASK_DBCOMM_19,
    WDT_TASK_DBCOMM_20,
    WDT_TASK_DBCOMM_21,
    WDT_TASK_DBCOMM_22,
    WDT_TASK_DBCOMM_23,
    WDT_TASK_DBCOMM_24,
    WDT_TASK_DBCOMM_25,
    WDT_TASK_DBCOMM_26,
    WDT_TASK_DBCOMM_27,
    WDT_TASK_DBCOMM_28,
    WDT_TASK_DBCOMM_29,
    WDT_TASK_DBCOMM_30,
    WDT_TASK_DBCOMM_31,
    WDT_TASK_DBCOMM_32,
    WDT_TASK_DBCOMM_33,
    WDT_TASK_DBCOMM_34,
    WDT_TASK_DBCOMM_35,
    WDT_TASK_DBCOMM_36,
    WDT_TASK_DBCOMM_37,
    WDT_TASK_DBCOMM_38,
    WDT_TASK_DBCOMM_39,
    WDT_TASK_DBCOMM_40,
    WDT_TASK_DBCOMM_41,
    WDT_TASK_DBCOMM_42,
    WDT_TASK_DBCOMM_43,
    WDT_TASK_DBCOMM_44,
    WDT_TASK_DBCOMM_45,
    WDT_TASK_DBCOMM_46,
    WDT_TASK_DBCOMM_47,
    WDT_TASK_MONGOOSE,
    WDT_TASK_GATHER,
    WDT_TASK_UDPCONNECTION,
    WDT_TASK_RESET,
    WDT_TASK_DDSTRIGGER,
    WDT_NUM_TASKS // Not a real task. Must be at the end
} WatchdogTask_e;

/**
 * @fn watchDogTaskInit
 *
 * @brief Initialize the watchdog task
 *
 * @param priority  OS task priority
 *
 * @param stackSize  OS stak size in words
 *
 **/
void watchDogTaskInit(int priority, int stackSize);

/**
 * @fn watchdogResetPeriodSetting
 *
 * @brief Initialize the watchdog task
 *
 * @param targetPeriod IWDG reset period, if watchdog counter is not reset within a period system
 * will reboot
 *
 **/
void watchdogResetPeriodSetting(float targetPeriod);

#endif /* __WATCH_DOG_H__ */
