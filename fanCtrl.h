/*
 * fanCtrl.h
 *
 *  Copyright Nuvation Research Corporation 2018-2025. All Rights Reserved.
 *  Created on: Jan 14, 2025
 *      Author: rlegault
 */

#ifndef LIB_INC_FANCTRL_H_
#define LIB_INC_FANCTRL_H_

#include "cli/cli.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum { FAN1 = 0, FAN2, MAX_FAN_ID } FAN_ID_e;

typedef enum {
    VALUETYPE_UINT8 = 0,
    VALUETYPE_FLOAT,
    VALUETYPE_BOOL,
    VALUETYPE_UINT32,
} VALUETYPE_e;

typedef struct {
    VALUETYPE_e type;
    union {
        float flValue;
        uint8_t u8Value;
        bool boolean;
        uint32_t u32Value;
    };
} fanCtrlValue_t, *fanCtrlValue_tp;

#define FANCTRL_WAIT_MS 100

#define MAX31760_CTRL1_ADDR 0x00
#define MAX31760_CTRL2_ADDR 0x01
#define MAX31760_CTRL3_ADDR 0x02

#define MAX31760_FANCTRL_PWM_REGADDR 0x50

#define MAX31760_TEMPERATURE_MSB_REGADDR 0x58
#define MAX31760_TEMPERATURE_LSB_REGADDR 0x59

#define MAX31760_MAX_REGADDR 0x5B

#define FANCTRL_MANUAL_REGADDR 0x5C       // Read/Write
#define FANCTRL_TEMPERATURE_REGADDR 0x5D  // READ ONLY
#define FANCTRL_TACHOMETER_1_REGADDR 0x5E // READ ONLY
#define FANCTRL_TACHOMETER_2_REGADDR 0x5F // READ ONLY
#define FANCTRL_SPEEDCTRL_REGADDR 0x60    // Read/Write

/**
 * @brief Initialize the fan library
 */
void fanCtrlInit(void);

/**
 * @brief Get Mutex access
 * @param wait_ms amount of time to wait for mutex
 * @return osOK if successful
 */
osStatus fanCtrlOpen(uint32_t wait_ms);

/**
 * @brief release access
 */
osStatus fanCtrlClose(void);

/**
 * @brief Command Line Interface handler for fan control
 * @param hCli output location
 * @param argc amount of arguments in argv
 * @param argv cli arguments
 * @return 0 if successful
 */
int16_t fanCtrlCliCmd(CLI *hCli, int argc, char *argv[]);

/**
 * @brief Based on address read the appropriate data
 * @param addr register value to read or action to take
 * @param[out] p_value, value read at addr
 * @return osOK if successful
 */
osStatus fanCtrlReadHelper(uint8_t addr, fanCtrlValue_tp p_value);

/**
 * @brief Base on address write the appropriate data to fan ctrl IC
 * @param addr register value to read or action to take
 * @param[in] p_value, value to write to addr
 * @return osOK if successful
 */
osStatus fanCtrlWriteHelper(uint8_t addr, fanCtrlValue_tp p_value);
#endif /* LIB_INC_FANCTRL_H_ */
