/*
 * fanCtrl.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Jan 13, 2025
 *      Author: rlegault
 */

#include "fanCtrl.h"

#include "cli/cli_print.h"
#include "cli_task.h"
#include "cmdAndCtrl.h"
#include "debugPrint.h"
#include "registerParams.h"
#include "saqTarget.h"
#include "stmTarget.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c2;

#define CLI_BUFFER_BYTES 100
#define DEFAULT_FAN_SPEED_PERC 44.0

typedef struct {
    uint8_t msb;
    uint8_t lsb;
} tachRegisterAddr_t;

static const tachRegisterAddr_t tachRegisterArray[MAX_FAN_ID] = {
    [FAN1].msb = 0x52, [FAN1].lsb = 0x53, [FAN2].msb = 0x54, [FAN2].lsb = 0x55};
#define I2C_DELAY_MS 100

#define FANCTRL_I2C_ADDR 0xA0

#define MAX31760_CTRL1_TEMP_SRC 0x01
#define MAX31760_CTRL1_PWM_FREQ 0x18 // DRV=11

#define MAX31760_CTRL2_MANUAL_BIT 0x01
#define MAX31760_CTRL2_FFCOMPARATOR 0x10

#define MAX31760_CTRL3_TACH1_ALARM_EN 0x01
#define MAX31760_CTRL3_TACH2_ALARM_EN 0x02

// Temperature Defines
#define TEMPERATURE_MSB_SIGN_BIT 0x80
// temperature only have a resolution of 1/8
#define TEMPERATURE_LSB_SHIFT 5              // only the top 3 bits are used right shift by 5
#define TEMPERATURE_LSB_MULTIPLIER (1.0 / 8) // Since only 3 bits are used divide by 1=0.125, 2=0.25 ... 7=.875

// PWM Defines
#define MAX_PERC 100.0

// Tachometer Defines
#define SECS_IN_A_MINUTE 60
#define TACH_FREQ_RATE_HZ 100000
#define MOVE_TO_MSB(x) (x << 8)
#define FAN_PULSE_PER_REVOLUTION 2

// CLI Defines
#define CMD_IDX 1
#define SUBCMD_IDX 2
#define CMD_PARA_IDX(x) (CMD_IDX + 1 + x)
#define CMD_PARA_CNT(x) (CMD_PARA_IDX(x))

#define TEST_MATCH_CMD(cmdCnt, cmdStr) ((argc == cmdCnt) && (strcmp(argv[CMD_IDX], cmdStr) == 0))

static osMutexId fanCtrlMutexId = NULL;
static uint32_t cliResult = CLI_RESULT_SUCCESS;

/**
 * fanCliReadCallback
 *
 * @brief Call back function when Read Cli command is complete by CNC
 *
 * @param cmd: spi command
 * @param cbId: contains hCli to know where to print the result
 * @param xInfo: user defined value
 * @param cmdResponse: Not relevant on reads
 * @param p_cncMsg: if non short SPI command this will contain the values that were read
 *
 * @return osStatus of fan read command
 */
static osStatus fanCliReadCallback(
    spiDbMbCmd_e cmd, uint32_t cbId, uint8_t xInfo, spiDbMbPacketCmdResponse_t cmdResponse, cncPayload_tp p_cncMsg);

/**
 * fanCliWriteCallback
 *
 * @brief Call back function when Write Cli command is complete by CNC
 *
 * @param cmd: spi command
 * @param cbId: contains hCli to know where to print the result
 * @param xInfo: user defined value
 * @param cmdResponse: Not relevant on cli commands
 * @param p_cncMsg: if non short SPI command this will contain the values that were read
 *
 * @return osStatus of fan write command
 */
static osStatus fanCliWriteCallback(
    spiDbMbCmd_e cmd, uint32_t cbId, uint8_t xInfo, spiDbMbPacketCmdResponse_t cmdResponse, cncPayload_tp p_cncMsg);

/**
 * fanCtrlSetFanSpeed
 *
 * @brief Set the fan speed via PERC value 0-100
 *
 * @param speedPerc: value to set fan speed to
 *
 * @return HAL_OK if write was successful
 */
static HAL_StatusTypeDef fanCtrlSetFanSpeed(float speedPerc);

