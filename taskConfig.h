/*
 * pwmPinConfig.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 10, 2023
 *      Author: rlegault
 */

#ifndef __TASK_CONFIG_H__
#define __TASK_CONFIG_H__

#include "cmsis_os.h"

//#define STARTUP_TASK_PRIORITY   osPriorityNormal //IMPORTANT: this is configured in stmCubeMx (copied here for
// reference)
#define WATCHDOG_TASK_PRIORITY osPriorityRealtime
#define CLI_TASK_PRIORITY osPriorityBelowNormal

//#define STARTUP_STACK_SIZE             600 //IMPORTANT: this is configured in stmCubeMx (copied here for reference)
#define WATCHDOG_STACK_SIZE 256
#define CLI_STACK_SIZE 2048

#endif /* __TASK_CONFIG_H__ */
