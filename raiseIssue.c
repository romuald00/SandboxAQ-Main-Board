/*
 * raiseIssue.c
 *
 *  Functions to log and set led for detected failures
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Sep 9, 2024
 *      Author: rlegault
 */

#include "raiseIssue.h"

#include "cli/cli_print.h"
#include "cmsis_os.h"
#include "pwm.h"
#include "pwmPinConfig.h"
#ifdef STM32H743xx
#include "MB_gatherTask.h"
#endif

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef STM32F411xE
#include "gpioDB.h"
#include "registerParams.h"
#endif

#define MSEC_MULTIPLER 1000
#define HZ_MULTIPLIER 100
#define RED_LED_OFF_100Hz 100
#define CONFIG_ERROR_RLED_FREQ_100Hz 200
#define NETWORK_ERROR_RLED_FREQ_100Hz 100
#define HARDWARE_ERROR_RLED_FREQ_100Hz 50
#define RLED_ON_DUTY_PERC 50
#define RLED_OFF_DUTY_PERC 0

typedef enum { HARDWARE_ERR = (1 << 0), CONFIG_ERR = (1 << 1), NETWORK_ERR = (1 << 2) } ERROR_BITS;

ERROR_BITS errorList;
bool errorPeripheral[PER_MAX] = {false};
bool networkError = false;
bool configurationError = false;
uint32_t rledFrequency_100Hz = 0;

#define TASK_DELAY(FREQx100Hz)                                                                                         \
    FREQx100Hz ? ((MSEC_MULTIPLER * HZ_MULTIPLIER) / FREQx100Hz)                                                       \
               : ((MSEC_MULTIPLER * HZ_MULTIPLIER) / RED_LED_OFF_100Hz)
#ifdef STM32F411xE

/**
 * @fn redLedThread
 *
 * @brief RED LED control thread.
 *
 * @param[in] arg, unused
 *
 * @return 0 osStatus
 **/
static void redLedThread(const void *arg);

#endif

/**
 * @fn testForPeripheralError
 *
 * @brief iterate over peripheral error table, return true if any peripherals
 *        are in error, else false
 *
 * @return return true if any peripherals are in error, else false
 **/
bool testForPeripheralError(void) {
    for (int i = 0; i < PER_MAX; i++) {
        if (errorPeripheral[i]) {
            return true;
        }
    }
    return false;
}

/**
 * @fn  peripheralErrorPrint
 *
 * @brief Display on CLI each failed peripheral that has failed
 *
 * @param hCli: pointer to display CLI
 *
 **/
void peripheralErrorPrint(CLI *hCli) {
    for (int i = 0; i < PER_MAX; i++) {
        if (errorPeripheral[i]) {
            CliPrintf(hCli, "%s\r\n", PERIPHERAL_e_Strings[i]);
        }
    }
}

void peripheralErrorAppend(char *buf, uint32_t maxSz) {
    char *nxt = buf;
    int tmp;
    for (int i = 0; i < PER_MAX; i++) {
        if (errorPeripheral[i]) {
            tmp = snprintf(nxt, maxSz - (nxt - buf), "%s\r\n", PERIPHERAL_e_Strings[i]);
            SNPRINTF_TEST_AND_ADD(tmp,nxt, return;);
        }
    }
}

/**
 * @fn handleRedLedPriority
 *
 * @brief iterate over list of errors and set the RED led to the proper blinking rate
 *
 * @param testRegState: read the state of the test reg and if it is set then leave the
 *                      the red led in its current state.
 **/

void handleRedLedPriority(bool testRegState) {
#ifdef STM32H743xx
    if (testRegState) {
        registerInfo_t regInfo = {.mbId = RED_LED_ON, .type = DATA_UINT};
        registerRead(&regInfo);
        if (regInfo.u.dataUint) {
            return;
        }
    }
    if (testForPeripheralError()) {
        pwmSetFrequency(LED_RED, HARDWARE_ERROR_RLED_FREQ_100Hz / HZ_MULTIPLIER);
        pwmSetDutyCycle(LED_RED, RLED_ON_DUTY_PERC);
        pwmStart(LED_RED);
    } else if (configurationError) {
        pwmSetFrequency(LED_RED, CONFIG_ERROR_RLED_FREQ_100Hz / HZ_MULTIPLIER);
        pwmSetDutyCycle(LED_RED, RLED_ON_DUTY_PERC);
        pwmStart(LED_RED);
    } else if (networkError) {
        pwmSetFrequency(LED_RED, NETWORK_ERROR_RLED_FREQ_100Hz / HZ_MULTIPLIER);
        pwmSetDutyCycle(LED_RED, RLED_ON_DUTY_PERC);
        pwmStart(LED_RED);
    } else { // No Error, set duty cycle to 0 to turn off.
        pwmSetDutyCycle(LED_RED, RLED_OFF_DUTY_PERC);
    }
#else
    // STM32F411 Sensor board.
    if (testForPeripheralError()) {
        rledFrequency_100Hz = HARDWARE_ERROR_RLED_FREQ_100Hz;
    } else { // No error, set frequency to 0 so it is turned off by task.
        rledFrequency_100Hz = 0;
    }
#endif
}