/**
 * fanCtrlSetManual
 *
 * @brief Set whether the fan speed is under manual control (1) or chip control (0)
 *
 * @param manual: value to set manual setting to
 *
 * @return HAL_OK if write was successful
 */
static HAL_StatusTypeDef fanCtrlSetManual(bool manual);

/**
 * fanCtrlWriteRegister
 *
 * @brief Write VALUE to the specified ADDR. Will write 1 byte value
 *
 * @param addr: address to write value to to
 * @param value: pointer to uin8_t to write to addr
 *
 * @return HAL_OK if write was successful
 */
static HAL_StatusTypeDef fanCtrlWriteRegister(uint8_t addr, uint8_t *value);

void fanCtrlInit(void) {
    // initialize mutex semaphore
    osMutexDef(fanCtrlMutex);
    fanCtrlMutexId = osMutexCreate(osMutex(fanCtrlMutex));
    assert(fanCtrlMutexId != NULL);

    uint8_t regSetting = MAX31760_CTRL1_PWM_FREQ | MAX31760_CTRL1_TEMP_SRC;
    fanCtrlWriteRegister(MAX31760_CTRL1_ADDR, &regSetting);

    regSetting = MAX31760_CTRL2_FFCOMPARATOR | MAX31760_CTRL2_MANUAL_BIT;
    fanCtrlWriteRegister(MAX31760_CTRL2_ADDR, &regSetting);

    regSetting = 0; // Disable TACH1 & TACH2 Alarms
    registerInfo_t fanPop = {.mbId = FAN_POP, .type = DATA_UINT};
    RETURN_CODE rc = registerRead(&fanPop);
    if (rc != RETURN_OK) {
        regSetting = MAX31760_CTRL3_TACH1_ALARM_EN;
    } else {
        if (fanPop.u.dataUint == UINT_MAX) {
            // because this was added after systems were programmed, need to update it once.
            // Assume value of 0xFFFFFFFF needs updating.
            // Default is for FAN1 to be populated and FAN2 to be depopulated
            fanPop.u.dataUint = (1 >> FAN1);
            registerWrite(&fanPop);
        }
        if (fanPop.u.dataUint & (1 >> FAN1)) {
            regSetting |= MAX31760_CTRL3_TACH1_ALARM_EN;
        }
        if (fanPop.u.dataUint & (1 >> FAN2)) {
            regSetting |= MAX31760_CTRL3_TACH2_ALARM_EN;
        }
    }
    fanCtrlWriteRegister(MAX31760_CTRL3_ADDR, &regSetting);

    fanCtrlSetManual(true);
    fanCtrlSetFanSpeed(DEFAULT_FAN_SPEED_PERC);
}

osStatus fanCtrlOpen(uint32_t wait_ms) {
    // get semaphore
    return osMutexWait(fanCtrlMutexId, wait_ms);
}

osStatus fanCtrlClose(void) {
    // release semaphore
    return osMutexRelease(fanCtrlMutexId);
}

HAL_StatusTypeDef fanCtrlReadRegister(uint8_t addr, uint8_t *value) {
    HAL_StatusTypeDef rc =
        HAL_I2C_Mem_Read(&hi2c2, FANCTRL_I2C_ADDR, addr, sizeof(uint8_t), value, sizeof(uint8_t), I2C_DELAY_MS);
    return rc;
}

HAL_StatusTypeDef fanCtrlWriteRegister(uint8_t addr, uint8_t *value) {
    HAL_StatusTypeDef rc =
        HAL_I2C_Mem_Write(&hi2c2, FANCTRL_I2C_ADDR, addr, sizeof(uint8_t), value, sizeof(uint8_t), I2C_DELAY_MS);
    return rc;
}

HAL_StatusTypeDef fanCtrlSetManual(bool manual) {
    uint8_t cfg2;
    HAL_StatusTypeDef rc = fanCtrlReadRegister(MAX31760_CTRL2_ADDR, &cfg2);
    if (rc == HAL_OK) {
        if (manual) {
            cfg2 |= MAX31760_CTRL2_MANUAL_BIT;
        } else {
            cfg2 &= (~MAX31760_CTRL2_MANUAL_BIT);
        }
        rc = fanCtrlWriteRegister(MAX31760_CTRL2_ADDR, &cfg2);
    }
    return rc;
}

