/*
 * cli_task.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 26, 2023
 *      Author: jzhang
 */

#include "cli_task.h"

#include <string.h>

#include "cli/cli.h"
#include "cli/cli_print.h"
#include "cli/cli_uart.h"
#include "cli_commands.h"
#include "cmsis_os.h"
#include "debugPrint.h"
#include "macros.h"
#include "stmTarget.h"
#include "taskWatchDog.h"
#include "watchDog.h"

// RTOS thread ids
osThreadId cliTaskHandle;
void startCliTask(void const *argument);

__DTCMRAM__ StaticTask_t cliTaskCtrlBlock;
__DTCMRAM__ StackType_t cliTaskStack[CLI_STACK_WORDS];

osMessageQDef(cliQueue, 128, uint32_t);
osMessageQId cliQueueId = NULL;

static osSemaphoreId cliCmdCompleteId = NULL;

CLI hCli;

static uint8_t inputBuf[MIN_CLI_BUFFER_SIZE];                             // input buffer for CLI
static uint8_t historyBuf[MIN_CLI_BUFFER_SIZE * CLI_MAX_COMMAND_HISTORY]; // history buffer for CLI

osStatus stallCliUntilComplete(uint32_t timeout_ms) {
    return osSemaphoreWait(cliCmdCompleteId, timeout_ms);
}

osStatus cliCommandComplete(void) {
    return osSemaphoreRelease(cliCmdCompleteId);
}

void cliTaskInit(int priority, int stackSize) {
    // create the cli task
    assert(stackSize == CLI_STACK_WORDS);
    osSemaphoreDef(cliCmdComplete); // updated by either RXTX complete or queueSend.
    cliCmdCompleteId = osSemaphoreCreate(osSemaphore(cliCmdComplete), 1);
    osStatus status = osSemaphoreWait(cliCmdCompleteId, 0); // block so release continue execution;
    assert(status == osOK);
    osThreadStaticDef(cliTask, startCliTask, priority, 0, stackSize, cliTaskStack, &cliTaskCtrlBlock);
    cliTaskHandle = osThreadCreate(osThread(cliTask), NULL);
    assert(cliTaskHandle != NULL);
    cliQueueId = osMessageCreate(osMessageQ(cliQueue), NULL);

    CliInit(&hCli, 0, NUM_COMMANDS, cliCommandList, "$ ", inputBuf, historyBuf, &cliCommandReadyCallback, NULL);

    __HAL_UART_ENABLE_IT(&CLI_UART, UART_IT_RXNE);
}

void startCliTask(void const *argument) {
    osEvent evt;
    DPRINTF_CLI("CLI task started\r\n");
    watchdogAssignToCurrentTask(WDT_TASK_CLI);
    watchdogSetTaskEnabled(WDT_TASK_CLI, 1);

    while (1) {
        // wait for incoming data
        evt = osMessageGet(cliQueueId, 1000); // wait for message
        uint8_t data;
        if (evt.status == osEventMessage) {
            data = evt.value.v;
            CliCharacterReceivedCallback(&hCli, data);
        }

        watchdogKickFromCurrentTask();
    }
}

void cliCommandReadyCallback(CLI *hCli) {
    const CLI_COMMAND *cmd;
    CLI_RESULT result;
    int argc;
    char *argv[CLI_MAX_COMMAND_ARGUMENTS];

    result = CliGetNewCommand(hCli, &cmd, &argc, argv);

    if (result == CLI_RESULT_SUCCESS && cmd != NULL) {
        cmd->function(hCli, argc, argv);
    }

    CliGetReadyForNewCommand(hCli);
}

int16_t testdumplogCommand(CLI *hCli, int argc, char *argv[]) {
    watchdogKickFromCurrentTask();

    uint32_t usableLength = usableRamLogLength();
    uint32_t lengthSent = 0;
    uint32_t chunkSize;
    uint16_t inputLen = 0;
    uint8_t inputBuf[1];

    CliWriteString(hCli, "Press any key to abort dump...\r\n");

    while (lengthSent < usableLength) {
        watchdogKickFromCurrentTask();

        chunkSize = (usableLength - lengthSent) < 4096 ? usableLength - lengthSent : 4096;

        if (CliWriteOnUart(hCli->cliUart, (uint8_t *)ramLogReadFrom(lengthSent), chunkSize)) {
            break;
        }

        inputLen = SmallRbRead(&hCli->stdinBuffer.ringBuffer, inputBuf, ARRAY_SIZE(inputBuf));

        if (inputLen > 0) {
            break;
        }

        lengthSent += chunkSize;
    }

    CliWriteString(hCli, "\r\n");
    return CLI_RESULT_SUCCESS;
}
