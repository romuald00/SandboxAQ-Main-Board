/*
 * eeprom.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 31, 2023
 *      Author: jzhang
 */

// Create Instance is use to identify when to create the string mapping table of the enum
// It can only be created once.
#define CREATE_INSTANCE
#include "eeprom.h"
#undef CREATE_INSTANCE
#include "cli/cli.h"
#include "cli/cli_print.h"
#include "eeprma2_m24.h"
#include "eeprom_cmd.h"
#include "registerParams.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_MAX_LEN 16

extern int M24PageSize;

#define CMD_ARG_IDX 1
#define IDX_ARG_IDX 2
#define READWRITE_ADDR_ARG_IDX 2
#define WRITE_VALUE_ARG_IDX 3
#define READ_CNT_ARG_IDX 3
#define DATA_ARG_IDX 3

#define EEPROM_RETRYS 5

#define NEW_LINE_EVERY_VALUE(x) (x)

int16_t testEepromCommand(CLI *hCli, int argc, char *argv[]) {
    uint8_t buffer[256];
    EEPRMA2_M24MemorySizeLocator(EEPRMA2_M24M01_0);
    CliPrintf(hCli, "page size %d\n\r", M24PageSize);
    if (argc > CMD_ARG_IDX) {
        if (argc == IDX_ARG_IDX && strcmp(argv[CMD_ARG_IDX], "read") == 0) {
            if (EEPRMA2_M24_Init(EEPRMA2_M24M01_0) == BSP_ERROR_NONE) {
                CliWriteString(hCli, "Init success\n\r");
            }
            if (EEPRMA2_M24_IsDeviceReady(EEPRMA2_M24M01_0, EEPROM_RETRYS) == BSP_ERROR_NONE) {
                CliWriteString(hCli, "Device is ready\n\r");
            }
            if (EEPRMA2_M24_ReadData(EEPRMA2_M24M01_0, buffer, 0, M24PageSize) == BSP_ERROR_NONE) {
                CliWriteString(hCli, "Read is successful:\n\r");
                CliPrintf(hCli, "%s\n\r", buffer);
            }
            EEPRMA2_M24_DeInit(EEPRMA2_M24M01_0);
        } else if (argc == DATA_ARG_IDX && strcmp(argv[CMD_ARG_IDX], "write") == 0) {
            snprintf((char *)buffer, sizeof(buffer), "%s", argv[DATA_ARG_IDX]);
            if (EEPRMA2_M24_Init(EEPRMA2_M24M01_0) == 0) {
                CliWriteString(hCli, "Init success\n\r");
            }
            if (EEPRMA2_M24_IsDeviceReady(EEPRMA2_M24M01_0, EEPROM_RETRYS) == BSP_ERROR_NONE) {
                CliWriteString(hCli, "Device is ready\n\r");
            }
            if (EEPRMA2_M24_WriteData(EEPRMA2_M24M01_0, buffer, 0, M24PageSize) == BSP_ERROR_NONE) {
                CliPrintf(hCli, "Write %s is successful\n\r", buffer);
            }
            EEPRMA2_M24_DeInit(EEPRMA2_M24M01_0);
        }
    } else {
        CliWriteString(hCli, "no write/read command received\n\r");
    }

    return CLI_RESULT_SUCCESS;
}