HAL_StatusTypeDef fanCtrlGetManual(bool *manual) {
    uint8_t cfg2;
    HAL_StatusTypeDef rc = fanCtrlReadRegister(MAX31760_CTRL2_ADDR, &cfg2);
    if (manual == NULL) {
        return HAL_ERROR;
    }
    if (rc == HAL_OK) {
        *manual = (cfg2 & MAX31760_CTRL2_MANUAL_BIT);
    }
    return rc;
}

HAL_StatusTypeDef fanCtrlGetTemperature(float *temperature) {
    HAL_StatusTypeDef rc;
    uint8_t msb;
    uint8_t lsb;
    if (temperature == NULL) {
        return HAL_ERROR;
    }
    *temperature = NAN;

    if (HAL_OK != (rc = fanCtrlReadRegister(MAX31760_TEMPERATURE_MSB_REGADDR, &msb))) {
        DPRINTF_ERROR("failed to read temperature MSB register, rc=%d\r\n", rc);
    } else if (HAL_OK != (rc = fanCtrlReadRegister(MAX31760_TEMPERATURE_LSB_REGADDR, &lsb))) {
        DPRINTF_ERROR("failed to read temperature LSB register, rc=%d\r\n", rc);
    } else {
        *temperature = (msb & (~TEMPERATURE_MSB_SIGN_BIT));
        float temp = (lsb >> (TEMPERATURE_LSB_SHIFT));
        temp *= (TEMPERATURE_LSB_MULTIPLIER);
        *temperature += temp;
        // if sign bit is set then multiply by -1
        if (msb & TEMPERATURE_MSB_SIGN_BIT) {
            *temperature *= (-1.0);
        }
    }
    return rc;
}

HAL_StatusTypeDef fanCtrlSetFanSpeed(float speedPerc) {
    HAL_StatusTypeDef rc;
    uint8_t regValue = round((speedPerc * UCHAR_MAX) / MAX_PERC);
    if (HAL_OK != (rc = fanCtrlWriteRegister(MAX31760_FANCTRL_PWM_REGADDR, &regValue))) {
        DPRINTF_ERROR("fanCtrl Write register failed with rc=%d\r\n", rc);
    }
    if (HAL_OK != (rc = fanCtrlWriteRegister(MAX31760_FANCTRL_PWM_REGADDR + 1, &regValue))) {
        DPRINTF_ERROR("fanCtrl Write register failed with rc=%d\r\n", rc);
    }
    return rc;
}

HAL_StatusTypeDef fanCtrlGetFanSpeed(uint8_t *speedPerc) {
    uint8_t regValue;
    HAL_StatusTypeDef rc;
    if (HAL_OK != (rc = fanCtrlReadRegister(MAX31760_FANCTRL_PWM_REGADDR, &regValue))) {
        DPRINTF_ERROR("fanCtrl Write register failed with rc=%d\r\n", rc);
    } else {
        *speedPerc = round(regValue * MAX_PERC / UCHAR_MAX);
    }
    return rc;
}

HAL_StatusTypeDef fanCtrlGetTachSpeed(FAN_ID_e fanId, float *speed_rpm) {
    assert(fanId < MAX_FAN_ID);
    HAL_StatusTypeDef rc;
    uint8_t msb, lsb;
    *speed_rpm = NAN;

    if (HAL_OK != (rc = fanCtrlReadRegister(tachRegisterArray[fanId].msb, &msb))) {
        DPRINTF_ERROR("failed to read fan %d tach MSB register, rc=%d\r\n", fanId, rc);
    } else if (HAL_OK != (rc = fanCtrlReadRegister(tachRegisterArray[fanId].lsb, &lsb))) {
        DPRINTF_ERROR("failed to read fan %d tach LSB register, rc=%d\r\n", fanId, rc);
    } else {

        uint16_t pulseCnt = MOVE_TO_MSB(msb) | lsb;
        *speed_rpm = 1.0 * SECS_IN_A_MINUTE * TACH_FREQ_RATE_HZ / (pulseCnt) / FAN_PULSE_PER_REVOLUTION;
    }
    return rc;
}

