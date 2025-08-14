/*
 * ddsTrigTask.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Feb 23, 2024
 *      Author: rlegault
 */

#include "ddsTrigTask.h"

#include "cli/cli_print.h"
#include "cmdAndCtrl.h"
#include "cmsis_os.h"
#include "debugPrint.h"
#include "pwm.h"
#include "pwmPinConfig.h"
#include "saqTarget.h"
#include "taskWatchdog.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define CMD_IDX 1
#define DATA_IDX 2

#define CMD_DATA_IP_ADDR_IDX 2
#define CMD_DATA_NET_MASK_IDX 3
#define CMD_DATA_GATEWAY_IDX 4
#define PWM_DUTY_100PERCENT 100
#define PWM_DUTY_50PERCENT 50

#define SB_BOARD_STARTUP_MS                                                                                            \
    500 // delay to give time for all boards to configure their DDS and then trigger enable will cause them to be in
        // sync.
/* only works on boot. otherwise to resync their phases they need to
    disable trigger from main board,
    toggle reset from sensor board
    write configuration to DDS registers.
    Enable trigger from main board.
*/

void ddsTrigThread(const void *arg);

__DTCMRAM__ EventGroupHandle_t ddsTriggerEvent = NULL;

#define PENDING_DELAY 1000
osThreadId ddsTrigTaskHandle = NULL;
bool ddsTrigEn = true;

#define DDS_TRIG_EVT 1

static void ddsWaveEnable(void) {
    // wave generation is enabled when the trigger is low
    HAL_GPIO_WritePin(pwmMap[DDS_PIN].gpioPort, pwmMap[DDS_PIN].gpioPin, 0);
}

void ddsWaveDisable(void) {
    // wave generation is disabled when the trigger is high
    HAL_GPIO_WritePin(pwmMap[DDS_PIN].gpioPort, pwmMap[DDS_PIN].gpioPin, 1);
}

static void coilDDSWaveEnable(void) {
    // wave generation is enabled when the trigger is low
    pwmPort_e port = PWM_SPARE_1;
    // the wave form is enabled when the line goes low.
    pwmSetDutyCycle(port, 0);
    pwmStart(port);
}

void coilDDSWaveDisable(void) {
    // wave generation is disabled when the trigger is high
    pwmPort_e port = PWM_SPARE_1;
    // The Wave form is disabled when the line stays high.
    pwmSetDutyCycle(port, 100);
    pwmStart(port);
}

static volatile uint32_t nopCnt;
static void ddsWaveSync(void) {
    ddsWaveDisable();
    for (int i = 0; i < 1000; i++) {
        nopCnt++;
    }
    ddsWaveEnable();
}

void ddsTriggerSync(void) {
    xEventGroupSetBits(ddsTriggerEvent, DDS_TRIG_EVT);
}

void ddsTriggerEnable(void) {
    ddsTrigEn = true;
    ddsTriggerSync();
}

void ddsTriggerDisable(void) {
    ddsTrigEn = false;
    ddsWaveDisable();
}

void ddsTrigThreadInit(int priority, int stackSize) {
    ddsTriggerEvent = xEventGroupCreate();
    assert(ddsTriggerEvent != NULL);
    osThreadDef(ddsTrigTask, ddsTrigThread, priority, 0, stackSize);
    ddsTrigTaskHandle = osThreadCreate(osThread(ddsTrigTask), NULL);
    assert(ddsTrigTaskHandle != NULL);
}

void ddsTrigThread(const void *arg) {
    watchdogAssignToCurrentTask(WDT_TASK_DDSTRIGGER);
    watchdogSetTaskEnabled(WDT_TASK_DDSTRIGGER, 1);
    bool startRequestPending = false;
    uint32_t pendingTick = 0;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 1); // Enable clock to the DDS
    osDelay(SB_BOARD_STARTUP_MS);            // Give time for DDS to be configured
    ddsWaveEnable();
    coilDDSWaveEnable();
    ddsTrigEn = true;

    while (1) {
        watchdogKickFromTask(WDT_TASK_DDSTRIGGER);
        EventBits_t evBits = xEventGroupWaitBits(ddsTriggerEvent, DDS_TRIG_EVT, true, true, 1000);
        if (evBits & DDS_TRIG_EVT) {
            if (startRequestPending == false) {
                pendingTick = HAL_GetTick();
                startRequestPending = true;
            }
        }
        if (startRequestPending && (HAL_GetTick() - pendingTick > PENDING_DELAY)) {
            startRequestPending = false;
            ddsWaveSync();
        }
    }
}