void hardwareFailure(PERIPHERAL_e peripheral) {
    errorPeripheral[peripheral] = true;
    handleRedLedPriority(true);
}

#ifdef STM32H743xx
void configurationErrorRaise(const char *msg) {
    configurationError = true;
    handleRedLedPriority(true);
}

void configurationErrorClear(void) {
    configurationError = false;
    handleRedLedPriority(true);
}

void networkErrorRaise(const char *msg) {
    networkError = true;
    handleRedLedPriority(true);
}
void networkErrorClear(void) {
    networkError = false;
    handleRedLedPriority(true);
}
void errorListJson(json_object *json) {
}

#else
// PD7 Red Led is only a GPIO pin, No timers available, must control with a task.

void redLedInit(int priority, int stackSize) {
    osThreadDef(redLedTask, redLedThread, priority, 0, stackSize);
    osThreadCreate(osThread(redLedTask), NULL);
}

static void redLedThread(const void *arg) {
    uint32_t prevRledFreq_100Hz = rledFrequency_100Hz;
    uint32_t delay_ticks = TASK_DELAY(RED_LED_OFF_100Hz);
    uint32_t previousWakeTime = 0;
    registerInfo_t redLedReg = {.sbId = SB_RED_LED_ON, .type = DATA_UINT};
    bool prevRedLedOn = false;
    while (1) {
        osDelayUntil(&previousWakeTime, delay_ticks);

        registerRead(&redLedReg);
        if (redLedReg.u.dataUint || prevRedLedOn) {
            gpioSetOutput(LED_RED, redLedReg.u.dataUint ? GPIO_STATE_HIGH : GPIO_STATE_LOW);
            prevRedLedOn = redLedReg.u.dataUint;
            continue;
        }

        if (prevRledFreq_100Hz == rledFrequency_100Hz) {
            if (rledFrequency_100Hz != 0) {
                gpioToggle(LED_RED);
            }
        } else {
            prevRledFreq_100Hz = rledFrequency_100Hz;
            gpioSetOutput(LED_RED, rledFrequency_100Hz ? GPIO_STATE_HIGH : GPIO_STATE_LOW);
            delay_ticks = TASK_DELAY(rledFrequency_100Hz);
        }
    }
}

#endif

#ifdef STM32H743xx
int16_t cliHandler_raiseIssue(CLI *hCli, int argc, char *argv[]) {

    if (argc < 1) {
        CliWriteString(hCli, "Failed to parse command\r\n");
        return CLI_RESULT_ERROR;
    }

    bool noErrors = true;

    if (testForPeripheralError()) {
        CliPrintf(hCli, "Peripheral Error Present\r\n");
        noErrors = false;
    }

    if (networkError) {
        CliPrintf(hCli, "Network Error Present\r\n");
        noErrors = false;
    }
    if (configurationError) {
        CliPrintf(hCli, "Configuration Error Present\r\n");
        noErrors = false;
    }

    if (noErrors) {
        CliPrintf(hCli, "No Errors\r\n");
    }
    if ((argc >= 2) && (strcmp(argv[1], "detail") == 0)) {
        if (configurationError) {
            cliDetailConfigurationError(hCli);
        }
    }

    return CLI_RESULT_SUCCESS;
}

void webAddSystemStatus(json_object *jsonObjectResponse) {
    json_object *jsonObjectBool = json_object_new_boolean(testForPeripheralError());
    json_object_object_add_ex(jsonObjectResponse, "Peripheral Errors", jsonObjectBool, JSON_C_OBJECT_KEY_IS_CONSTANT);
    jsonObjectBool = json_object_new_boolean(networkError);
    json_object_object_add_ex(jsonObjectResponse, "Network Errors", jsonObjectBool, JSON_C_OBJECT_KEY_IS_CONSTANT);
    jsonObjectBool = json_object_new_boolean(configurationError);
    json_object_object_add_ex(
        jsonObjectResponse, "Configuration Errors", jsonObjectBool, JSON_C_OBJECT_KEY_IS_CONSTANT);
    if (configurationError) {
        json_object *jsonArray = json_object_new_array();
        jsonDetailConfigurationError(jsonArray);
        json_object_object_add_ex(jsonObjectResponse, "Config Details", jsonArray, JSON_C_OBJECT_KEY_IS_CONSTANT);
    }
}

#else
int16_t cliHandler_raiseIssue(CLI *hCli, int argc, char *argv[]) {

    if (argc < 1) {
        CliWriteString(hCli, "Failed to parse command\r\n");
        return CLI_RESULT_ERROR;
    }

    bool noErrors = true;

    if (testForPeripheralError()) {
        CliPrintf(hCli, "Peripheral Error Present\r\n");
        peripheralErrorPrint(hCli);
        noErrors = false;
    }

    if (noErrors) {
        CliPrintf(hCli, "No Errors\r\n");
    }

    return CLI_RESULT_SUCCESS;
}

#endif

RETURN_CODE mapPeriperalError(const registerInfo_tp regInfo) {
    assert(PER_MAX < U32_BITS);
    regInfo->type = DATA_UINT;
    regInfo->u.dataUint = 0;

    for (int i = 0; i < PER_MAX; i++) {
        if (errorPeripheral[i]) {
            regInfo->u.dataUint |= (1 << i);
        }
    }

    return RETURN_OK;
}