osStatus fanCtrlReadHelper(uint8_t addr, fanCtrlValue_tp p_value) {
    HAL_StatusTypeDef rc;
    if (addr <= MAX31760_MAX_REGADDR) {
        p_value->type = VALUETYPE_UINT8;
        p_value->u32Value = 0;
        rc = fanCtrlReadRegister(addr, &(p_value->u8Value));
        if (rc == HAL_OK)
            return osOK;
        DPRINTF_ERROR("read of addr 0x%x rc=%d\r\n", addr, rc);
        return osErrorOS;
    } else {
        switch (addr) {
        case FANCTRL_MANUAL_REGADDR:
            p_value->type = VALUETYPE_BOOL;
            rc = fanCtrlGetManual(&p_value->boolean);

            if (rc == HAL_OK)
                return osOK;
            DPRINTF_ERROR("read of Manual setting failed rc=%d\r\n", rc);
            return osErrorOS;
            break;
        case FANCTRL_TEMPERATURE_REGADDR:
            p_value->type = VALUETYPE_FLOAT;
            rc = fanCtrlGetTemperature(&p_value->flValue);
            if (rc == HAL_OK)
                return osOK;
            DPRINTF_ERROR("read of temperature failed rc=%d\r\n", rc);
            return osErrorOS;
            break;
        case FANCTRL_TACHOMETER_1_REGADDR:
            // fall through
        case FANCTRL_TACHOMETER_2_REGADDR:
            FAN_ID_e fanId = addr - FANCTRL_TACHOMETER_1_REGADDR;
            assert(fanId < MAX_FAN_ID);

            p_value->type = VALUETYPE_FLOAT;
            rc = fanCtrlGetTachSpeed(fanId, &p_value->flValue);

            if (rc == HAL_OK)
                return osOK;
            DPRINTF_ERROR("read of Tachometer fan %d failed rc=%d\r\n", fanId, rc);
            return osErrorOS;

            break;
        case FANCTRL_SPEEDCTRL_REGADDR:
            p_value->u32Value = 0;
            p_value->type = VALUETYPE_UINT8;
            rc = fanCtrlGetFanSpeed(&p_value->u8Value);

            if (rc == HAL_OK)
                return osOK;
            DPRINTF_ERROR("read of Speed Ctrl failed rc=%d\r\n", rc);
            return osErrorOS;

            break;
        }
    }
    return osErrorOS;
}

osStatus fanCtrlWriteHelper(uint8_t addr, fanCtrlValue_tp p_value) {
    HAL_StatusTypeDef rc = osErrorOS;
    if (addr <= MAX31760_MAX_REGADDR) {
        p_value->type = VALUETYPE_UINT8;
        rc = fanCtrlWriteRegister(addr, &(p_value->u8Value));
        if (rc == HAL_OK)
            return osOK;
        DPRINTF_ERROR("read of addr 0x%x rc=%d\r\n", addr, rc);
        return osErrorOS;
    } else {
        switch (addr) {
        case FANCTRL_MANUAL_REGADDR:
            p_value->type = VALUETYPE_BOOL;
            rc = fanCtrlSetManual(p_value->boolean);

            if (rc == HAL_OK)
                return osOK;
            DPRINTF_ERROR("write of Manual setting failed rc=%d\r\n", rc);
            return osErrorOS;
            break;
        case FANCTRL_TEMPERATURE_REGADDR:
            DPRINTF_ERROR("Temperature is read only failed rc=%d\r\n", rc);
            return osErrorOS;
            break;
        case FANCTRL_TACHOMETER_1_REGADDR:
            // fall through
        case FANCTRL_TACHOMETER_2_REGADDR:
            DPRINTF_ERROR("Tachometer is read only failed rc=%d\r\n", rc);
            return osErrorOS;
            break;
        case FANCTRL_SPEEDCTRL_REGADDR:

            rc = fanCtrlSetFanSpeed(p_value->u8Value);

            if (rc == HAL_OK)
                return osOK;
            DPRINTF_ERROR("write of Speed Ctrl failed rc=%d\r\n", rc);
            return osErrorOS;

            break;
        }
    }
    return osErrorOS;
}