int16_t ddsTrigCliCmd(CLI *hCli, int argc, char *argv[]) {
    uint16_t success = 0;
    if (argc > CMD_IDX) {
        if (argc == CMD_IDX + 1) {
            if (strcmp(argv[CMD_IDX], "start") == 0) {
                ddsTriggerEnable();
                success = true;
            } else if (strcmp(argv[1], "sync") == 0) {
                if (ddsTrigEn) {
                    success = true;
                    ddsWaveSync();
                } else {
                    CliPrintf(hCli, "dds must be enabled first\r\n");
                }

            } else if (strcmp(argv[1], "stop") == 0) {
                ddsTriggerDisable();
                success = true;
            }
        }
    }
    if (success) {
        return CLI_RESULT_SUCCESS;
    }
    return CLI_RESULT_ERROR;
}

int16_t ddsClockCliCmd(CLI *hCli, int argc, char *argv[]) {
    uint16_t success = 0;
    pwmPort_e port = PWM_SPARE_0;
    if (argc > CMD_IDX) {
        if (argc > DATA_IDX && strcmp(argv[CMD_IDX], "set_duty") == 0) {
            uint32_t duty = atoi(argv[DATA_IDX]);
            CliPrintf(hCli, "Duty: %d\n\r", duty);
            pwmPinStates[port].dutyInfo.u.dataUint = duty;
            if (registerWrite(&pwmPinStates[port].dutyInfo) != 0) {
                CliWriteString(hCli, "Failure\n\r");
                return CLI_RESULT_ERROR;
            }
            CliPrintf(hCli, "Success: %d\n\r", pwmPinStates[port].dutyInfo.u.dataUint);
            success = true;
        } else if (argc > DATA_IDX && strcmp(argv[CMD_IDX], "set_frequency") == 0) {
            uint32_t freq = atoi(argv[DATA_IDX]);
            CliPrintf(hCli, "Frequency: %d\n\r", freq);
            pwmPinStates[port].freqInfo.u.dataUint = freq * 100; // scale factor
            if (registerWrite(&pwmPinStates[port].freqInfo) != 0) {
                CliWriteString(hCli, "Failure\n\r");
                return CLI_RESULT_ERROR;
            }
            CliPrintf(hCli, "Success: %d\n\r", pwmPinStates[port].freqInfo.u.dataUint / 100);
            success = true;
        } else if (argc == (CMD_IDX + 1) && strcmp(argv[CMD_IDX], "get_duty") == 0) {
            if (registerRead(&pwmPinStates[port].dutyInfo) != 0) {
                CliWriteString(hCli, "Failure\n\r");
                return CLI_RESULT_ERROR;
            }
            CliPrintf(hCli, "Duty: %d\n\r", pwmPinStates[port].dutyInfo.u.dataUint);
            success = true;
        } else if (argc == (CMD_IDX + 1) && strcmp(argv[CMD_IDX], "get_frequency") == 0) {
            if (registerRead(&pwmPinStates[port].freqInfo) != 0) {
                CliWriteString(hCli, "Failure\n\r");
                return CLI_RESULT_ERROR;
            }
            CliPrintf(hCli, "Frequency: %d hz\n\r", pwmPinStates[port].freqInfo.u.dataUint / 100);
            success = true;
        } else if (strcmp(argv[CMD_IDX], "start") == 0) {
            int ret = pwmStart(port);
            CliPrintf(hCli, "Success: %d\n\r", ret);
            success = true;
        } else if (strcmp(argv[CMD_IDX], "stop") == 0) {
            int ret = pwmStop(port);
            CliPrintf(hCli, "Success: %d\n\r", ret);
            success = true;
        }
    }
    if (success) {
        return CLI_RESULT_SUCCESS;
    }
    return CLI_RESULT_ERROR;
}
