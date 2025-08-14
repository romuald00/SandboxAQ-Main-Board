/*
 * registerParams.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 5, 2023
 *      Author: rlegault
 */

#include "registerParams.h"
#include "board_registerParams.h"
#include "cli/cli_print.h"
#include "cmsis_os.h"
#include "debugPrint.h"
#include "eeprom.h"
#include "version.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef STM32F411xE
#include "dbTriggerTask.h"
#endif

#define REGISTER_MUTEX_TIMEOUT 100

extern paramStorage_t paramStorage;

#define PRINT_DELAY_MS 1
#define PRINT_REGISTER_ARRAY 0

#define MAX_PROTECTED_EEPROM_DATA 3

static uint32_t protectedEepromData[MAX_PROTECTED_EEPROM_DATA] = {
    EEPROM_SERIAL_NUMBER, EEPROM_HW_TYPE, EEPROM_HW_VERSION};

RETURN_CODE registerInit(void) {
    assert(paramStorage.mutex == NULL);
    osMutexDef(paramStorage);
    paramStorage.mutex = osMutexCreate(osMutex(paramStorage));
    assert(paramStorage.mutex != NULL);
#if PRINT_REGISTER_ARRAY
    for (REGISTER_ID i = REG_FIRST; i < REG_MAX; i++) {
        DPRINTF("[%i]  id=%d, name=%s\r\n", i, paramStorage.reg[i].info.mbId, paramStorage.reg[i].name);
        osDelay(PRINT_DELAY_MS);
    }
#endif
    // Test that REGISTER_ID matches array index
    for (REGISTER_ID i = REG_FIRST; i < REG_MAX; i++) {
        if (INFO_ID(i) != i) {
            DPRINTF_ERROR("ERROR paramStorage out of order at index %d,  %d %s\r\n", i, paramStorage.reg[i].name);
            assert(false);
        }
        if (paramStorage.reg[i].info.size > DATA_STRING_SZ) {
            DPRINTF_ERROR("[%i] %s size %d is too large > %d\r\n",
                          i,
                          paramStorage.reg[i].name,
                          paramStorage.reg[i].info.size,
                          DATA_STRING_SZ);
        }
    }

    return RETURN_OK;
}

RETURN_CODE registerByName(const char *name, registerInfo_tp regInfo) {
    assert(regInfo != NULL);
    assert(name != NULL);

    for (REGISTER_ID i = REG_FIRST; i < REG_MAX; i++) {
        if (stricmp(name, paramStorage.reg[i].name) == 0) {
            memcpy(regInfo, &paramStorage.reg[i].info, sizeof(registerInfo_t));
            return RETURN_OK;
        }
    }

    // DPRINTF_ERROR("Error, register %s not found\r\n", name);
    return RETURN_ERR_GEN;
}

RETURN_CODE registerGetName(registerInfo_tp regInfo, const char **name) {
    assert(regInfo != NULL);

    if (REGINFO_ID >= REG_MAX || name == NULL) {
        DPRINTF_ERROR("Error, parameter error\r\n");
        return RETURN_ERR_GEN;
    }
    *name = paramStorage.reg[REGINFO_ID].name;
    return RETURN_OK;
}