int16_t fanCtrlCliCmd(CLI *hCli, int argc, char *argv[]) {

    bool wait = false;
    cncCbFnPtr p_cbFn = NULL;
    cncPayload_t cncMsg = {.cmd.cncMsgPayloadHeader.peripheral = PER_FAN};

    if (argc > CMD_IDX) {
        wait = true;
        if (TEST_MATCH_CMD(CMD_PARA_CNT(1), "read")) {
            cncMsg.cmd.cncMsgPayloadHeader.addr = strtol(argv[CMD_PARA_IDX(0)], NULL, STRING_NUMER_BASE(16));
            cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ;
            p_cbFn = fanCliReadCallback;
            if (cncMsg.cmd.cncMsgPayloadHeader.addr > MAX31760_MAX_REGADDR) {
                CliPrintf(hCli, "Addr 0x%x must be <= 0x%x\r\n", cncMsg.cmd.cncMsgPayloadHeader.addr, MAX31760_MAX_REGADDR);
                return CLI_RESULT_ERROR;
            }

        } else if (TEST_MATCH_CMD(CMD_PARA_CNT(2), "write")) {
            cncMsg.cmd.cncMsgPayloadHeader.addr = strtol(argv[CMD_PARA_IDX(0)], NULL, STRING_NUMER_BASE(16));
            cncMsg.cmd.value = strtol(argv[CMD_PARA_IDX(1)], NULL, STRING_NUMER_BASE(16));
            cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_WRITE;
            p_cbFn = fanCliWriteCallback;
            if (cncMsg.cmd.cncMsgPayloadHeader.addr > MAX31760_MAX_REGADDR) {
                CliPrintf(hCli, "Addr 0x%x must be <= 0x%x\r\n", cncMsg.cmd.cncMsgPayloadHeader.addr, MAX31760_MAX_REGADDR);
                return CLI_RESULT_ERROR;
            }
        } else if (TEST_MATCH_CMD(CMD_PARA_CNT(0), "temperature")) {
            cncMsg.cmd.cncMsgPayloadHeader.addr = FANCTRL_TEMPERATURE_REGADDR;
            cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ;
            p_cbFn = fanCliReadCallback;

        } else if (TEST_MATCH_CMD(CMD_PARA_CNT(1), "tach")) {
            uint32_t fanId = strtol(argv[CMD_PARA_IDX(0)], NULL, STRING_NUMER_BASE(16));
            if (fanId > MAX_FAN_ID) {
                CliPrintf(hCli, "Fan Id Value %d is out of range\r\n", fanId);
                return CLI_RESULT_ERROR;
            }
            cncMsg.cmd.cncMsgPayloadHeader.addr = FANCTRL_TACHOMETER_1_REGADDR + fanId;
            cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ;
            p_cbFn = fanCliReadCallback;

        } else if (TEST_MATCH_CMD(CMD_PARA_CNT(0), "getSpeed")) {
            cncMsg.cmd.cncMsgPayloadHeader.addr = FANCTRL_SPEEDCTRL_REGADDR;
            cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ;
            p_cbFn = fanCliReadCallback;

        } else if (TEST_MATCH_CMD(CMD_PARA_CNT(0), "getManual")) {
            cncMsg.cmd.cncMsgPayloadHeader.addr = FANCTRL_MANUAL_REGADDR;
            cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ;
            p_cbFn = fanCliReadCallback;
        } else if (TEST_MATCH_CMD(CMD_PARA_CNT(1), "setSpeed")) {
            cncMsg.cmd.cncMsgPayloadHeader.addr = FANCTRL_SPEEDCTRL_REGADDR;
            cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_WRITE;
            cncMsg.cmd.value = strtol(argv[CMD_PARA_IDX(0)], NULL, STRING_NUMER_BASE(10));
            p_cbFn = fanCliWriteCallback;

        } else if (TEST_MATCH_CMD(CMD_PARA_CNT(1), "setManual")) {
            cncMsg.cmd.cncMsgPayloadHeader.addr = FANCTRL_MANUAL_REGADDR;
            cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_WRITE;
            cncMsg.cmd.value = strtol(argv[CMD_PARA_IDX(0)], NULL, STRING_NUMER_BASE(10));
            p_cbFn = fanCliWriteCallback;
        }

        if (wait) {
            if (fanCtrlOpen(FANCTRL_WAIT_MS) == osOK) {
                uint8_t xinfo = PER_FAN; // don't care what the value is
                osStatus status = cncSendMsg(MAIN_BOARD_ID, SPICMD_UNUSED, &cncMsg, xinfo, p_cbFn, (uint32_t)hCli);

                if (status != osOK) {
                    fanCtrlClose();
                    DPRINTF_ERROR("fan cli failed in call cncSendMsg\r\n");
                } else {
                    status = stallCliUntilComplete(STALL_CLI_TIMEOUT_MS);
                    fanCtrlClose();
                    if (status == osOK) {
                        return CLI_RESULT_SUCCESS;
                    } else {
                        DPRINTF_ERROR("adc cli command timeout\r\n");
                    }
                }
            } else {
                DPRINTF_ERROR("Failed to get Fan Ctrl MUTEX\r\n");
            }
            return CLI_RESULT_ERROR;
        }
    }
    return cliResult;
}

