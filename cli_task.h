/*
 * cli_task.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 26, 2023
 *      Author: jzhang
 */

#ifndef APP_INC_CLI_TASK_H_
#define APP_INC_CLI_TASK_H_

#include "cli/cli.h"
#include "cmsis_os.h"
#include "stmTarget.h"

#define STALL_CLI_TIMEOUT_MS 1000

extern UART_HandleTypeDef CLI_UART;

/**
 * @fn cliTaskInit
 *
 * @brief Initialize the CLI
 *
 * @param[in]     priority thread priority
 * @param[in]     stackSz  thread stack size
 **/
void cliTaskInit(int priority, int stackSize);

/**
 * @fn cliCommandReadyCallback
 *
 * @brief Callback function when command is received on CLI
 *
 * @param[in] CLI *hCli: CLI instance
 **/
void cliCommandReadyCallback(CLI *hCli);

/**
 * @fn testdumplogCommand
 *
 * @brief CLI command to test dump log
 *
 * @param[in] CLI *hCli: CLI instance
 * @param[in] int argc: Number or command line arguments
 * @param[in] char *argv[]: List of command line arguments
 *
 * @return CLI_RESULT_SUCCESS on success
 **/
int16_t testdumplogCommand(CLI *hCli, int argc, char *argv[]);

/**
 * @fn stallCliUntilComplete
 *
 * @brief The cli will stall until cliCommandComplete is called which must
 *   be called within 2 seconds.
 *
 * @param[in] timeout_ms: amount of time to wait for the semaphore to be released.
 *
 * @return osStatus from osSemaphoreWait
 **/
osStatus stallCliUntilComplete(uint32_t timeout_ms);

/**
 * @fn cliCommandComplete
 *
 * @brief The cli will stall until this command is called. It is the other half
 * of stallCliUntilComplete
 *
 ** @return osStatus from osSemaphoreRelease
 **/
osStatus cliCommandComplete(void);

#endif /* APP_INC_CLI_TASK_H_ */
