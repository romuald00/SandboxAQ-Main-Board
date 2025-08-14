/*
 * board_registerParams.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 5, 2023
 *      Author: rlegault
 */

#include "board_registerParams.h"
#include "MB_gatherTask.h"
#include "dbTriggerTask.h"
#include "debugPrint.h"
#include "pwm.h"
#include "raiseIssue.h"
#include "registerParams.h"
#include "version.h"

#define LED_PWM_ALWAYS_ON 100
#define LED_PWM_BLINK_50 50
#define UNKNOWN_RESULT 0xFF

/**
 * @fn streamIntervalWrite
 *
 * @brief Set the streaming interval
 *
 * @param[in] regInfo contains the desired interval value
 *
 * @return RETURN_OK on success
 **/
RETURN_CODE streamIntervalWrite(const registerInfo_tp regInfo);

/**
 * @fn dbSpiInterval
 *
 * @brief Set the daughter board trigger intervale
 *
 * @param[in] regInfo contains the desired interval value
 *
 * @return RETURN_OK on success
 **/
RETURN_CODE dbSpiInterval(const registerInfo_tp regInfo);

/**
 * @fn greenLedStateChange
 *
 * @brief Set the green led to either blinking or always on
 *
 * @param[in] regInfo contains the desired state
 *
 * @return RETURN_OK on success
 **/
static RETURN_CODE greenLedStateChange(const registerInfo_tp regInfo);

/**
 * @fn redLedStateChange
 *
 * @brief Set the red led to either blinking or always on
 *
 * @param[in] regInfo contains the desired state
 *
 * @return RETURNO_OK on success
 **/
static RETURN_CODE redLedStateChange(const registerInfo_tp regInfo);

