/**
 * @file
 * Functions for Watchdog task.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#include "watchDog.h"
#include "macros.h"
#include "main.h"
#include "stmTarget.h"
#include "taskConfig.h"
#include "taskWatchdog.h"

extern IWDG_HandleTypeDef hiwdg;

osThreadId watchDogTaskHandle;

#define WDT_TASK_ROW(ID)                                                                                               \
    { WDT_TASK_DBCOMM_##ID, NULL, "dbComm" #ID, 3000, 0, 0, 0 }

// Note: The ordering in this list must be the same as the enum ordering
WatchdogTaskDef_t watchdogTaskDefs[WDT_NUM_TASKS] = {{WDT_TASK_CLI, NULL, "cliTask", 6000, 0, 0, 0},
                                                     {WDT_TASK_CNC, NULL, "cncTask", 6000, 0, 0, 0},
                                                     {WDT_TASK_CTRLCOMM_SPI1, NULL, "commSPI1", 6000, 0, 0, 0},
                                                     {WDT_TASK_CTRLCOMM_SPI2, NULL, "commSPI2", 6000, 0, 0, 0},
                                                     {WDT_TASK_CTRLCOMM_SPI3, NULL, "commSPI3", 6000, 0, 0, 0},
                                                     {WDT_TASK_CTRLCOMM_SPI4, NULL, "commSPI4", 6000, 0, 0, 0},
                                                     {WDT_TASK_DBTRIGGER, NULL, "dbTrigger", 6000, 0, 0, 0},
                                                     WDT_TASK_ROW(0),
                                                     WDT_TASK_ROW(1),
                                                     WDT_TASK_ROW(2),
                                                     WDT_TASK_ROW(3),
                                                     WDT_TASK_ROW(4),
                                                     WDT_TASK_ROW(5),
                                                     WDT_TASK_ROW(6),
                                                     WDT_TASK_ROW(7),
                                                     WDT_TASK_ROW(8),
                                                     WDT_TASK_ROW(9),
                                                     WDT_TASK_ROW(10),
                                                     WDT_TASK_ROW(11),
                                                     WDT_TASK_ROW(12),
                                                     WDT_TASK_ROW(13),
                                                     WDT_TASK_ROW(14),
                                                     WDT_TASK_ROW(15),
                                                     WDT_TASK_ROW(16),
                                                     WDT_TASK_ROW(17),
                                                     WDT_TASK_ROW(18),
                                                     WDT_TASK_ROW(19),
                                                     WDT_TASK_ROW(20),
                                                     WDT_TASK_ROW(21),
                                                     WDT_TASK_ROW(22),
                                                     WDT_TASK_ROW(23),
                                                     WDT_TASK_ROW(24),
                                                     WDT_TASK_ROW(25),
                                                     WDT_TASK_ROW(26),
                                                     WDT_TASK_ROW(27),
                                                     WDT_TASK_ROW(28),
                                                     WDT_TASK_ROW(29),
                                                     WDT_TASK_ROW(30),
                                                     WDT_TASK_ROW(31),
                                                     WDT_TASK_ROW(32),
                                                     WDT_TASK_ROW(33),
                                                     WDT_TASK_ROW(34),
                                                     WDT_TASK_ROW(35),
                                                     WDT_TASK_ROW(36),
                                                     WDT_TASK_ROW(37),
                                                     WDT_TASK_ROW(38),
                                                     WDT_TASK_ROW(39),
                                                     WDT_TASK_ROW(40),
                                                     WDT_TASK_ROW(41),
                                                     WDT_TASK_ROW(42),
                                                     WDT_TASK_ROW(43),
                                                     WDT_TASK_ROW(44),
                                                     WDT_TASK_ROW(45),
                                                     WDT_TASK_ROW(46),
                                                     WDT_TASK_ROW(47),
                                                     {WDT_TASK_MONGOOSE, NULL, "mongoose", 2000, 0, 0, 0},
                                                     {WDT_TASK_GATHER, NULL, "gather", 2000, 0, 0, 0},
                                                     {WDT_TASK_UDPCONNECTION, NULL, "udp", 2000, 0, 0, 0},
                                                     {WDT_TASK_RESET, NULL, "resetTask", 4000, 0, 0, 0},
                                                     {WDT_TASK_DDSTRIGGER, NULL, "ddsTrig", 4000, 0, 0, 0}};

// Initializes the watchdog task.
// This assumes the hardware watchdog is already configured
void watchDogTaskInit(int priority, int stackSize) {
    // create the watchdog task
    osThreadDef(watchDogTask, taskWatchdogThread, priority, 0, stackSize);
    watchDogTaskHandle = osThreadCreate(osThread(watchDogTask), NULL);
}

// Implement hook for when a task expires
void watchdogExpiredHandle(uint32_t task) {
    // blShared.swResetReason |= SW_RESET_REASON_WDT_TIMEOUT;
    // blShared.wdtTimeoutTask = task;
}

// Wind down the watchdog quickly to reset immediately
#define DISABLE_IWDG_WINDOW 4095
void watchdogResetPeriodSetting(float targetPeriod) {
    uint32_t prescaler;
    uint32_t reloadRegister;
    calculateWatchdogRegisters(targetPeriod, &prescaler, &reloadRegister);
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = prescaler;
    hiwdg.Init.Reload = reloadRegister;
    hiwdg.Init.Window = DISABLE_IWDG_WINDOW;
    HAL_IWDG_Init(&hiwdg);
}