RETURN_CODE registerGetType(uint32_t idx, REGISTER_TYPE *p_type) {
#ifdef STM32F411xE
    if (idx >= SB_REG_MAX) {
#else
    if (idx >= MB_REG_MAX) {
#endif
        return RETURN_ERR_PARAM;
    }
    *p_type = paramStorage.reg[idx].info.type;
    return RETURN_OK;
}

// DO NOT ADD DPRINTF! Danger of inifinite loop
RETURN_CODE registerRead(registerInfo_tp regInfo) {
    assert(regInfo != NULL);
    if (REGINFO_ID >= REG_MAX) {
        return RETURN_ERR_GEN;
    }
    osStatus osRc;
    if ((osRc = osMutexWait(paramStorage.mutex, REGISTER_MUTEX_TIMEOUT)) != osOK) {
        return RETURN_ERR_GEN;
    }
    RETURN_CODE rc = RETURN_OK;

    if (paramStorage.reg[REGINFO_ID].readPtr != NULL) {
        if (paramStorage.reg[REGINFO_ID].info.type == DATA_STRING && regInfo->u.dataString == NULL) {
            DPRINTF_ERROR("Register read %s of string requires char *\r\n", paramStorage.reg[REGINFO_ID].name);
            rc = RETURN_ERR_PARAM;
        } else if ((paramStorage.reg[REGINFO_ID].readPtr(regInfo)) != RETURN_OK) {
            rc = RETURN_ERR_GEN;
        }
    } else {
        switch (paramStorage.reg[REGINFO_ID].info.type) {
        case DATA_UINT:
            regInfo->type = DATA_UINT;
            regInfo->u.dataUint = paramStorage.reg[REGINFO_ID].info.u.dataUint;
            break;
        case DATA_INT:
            regInfo->type = DATA_INT;
            regInfo->u.dataInt = paramStorage.reg[REGINFO_ID].info.u.dataInt;
            break;
        case DATA_STRING:
            regInfo->type = DATA_STRING;
            regInfo->u.dataString = paramStorage.reg[REGINFO_ID].info.u.dataString;
            break;
        default:
            assert(false);
        }
    }
    osMutexRelease(paramStorage.mutex);
    return rc;
}

RETURN_CODE registerWrite(const registerInfo_tp regInfo) {
    assert(regInfo != NULL);
    if (REGINFO_ID >= REG_MAX) {
        return RETURN_ERR_GEN;
    }
    RETURN_CODE rc = RETURN_OK;
    if ((osMutexWait(paramStorage.mutex, REGISTER_MUTEX_TIMEOUT)) != osOK) {
        return RETURN_ERR_GEN;
    }

    if (paramStorage.reg[REGINFO_ID].writePtr != NULL) {
        if ((paramStorage.reg[REGINFO_ID].writePtr(regInfo)) != RETURN_OK) {
            rc = RETURN_ERR_GEN;
        }
    } else {
        switch (paramStorage.reg[REGINFO_ID].info.type) {
        case DATA_UINT:
            regInfo->type = DATA_UINT;
            paramStorage.reg[REGINFO_ID].info.u.dataUint = regInfo->u.dataUint;
            break;
        case DATA_INT:
            regInfo->type = DATA_INT;
            paramStorage.reg[REGINFO_ID].info.u.dataInt = regInfo->u.dataInt;
            break;
        case DATA_STRING:
            regInfo->type = DATA_STRING;
            strlcpy(paramStorage.reg[REGINFO_ID].info.u.dataString,
                    regInfo->u.dataString,
                    paramStorage.reg[REGINFO_ID].info.size);
            break;
        default:
            assert(false);
        }
    }
    osMutexRelease(paramStorage.mutex);
    return rc;
}

void registerWriteForce(const registerInfo_tp regInfo) {
    assert(REGINFO_ID < REG_MAX);
    switch (paramStorage.reg[REGINFO_ID].info.type) {
    case DATA_UINT:
        regInfo->type = DATA_UINT;
        paramStorage.reg[REGINFO_ID].info.u.dataUint = regInfo->u.dataUint;
        break;
    case DATA_INT:
        regInfo->type = DATA_INT;
        paramStorage.reg[REGINFO_ID].info.u.dataInt = regInfo->u.dataInt;
        break;
    case DATA_STRING:
        if ((osMutexWait(paramStorage.mutex, REGISTER_MUTEX_TIMEOUT)) != osOK) {
            return;
        }
        regInfo->type = DATA_STRING;
        strcpy(paramStorage.reg[REGINFO_ID].info.u.dataString, regInfo->u.dataString);
        osMutexRelease(paramStorage.mutex);
        break;
    default:
        assert(false);
    }
}

#define CASE_EEPROM_MAP(REGISTER_ID, EEPROM_IDX)                                                                       \
    case REGISTER_ID:                                                                                                  \
        idx = EEPROM_IDX;                                                                                              \
        break

#ifdef STM32H743xx
RETURN_CODE getEepromIdx(const registerInfo_tp regInfo, EepromRegisters_e *p_idx, const char *type) {
    EepromRegisters_e idx = NUM_EEPROM_REGISTERS;
    switch (regInfo->mbId) {
        CASE_EEPROM_MAP(SERIAL_NUMBER, EEPROM_SERIAL_NUMBER);
        CASE_EEPROM_MAP(HW_TYPE, EEPROM_HW_TYPE);
        CASE_EEPROM_MAP(HW_VERSION, EEPROM_HW_VERSION);
        CASE_EEPROM_MAP(IP_ADDR, EEPROM_IP_ADDR);
        CASE_EEPROM_MAP(HTTP_PORT, EEPROM_HTTP_PORT);
        CASE_EEPROM_MAP(UDP_TX_PORT, EEPROM_UDP_TX_PORT);
        CASE_EEPROM_MAP(GATEWAY, EEPROM_GATEWAY);
        CASE_EEPROM_MAP(NETMASK, EEPROM_NETMASK);
        CASE_EEPROM_MAP(UDP_SERVER_IP, EEPROM_SERVER_UDP_IP);
        CASE_EEPROM_MAP(UDP_SERVER_PORT, EEPROM_SERVER_UPD_PORT);
        CASE_EEPROM_MAP(DEPRECATED_TCP_CLIENT_IP, EEPROM_CLIENT_TCP_IP);
        CASE_EEPROM_MAP(TCP_CLIENT_PORT, EEPROM_CLIENT_TCP_PORT);
        CASE_EEPROM_MAP(IP_TX_DATA_TYPE, EEPROM_IP_TX_DATA_TYPE);
        CASE_EEPROM_MAP(UNIQUE_ID, EEPROM_UNIQUE_ID);
        CASE_EEPROM_MAP(ADC_MODE, EEPROM_ADC_MODE);
        CASE_EEPROM_MAP(SENSOR_BOARD_0, EEPROM_SENSOR_BOARD_0);
        CASE_EEPROM_MAP(SENSOR_BOARD_1, EEPROM_SENSOR_BOARD_1);
        CASE_EEPROM_MAP(SENSOR_BOARD_2, EEPROM_SENSOR_BOARD_2);
        CASE_EEPROM_MAP(SENSOR_BOARD_3, EEPROM_SENSOR_BOARD_3);
        CASE_EEPROM_MAP(SENSOR_BOARD_4, EEPROM_SENSOR_BOARD_4);
        CASE_EEPROM_MAP(SENSOR_BOARD_5, EEPROM_SENSOR_BOARD_5);
        CASE_EEPROM_MAP(SENSOR_BOARD_6, EEPROM_SENSOR_BOARD_6);
        CASE_EEPROM_MAP(SENSOR_BOARD_7, EEPROM_SENSOR_BOARD_7);
        CASE_EEPROM_MAP(SENSOR_BOARD_8, EEPROM_SENSOR_BOARD_8);
        CASE_EEPROM_MAP(SENSOR_BOARD_9, EEPROM_SENSOR_BOARD_9);
        CASE_EEPROM_MAP(SENSOR_BOARD_10, EEPROM_SENSOR_BOARD_10);
        CASE_EEPROM_MAP(SENSOR_BOARD_11, EEPROM_SENSOR_BOARD_11);
        CASE_EEPROM_MAP(SENSOR_BOARD_12, EEPROM_SENSOR_BOARD_12);
        CASE_EEPROM_MAP(SENSOR_BOARD_13, EEPROM_SENSOR_BOARD_13);
        CASE_EEPROM_MAP(SENSOR_BOARD_14, EEPROM_SENSOR_BOARD_14);
        CASE_EEPROM_MAP(SENSOR_BOARD_15, EEPROM_SENSOR_BOARD_15);
        CASE_EEPROM_MAP(SENSOR_BOARD_16, EEPROM_SENSOR_BOARD_16);
        CASE_EEPROM_MAP(SENSOR_BOARD_17, EEPROM_SENSOR_BOARD_17);
        CASE_EEPROM_MAP(SENSOR_BOARD_18, EEPROM_SENSOR_BOARD_18);
        CASE_EEPROM_MAP(SENSOR_BOARD_19, EEPROM_SENSOR_BOARD_19);
        CASE_EEPROM_MAP(SENSOR_BOARD_20, EEPROM_SENSOR_BOARD_20);
        CASE_EEPROM_MAP(SENSOR_BOARD_21, EEPROM_SENSOR_BOARD_21);
        CASE_EEPROM_MAP(SENSOR_BOARD_22, EEPROM_SENSOR_BOARD_22);
        CASE_EEPROM_MAP(SENSOR_BOARD_23, EEPROM_SENSOR_BOARD_23);
        CASE_EEPROM_MAP(FAN_POP, EEPROM_FAN_POP);
    default:
        DPRINTF_WARN("Un-handled %s regInfo %d\r\n", type, REGINFO_ID);
        return RETURN_ERR_GEN;
    }
    *p_idx = idx;
    return RETURN_OK;
}

#else
RETURN_CODE getEepromIdx(const registerInfo_tp regInfo, EepromRegisters_e *p_idx, const char *type) {
    EepromRegisters_e idx = NUM_EEPROM_REGISTERS;
    switch (REGINFO_ID) {
        CASE_EEPROM_MAP(SB_SERIAL_NUMBER, EEPROM_SERIAL_NUMBER);
        CASE_EEPROM_MAP(SB_HW_TYPE, EEPROM_HW_TYPE);
        CASE_EEPROM_MAP(SB_HW_VERSION, EEPROM_HW_VERSION);
        CASE_EEPROM_MAP(SB_ADC_MODE, EEPROM_ADC_MODE);
        CASE_EEPROM_MAP(SB_IMU0_PRESENT, EEPROM_IMU0_PRESENT);
        CASE_EEPROM_MAP(SB_IMU1_PRESENT, EEPROM_IMU1_PRESENT);
        CASE_EEPROM_MAP(SB_IMU_BAUD, EEPROM_IMU_BAUD);

    default:
        DPRINTF_WARN("Un-handled %s regInfo %d\r\n", type, REGINFO_ID);
        return RETURN_ERR_GEN;
    }
    *p_idx = idx;
    return RETURN_OK;
}
#endif

RETURN_CODE eepromParamRead(const registerInfo_tp regInfo) {
    assert(regInfo != NULL);
    EepromRegisters_e idx;
    RETURN_CODE rc;
    if ((rc = getEepromIdx(regInfo, &idx, "read")) != RETURN_OK) {
        return rc;
    }
    eepromOpen(osWaitForever);
    if (regInfo->type == DATA_STRING) {
        eepromReadRegister(idx, (uint8_t *)regInfo->u.dataString, regInfo->size);
    } else {
        eepromReadRegister(idx, (uint8_t *)&regInfo->u.dataUint, sizeof(regInfo->u.dataUint));
    }
    eepromClose();
    return RETURN_OK;
}

RETURN_CODE eepromParamWrite(const registerInfo_tp regInfo) {
    assert(regInfo != NULL);
    EepromRegisters_e idx = NUM_EEPROM_REGISTERS;
    RETURN_CODE rc;
    bool allowedToWrite = true;

    if ((rc = getEepromIdx(regInfo, &idx, "write")) != RETURN_OK) {
        return rc;
    }

    for (int i = 0; i < MAX_PROTECTED_EEPROM_DATA; i++) {
        if (idx == protectedEepromData[i]) {
            allowedToWrite = false;
        }
    }

    if (!allowedToWrite) {
#ifdef STM32F411xE
        allowedToWrite = (paramStorage.reg[SB_CFG_ALLOWED].info.u.dataUint != 0);
#else
        allowedToWrite = (paramStorage.reg[MFG_WRITE_EN].info.u.dataUint != 0);
#endif
        if (!allowedToWrite) {
            return RETURN_ERR_STATE;
        }
    }

    eepromOpen(osWaitForever);
    if (regInfo->type == DATA_STRING) {
        eepromWriteRegister(idx, (uint8_t *)regInfo->u.dataString, regInfo->size);
    } else {
        eepromWriteRegister(idx, (uint8_t *)&regInfo->u.dataUint, sizeof(regInfo->u.dataUint));
    }

    eepromClose();
    return RETURN_OK;
}

RETURN_CODE noWriteFn(const registerInfo_tp regInfo) {
    assert(regInfo != NULL);
    const char *regName = NULL;
    registerGetName(regInfo, &regName);
    DPRINTF_ERROR("Write Disabled for register %d %s\r\n", REGINFO_ID, regName);
    return RETURN_ERR_GEN;
}
__attribute__((weak)) RETURN_CODE getUptimeAsRegister(const registerInfo_tp regInfo) {
    assert(false);
    return RETURN_ERR_GEN;
}

// CLI COMMANDS

int16_t cliHandler_readreg(CLI *hCli, int argc, char *argv[]) {
    if (argc != 2) {
        CliWriteString(hCli, "Failed to parse command\r\n");
        return CLI_RESULT_ERROR;
    }
    REGISTER_ID regId = 0;
    registerInfo_t regData;
    if (registerByName(argv[1], &regData) != RETURN_OK) {
        regId = (REGISTER_ID)strtol(argv[1], NULL, STRING_NUMER_BASE(10));
    }
    char regValue[REG_VALUE_SZ] = "";

    if (regId <= REG_FIRST || regId >= REG_MAX) {
        CliWriteString(hCli, "Register out of range\r\n");
        return CLI_RESULT_ERROR;
    }

    memcpy(&regData, &paramStorage.reg[regId].info, sizeof(registerInfo_t));

    char dataString[DATA_STRING_SZ];
    if (regData.type == DATA_STRING) {
        regData.u.dataString = dataString;
        regData.size = sizeof(dataString);
    }

    registerRead(&regData);

    if (regData.type != DATA_UINT && regData.type != DATA_INT && regData.type != DATA_STRING) {
        CliWriteString(hCli, "Register type unsupported\r\n");
        return CLI_RESULT_ERROR;
    }

    CliWriteString(hCli, argv[1]);
    CliWriteString(hCli, " = ");

    switch (regData.type) {
    case DATA_UINT:
        snprintf(regValue, REG_VALUE_SZ, "%" PRIu32 " (%lx)", regData.u.dataUint, regData.u.dataUint);
        break;
    case DATA_INT:
        snprintf(regValue, REG_VALUE_SZ, "%" PRId32 " (%lx)", regData.u.dataInt, regData.u.dataInt);
        break;
    case DATA_STRING:
        snprintf(regValue, REG_VALUE_SZ, "%s", regData.u.dataString);
        break;
    }
    CliWriteString(hCli, regValue);
    CliWriteString(hCli, "\r\n");
    return CLI_RESULT_SUCCESS;
}

int16_t cliHandler_writereg(CLI *hCli, int argc, char *argv[]) {
    if (argc != 3) {
        CliWriteString(hCli, "Failed to parse command\r\n");
        return CLI_RESULT_ERROR;
    }
    REGISTER_ID regId = 0;
    registerInfo_t regData;
    if (registerByName(argv[1], &regData) != RETURN_OK) {
        regId = (REGISTER_ID)strtol(argv[1], NULL, STRING_NUMER_BASE(10));
    }
    char regValue[50] = "";

    if (regId <= REG_FIRST || regId >= REG_MAX) {
        CliWriteString(hCli, "Register out of range\r\n");
        return CLI_RESULT_ERROR;
    }
    memcpy(&regData, &paramStorage.reg[regId].info, sizeof(registerInfo_t));
    registerRead(&regData);

    if (regData.type != DATA_UINT && regData.type != DATA_INT && regData.type != DATA_STRING) {
        CliWriteString(hCli, "Register type unsupported\r\n");
        return CLI_RESULT_ERROR;
    }

    switch (regData.type) {
    case DATA_UINT:
        regData.u.dataUint = strtol(argv[2], NULL, STRING_NUMER_BASE(10));
        break;
    case DATA_INT:
        regData.u.dataInt = strtol(argv[2], NULL, STRING_NUMER_BASE(10));
        break;
    case DATA_STRING:
        regData.u.dataString = argv[2];
        break;
    }

    if (registerWrite(&regData) != 0) {
        CliWriteString(hCli, "Register is read only\r\n");
        return CLI_RESULT_ERROR;
    }

    CliWriteString(hCli, argv[1]);
    CliWriteString(hCli, " = ");

    switch (regData.type) {
    case DATA_UINT:
        snprintf(regValue, 50, "%" PRIu32, regData.u.dataUint);
        break;
    case DATA_INT:
        snprintf(regValue, 50, "%" PRId32, regData.u.dataInt);
        break;
    case DATA_STRING:
        snprintf(regValue, 50, "%s", regData.u.dataString);
        break;
    }
    CliWriteString(hCli, regValue);
    CliWriteString(hCli, "\r\n");
    return CLI_RESULT_SUCCESS;
}

int16_t cliHandler_listreg(CLI *hCli, int argc, char *argv[]) {
    const char *name = NULL;
    if (argc != 1) {
        CliWriteString(hCli, "Failed to parse command\r\n");
        return CLI_RESULT_ERROR;
    }

    CliPrintf(hCli, "INDEX: NAME\r\n");
    CliPrintf(hCli, "==============================\r\n");
    for (REGISTER_ID id = REG_FIRST; ++id < REG_MAX;) {
        // Intentional side effect to skip REG_FIRST in list

        registerInfo_t regData;
        REGDATA_ID = id;
        registerGetName(&regData, &name);
        CliPrintf(hCli, "%5" PRIu32 ": %s\r\n", id, name);
    }

    return CLI_RESULT_SUCCESS;
}

uint32_t copyToBuffer_listreg(char *buf, uint32_t bufSz) {
    const char *name = NULL;
    char *nxt = buf;
    int tmp;

    tmp = snprintf(nxt, bufSz - (nxt - buf), "INDEX: NAME\r\n");
    SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

    tmp = snprintf(nxt, bufSz - (nxt - buf), "==============================\r\n");
    SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

    for (REGISTER_ID id = REG_FIRST; ++id < REG_MAX;) {
        // Intentional side effect to skip REG_FIRST in list

        registerInfo_t regData;
        REGDATA_ID = id;
        registerGetName(&regData, &name);
        tmp = snprintf(nxt, bufSz - (nxt - buf), "%5d: %s\r\n", id, name);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););
    }

    return (nxt - buf);
}