// search this and then boardParamStorage for registers
paramStorage_t paramStorage =
    {.mutex = NULL,
     .reg = {
         [REG_FIRST] = {.info = {.mbId = REG_FIRST, .type = DATA_UINT, .size = sizeof(uint32_t), .u.dataUint = 0x5a5a},
                        .name = "DUMMY_FIRST",
                        .writePtr = noWriteFn,
                        .readPtr = NULL},

         [FW_VERSION_MAJ] = {.info = {.mbId = FW_VERSION_MAJ,
                                      .type = DATA_UINT,
                                      .size = sizeof(uint32_t),
                                      .u.dataUint = MACRO_FW_VER_MAJ},
                             .name = "FW_VERSION_MAJ",
                             .writePtr = noWriteFn},
         [FW_VERSION_MIN] = {.info = {.mbId = FW_VERSION_MIN,
                                      .type = DATA_UINT,
                                      .size = sizeof(uint32_t),
                                      .u.dataUint = MACRO_FW_VER_MIN},
                             .name = "FW_VERSION_MIN",
                             .writePtr = noWriteFn},
         [FW_VERSION_MAINT] = {.info = {.mbId = FW_VERSION_MAINT,
                                        .type = DATA_UINT,
                                        .size = sizeof(uint32_t),
                                        .u.dataUint = MACRO_FW_VER_MAINT},
                               .name = "FW_VERSION_MAINT",
                               .writePtr = noWriteFn},
         [FW_VERSION_BUILD] = {.info = {.mbId = FW_VERSION_BUILD,
                                        .type = DATA_UINT,
                                        .size = sizeof(uint32_t),
                                        .u.dataUint = MACRO_FW_VER_BUILD},
                               .name = "FW_VERSION_BUILD",
                               .writePtr = noWriteFn},
         EEPROM_PARAM_ELEMENT(HW_TYPE, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(HW_VERSION, DATA_UINT, sizeof(uint32_t)),

         [REBOOT_FLAG] = {.info = {.mbId = REBOOT_FLAG, .type = DATA_UINT, .size = sizeof(uint32_t), .u.dataUint = 0},
                          .name = "REBOOT_FLAG",
                          .writePtr = updateResetFlag},
         [REBOOT_DELAY_MS] =
             {.info = {.mbId = REBOOT_DELAY_MS, .type = DATA_UINT, .size = sizeof(uint32_t), .u.dataUint = 0},
              .name = "REBOOT_DELAY_MS",
              .writePtr = updateResetDelay},
         [UPTIME_SEC] = {.info = {.mbId = UPTIME_SEC, .type = DATA_UINT, .size = sizeof(uint32_t), .u.dataUint = 0},
                         .name = "UPTIME",
                         .writePtr = noWriteFn,
                         .readPtr = getUptimeAsRegister},
         [ADC_READ_RATE] = {.info = {.mbId = ADC_READ_RATE,
                                     .type = DATA_UINT,
                                     .size = sizeof(uint32_t),
                                     .u.dataUint = ADC_READ_RATE_x100Hz}, /* divide by 100 to get value in Hz */
                            .name = "ADC_READ_RATE",
                            .writePtr = adcSetPwmRate},
         [ADC_READ_DUTY] =
             {.info = {.mbId = ADC_READ_DUTY, .type = DATA_UINT, .size = sizeof(uint32_t), .u.dataUint = 50},
              .name = "ADC_READ_DUTY",
              .writePtr = adcSetPwmDuty},
         EEPROM_PARAM_ELEMENT(SERIAL_NUMBER, DATA_STRING, 16),
         EEPROM_PARAM_ELEMENT(IP_ADDR, DATA_UINT, sizeof(uint32_t)),

         EEPROM_PARAM_ELEMENT(HTTP_PORT, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(UDP_TX_PORT, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(GATEWAY, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(NETMASK, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(UDP_SERVER_IP, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(UDP_SERVER_PORT, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(DEPRECATED_TCP_CLIENT_IP, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(TCP_CLIENT_PORT, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(IP_TX_DATA_TYPE, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(UNIQUE_ID, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(ADC_MODE, DATA_UINT, sizeof(uint32_t)),

         [STREAM_INTERVAL_US] = {.info = {.mbId = STREAM_INTERVAL_US,
                                          .type = DATA_UINT,
                                          .size = sizeof(uint32_t),
                                          .u.dataUint = VALUE_STREAM_INTERVAL_US},
                                 .name = "STREAM_INTERVAL_US",
                                 .writePtr = streamIntervalWrite},
         [DB_SPI_INTERVAL_US] = {.info = {.mbId = DB_SPI_INTERVAL_US,
                                          .type = DATA_UINT,
                                          .size = sizeof(uint32_t),
                                          .u.dataUint = VALUE_DB_SPI_INTERVAL_US},
                                 .name = "DB_SPI_INTERVAL_US",
                                 .writePtr = dbSpiInterval},
         [DB_RETRY_INTERVAL_S] = {.info = {.mbId = DB_RETRY_INTERVAL_S,
                                           .type = DATA_UINT,
                                           .size = sizeof(uint32_t),
                                           .u.dataUint = VALUE_DB_RETRY_INTERVAL_S},
                                  .name = "DB_RETRY_INTERVAL_S"},
         [TRIGGER_MASK_0] =
             {.info = {.mbId = TRIGGER_MASK_0, .type = DATA_UINT, .size = sizeof(uint32_t), .u.dataUint = 0},
              .name = "TRIGGER_MASK_0",
              .readPtr = dbTriggerMaskRead,
              .writePtr = dbTriggerMaskWrite},
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_0, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_1, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_2, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_3, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_4, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_5, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_6, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_7, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_8, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_9, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_10, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_11, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_12, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_13, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_14, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_15, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_16, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_17, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_18, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_19, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_20, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_21, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_22, DATA_UINT, sizeof(uint32_t)),
         EEPROM_PARAM_ELEMENT(SENSOR_BOARD_23, DATA_UINT, sizeof(uint32_t)),
         [MFG_WRITE_EN] = {.info = {.mbId = MFG_WRITE_EN, .type = DATA_UINT, .size = sizeof(uint32_t), .u.dataUint = 0},
                           .name = "MFG_WRITE_EN"},
         [DDS_CLK_RATE] = {.info = {.mbId = DDS_CLK_RATE,
                                    .type = DATA_UINT,
                                    .size = sizeof(uint32_t),
                                    .u.dataUint = DDS_CLOCK_RATE_x100Hz}, /* divide by 100 to get value in Hz */
                           .name = "DDS_CLK_RATE",
                           .writePtr = ddsSetPwmRate},
         [DDS_CLK_DUTY] =
             {.info = {.mbId = DDS_CLK_DUTY, .type = DATA_UINT, .size = sizeof(uint32_t), .u.dataUint = DUTY_50_PERC},
              .name = "DDS_CLK_DUTY",
              .writePtr = ddsSetPwmDuty},
         [GREEN_LED_ON] = {.info = {.mbId = GREEN_LED_ON, .type = DATA_UINT, .size = sizeof(uint32_t), .u.dataUint = 0},
                           .name = "GREEN_LED_ON",
                           .writePtr = greenLedStateChange},
         [RED_LED_ON] = {.info = {.mbId = RED_LED_ON, .type = DATA_UINT, .size = sizeof(uint32_t), .u.dataUint = 0},
                         .name = "RED_LED_ON",
                         .writePtr = redLedStateChange},
         [PERIPHERAL_FAIL_MASK] = {.info = {.sbId = PERIPHERAL_FAIL_MASK, .type = DATA_UINT, .u.dataUint = 0},
                                   .name = "PERIPHERAL_FAIL_MASK",
                                   .readPtr = mapPeriperalError,
                                   .writePtr = noWriteFn},
         EEPROM_PARAM_ELEMENT(FAN_POP, DATA_UINT, sizeof(uint32_t)),
     }};

RETURN_CODE streamIntervalWrite(const registerInfo_tp regInfo) {
    assert(regInfo != NULL);
    setUdpSendInterval(regInfo->u.dataUint);
    registerWriteForce(regInfo);
    return RETURN_OK;
}

RETURN_CODE dbSpiInterval(const registerInfo_tp regInfo) {
    assert(regInfo != NULL);
    setDbTriggerInterval(regInfo->u.dataUint);
    registerWriteForce(regInfo);
    return RETURN_OK;
}

RETURN_CODE greenLedStateChange(const registerInfo_tp regInfo) {
    if (regInfo->u.dataUint) {
        pwmSetDutyCycle(LED_GREEN, LED_PWM_ALWAYS_ON);
    } else {
        pwmSetDutyCycle(LED_GREEN, LED_PWM_BLINK_50);
    }
    registerWriteForce(regInfo);
    return RETURN_OK;
}

RETURN_CODE redLedStateChange(const registerInfo_tp regInfo) {
    registerWriteForce(regInfo);
    if (regInfo->u.dataUint) {
        pwmSetDutyCycle(LED_RED, LED_PWM_ALWAYS_ON);
    } else {
        pwmSetDutyCycle(LED_RED, LED_PWM_BLINK_50);
        handleRedLedPriority(true);
    }
    return RETURN_OK;
}