osStatus fanCliReadCallback(
    spiDbMbCmd_e cmd, uint32_t cbId, uint8_t xInfo, spiDbMbPacketCmdResponse_t cmdResponse, cncPayload_tp p_cncMsg) {
    CLI *hCli = (CLI *)cbId;
    char buffer[CLI_BUFFER_BYTES];
    if (p_cncMsg->cmd.cncMsgPayloadHeader.cncActionData.result == osOK) {
        if(p_cncMsg->cmd.cncMsgPayloadHeader.addr <= MAX31760_MAX_REGADDR) {
            CliPrintf(hCli, "Fan Ctrl Register 0x%x = 0x%x \r\n", p_cncMsg->cmd.cncMsgPayloadHeader.addr, p_cncMsg->cmd.value);
            cliResult = CLI_RESULT_SUCCESS;
        } else {
            cliResult = CLI_RESULT_SUCCESS;
            switch(p_cncMsg->cmd.cncMsgPayloadHeader.addr) {
            case (FANCTRL_MANUAL_REGADDR):
                CliPrintf(hCli, "Fan Manual Setting = %ld\r\n", p_cncMsg->cmd.boolean);
                break;
            case (FANCTRL_TEMPERATURE_REGADDR):
                snprintf(buffer, CLI_BUFFER_BYTES, "Fan Temperature = %f C", p_cncMsg->cmd.fvalue);
                CliPrintf(hCli, "%s\r\n", buffer);
                break;
            case (FANCTRL_TACHOMETER_1_REGADDR):
            case (FANCTRL_TACHOMETER_2_REGADDR):
                snprintf(buffer, CLI_BUFFER_BYTES, "Fan %ld Tachometer = %f RPM", p_cncMsg->cmd.cncMsgPayloadHeader.addr - FANCTRL_TACHOMETER_1_REGADDR,
                        p_cncMsg->cmd.fvalue);
                CliPrintf(hCli, "%s\r\n", buffer);
                break;
            case (FANCTRL_SPEEDCTRL_REGADDR):
                CliPrintf(hCli, "Fan Speed Setting = %ld%%\r\n", p_cncMsg->cmd.value);
                break;
            }
        }
    } else {
        CliPrintf(hCli, "Fan Read Failed\r\n");
        cliResult = CLI_RESULT_ERROR;
    }
    osStatus status = cliCommandComplete();
    if (status != osOK) {
        DPRINTF_ERROR("cliCommand Complete failed with %d\r\n", status);
    }
    return status;
}

osStatus fanCliWriteCallback(
    spiDbMbCmd_e cmd, uint32_t cbId, uint8_t xInfo, spiDbMbPacketCmdResponse_t cmdResponse, cncPayload_tp p_cncMsg) {
    CLI *hcli = (CLI *)cbId;
    if (p_cncMsg->cmd.cncMsgPayloadHeader.cncActionData.result == osOK) {
        CliPrintf(hcli, "Write command completed\r\n");
        cliResult = CLI_RESULT_SUCCESS;
    } else {
        CliPrintf(hcli, "Write command failed %d\r\n", p_cncMsg->cmd.cncMsgPayloadHeader.cncActionData.result);
        cliResult = CLI_RESULT_ERROR;
    }
    osStatus status = cliCommandComplete();
    if (status != osOK) {
        DPRINTF_ERROR("cliCommand Complete failed with %d\r\n", status);
    }
    return status;
}