int16_t eepromCommand(CLI *hCli, int argc, char *argv[]) {
    int16_t rc = CLI_RESULT_SUCCESS;
    uint8_t buffer[DATA_STRING_SZ + 1] = {0};
    // +3 leave space for \0
    if (strcmp(argv[CMD_ARG_IDX], "list") == 0) {
        CliWriteString(hCli, "RegId, Description\r\n");
        for (EepromRegisters_e i = 0; i < NUM_EEPROM_REGISTERS; i++) {
            CliPrintf(hCli, "%3d  %s\r\n", i, EepromRegisters_e_Strings[i]);
        }
        rc = CLI_RESULT_SUCCESS;
    } else {
        int32_t regIdx = atol(argv[IDX_ARG_IDX]);
        if (regIdx >= NUM_EEPROM_REGISTERS) {
            CliWriteString(hCli, "Index out of range\r\n");
            return CLI_RESULT_ERROR;
        }
        if (EEPRMA2_M24_Init(EEPRMA2_M24M01_0) != BSP_ERROR_NONE) {
            CliWriteString(hCli, "Failed to initialize eeprom\n\r");
            return CLI_RESULT_ERROR;
        }
        if (EEPRMA2_M24_IsDeviceReady(EEPRMA2_M24M01_0, EEPROM_RETRYS) != BSP_ERROR_NONE) {
            CliWriteString(hCli, "Eeprom device is not ready\n\r");
            rc = CLI_RESULT_ERROR;
            // Don't return because we have to DEINIT first
        } else if (argc > IDX_ARG_IDX && strcmp(argv[CMD_ARG_IDX], "read") == 0) {
            eepromOpen(osWaitForever);
            if (eepromReadRegister(regIdx, buffer, sizeof(buffer)) != osOK) {
                CliWriteString(hCli, "Read failed\n\r");
                rc = CLI_RESULT_ERROR;
            } else {

                CliWriteString(hCli, "Read successful:\n\r");

                if (strcmp("dotteddec", eepromRegisterLookupTable[regIdx].type) == 0) {
                    CliPrintf(hCli, "%d.%d.%d.%d\n\r", buffer[3], buffer[2], buffer[1], buffer[0]);
                } else if (strcmp("uint32_t", eepromRegisterLookupTable[regIdx].type) == 0) {
                    CliPrintf(hCli, "0x%x (%d)\n\r", *(uint32_t *)buffer, *(uint32_t *)buffer);
                } else if (strcmp("string", eepromRegisterLookupTable[regIdx].type) == 0) {
                    buffer[BUFFER_MAX_LEN] = '\0';
                    CliPrintf(hCli, "%s\n\r", (char *)buffer);
                }
                rc = CLI_RESULT_SUCCESS;
            }
            eepromClose();
        } else if (argc > IDX_ARG_IDX && strcmp(argv[CMD_ARG_IDX], "write") == 0) {

            if (strcmp("dotteddec", eepromRegisterLookupTable[regIdx].type) == 0) {
                *(uint32_t *)buffer = ipStringToInt(argv[DATA_ARG_IDX]);
            } else if (strcmp("uint32_t", eepromRegisterLookupTable[regIdx].type) == 0) {
                *(uint32_t *)buffer = atoi(argv[DATA_ARG_IDX]);
            } else if (strcmp("string", eepromRegisterLookupTable[regIdx].type) == 0) {
                snprintf((char *)buffer, (size_t)(BUFFER_MAX_LEN + 1), "%s", argv[DATA_ARG_IDX]);
                buffer[BUFFER_MAX_LEN] = '\0';
            }
            eepromOpen(osWaitForever);
            if (eepromWriteRegister(regIdx, buffer, eepromRegisterLookupTable[regIdx].size) != osOK) {
                CliWriteString(hCli, "Write failed\n\r");
                rc = CLI_RESULT_ERROR;
            } else {
                CliWriteString(hCli, "Write successful\n\r");
                rc = CLI_RESULT_SUCCESS;
            }
            eepromClose();
        } else if (argc > IDX_ARG_IDX && strcmp(argv[CMD_ARG_IDX], "addrRead") == 0) {
            uint16_t addr = atoi(argv[READWRITE_ADDR_ARG_IDX]);
            uint16_t cnt = atoi(argv[READ_CNT_ARG_IDX]);
            uint8_t *buffer = malloc(cnt);

            eepromOpen(osWaitForever);
            EEPRMA2_M24_ReadData(EEPRMA2_M24M01_0, buffer, addr, cnt);
            eepromClose();
            for (int i = 0; i < cnt; i++) {
                if ((i % NEW_LINE_EVERY_VALUE(16)) == 0) {
                    CliPrintf(hCli, "\r\n");
                }
                CliPrintf(hCli, "%02x ", buffer[i]);
            }
            CliPrintf(hCli, "\r\n");
            free(buffer);
        } else {
            CliWriteString(hCli, "Not enough command arguments received\n\r");
            rc = CLI_RESULT_TOO_MANY_ARGUMENTS;
        }
        EEPRMA2_M24_DeInit(EEPRMA2_M24M01_0);
    }

    return rc;
}
