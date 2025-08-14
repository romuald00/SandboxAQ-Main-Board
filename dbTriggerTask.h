/*
 * dbTriggerTask.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 18, 2023
 *      Author: rlegault
 */

#ifndef APP_INC_DBTRIGGERTASK_H_
#define APP_INC_DBTRIGGERTASK_H_

#include "cli/cli_print.h"
#include "cmsis_os.h"
#include "registerParams.h"
#include "saqTarget.h"
#include <stdint.h>

#define DB_EVENT_GROUPS_0_23 0

#define MAX_DB_EVENT_GROUPS 1 // 24 possible daughter boards, group of 24
extern EventGroupHandle_t dbTriggerEventGroup[MAX_DB_EVENT_GROUPS];

void timerTriggerDbFromISR(void);

/**
 *
 * Create daughterboard task triggers for sensor data requests
 *
 * @param[in]     priority thread priority
 * @param[in]     stackSz  thread stack size
 */

void dbTriggerThreadInit(int priority, int stackSize);

/**
 *
 * Set the trigger interval for spi messages to the
 * daughter boards
 *
 * @param[in]     interval in micro seconds
 */
void setDbTriggerInterval(uint32_t interval_us);

/**
 *
 * Disable Trigger for Daughter board process
 *
 *
 * @param[in]     daughter board ID
 */
void dbTriggerDisable(uint32_t dbId);

/**
 *
 * Disable Trigger for Daughter board process
 *
 *
 * @param[in]     daughter board ID
 */
void dbTriggerEnable(uint32_t dbId);

/**
 *
 * Enable Trigger for all Daughter board process
 *
 *
 */
void dbTriggerEnableAll(void);

/**
 *
 * Disable Trigger for all Daughter board process
 *
 *
 */
void dbTriggerDisableAll(void);

/**
 *
 * Handle Cli command
 *
 * @param[in] hCli, display handle
 * @param[in] argc, number of args in argv
 * @param[in] argv, list of string parameters
 *
 * @return success 1 or failure 0
 */
int16_t dbTriggerCliCmd(CLI *hCli, int argc, char *argv[]);

/**
 *
 * Register Write handler to update mask values
 *
 * @param[in] regInfo contains information about the register to update
 *
 * @return result 0=OK
 */
RETURN_CODE dbTriggerMaskWrite(const registerInfo_tp regInfo);

/**
 *
 * Register Write handler to update mask values
 *
 * @param[in/out] regInfo contains information about the register and location
 * to copy the data
 *
 * @return result 0=OK
 */
RETURN_CODE dbTriggerMaskRead(const registerInfo_tp regInfo);

#endif /* APP_INC_DBTRIGGERTASK_H_ */
