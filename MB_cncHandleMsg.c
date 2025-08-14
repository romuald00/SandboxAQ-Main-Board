/*
 * MB_cncHandleMsg.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 24, 2023
 *      Author: rlegault
 */

#include "MB_cncHandleMsg.h"
#include "ctrlSpiCommTask.h"
#include "dbCommTask.h"
#include "debugPrint.h"
#include "fanCtrl.h"
#include "json.h"
#include "largeBuffer.h"
#include "mongooseHandler.h"
#include "peripherals/MB_handlePwrCtrl.h"
#include "perseioTrace.h"
#include "raiseIssue.h"
#include "saqTarget.h"
#include "taskWatchdog.h"
#include "watchDog.h"
#include "webCliMisc.h"
#include "webCmdHandler.h"

#include <string.h>

#define ERROR_MSG_LENGTH 60

bool getPeripheralEnum(webResponse_tp p_webResponse, int destination, const char *peripheral, PERIPHERAL_e *periEnum);
bool getCommandEnum(webResponse_tp p_webResponse, const char *command, CNC_ACTION_e *cmdEnum);
void handleWebCncRequest(webResponse_tp p_webResponse,
                         int destination,
                         PERIPHERAL_e periEnum,
                         CNC_ACTION_e commandEnum,
                         uint32_t address,
                         uint32_t value,
                         uint32_t uid);

// only create this table in mongooseHandle.c
// Case of name is ignored
const strLUT_t peripheralLUT[PER_MAX] = {
    {.name = "NOP", .enumId = PER_NOP},
    {.name = "DAC_0_EXCITATION", .enumId = PER_DAC_0_EXCITATION},
    {.name = "DAC_0_COMPENSATION", .enumId = PER_DAC_0_COMPENSATION},
    {.name = "DAC_1_EXCITATION", .enumId = PER_DAC_1_EXCITATION},
    {.name = "DAC_1_COMPENSATION", .enumId = PER_DAC_1_COMPENSATION},
    {.name = "DDS0_A", .enumId = PER_DDS0_A},
    {.name = "DDS0_B", .enumId = PER_DDS0_B},
    {.name = "DDS0_C", .enumId = PER_DDS0_C},
    {.name = "DDS1_A", .enumId = PER_DDS1_A},
    {.name = "DDS1_B", .enumId = PER_DDS1_B},
    {.name = "DDS1_C", .enumId = PER_DDS1_C},
    {.name = "ADC", .enumId = PER_ADC},
    {.name = "IMU0", .enumId = PER_IMU0},
    {.name = "IMU1", .enumId = PER_IMU1},
    {.name = "MCU", .enumId = PER_MCU},
    {.name = "GPIO", .enumId = PER_GPIO},
    {.name = "EEPROM", .enumId = PER_EEPROM},
    {.name = "TEST_MSG", .enumId = PER_TEST_MSG},
    {.name = "FAN", .enumId = PER_FAN},
    {.name = "LOG", .enumId = PER_LOG},
    {.name = "DBG", .enumId = PER_DBG},
    {.name = "POWER", .enumId = PER_PWR},
};

/**
 * @fn handledPeripheral
 *
 * @brief check if the peripheral is handled by the main board
 *
 * @param[in] PERIPHERAL_e peripheral: peripheral to test
 *
 * @return true if peripheral is handled
 **/
static bool handledPeripheral(PERIPHERAL_e peripheral) {
    switch (peripheral) {
    case PER_MCU:
        // fall through
    case PER_FAN:
        // fall through
    case PER_DBG:
        // fall through
    case PER_PWR:
        return true;
    default:
        return false;
    }
    return false;
}

void cncHandleMsg(cncInfo_tp p_cncInfo) {
    cncPayload_tp p_payload = &p_cncInfo->payload;

    DPRINTF_CMD_STREAM_VERBOSE("MB Handling msg dest =%d peripheral=%d cbId=%d\r\n",
                               p_cncInfo->destination,
                               p_payload->cmd.peripheral,
                               p_cncInfo->cbId);
    spiDbMbPacketCmdResponse_t cmdResponse = {.cmdResponse = UNKNOWN_COMMAND, .cmdUid = CMD_UID_DONT_CARE};
    if (p_cncInfo->destination == MAIN_BOARD_ID) {
        DPRINTF_CMD_STREAM_VERBOSE("MB CNC action=%d  addr = %d,  value = %d \r\n",
                                   p_payload->cmd.action,
                                   p_payload->cmd.addr,
                                   p_payload->cmd.value);

        cncMsgPayload_tp p_msgPayload = (cncMsgPayload_tp)&p_cncInfo->payload;
        if (handledPeripheral(p_msgPayload->cncMsgPayloadHeader.peripheral) == false) {
            p_msgPayload->cncMsgPayloadHeader.cncActionData.result = osErrorValue;
        } else {
            p_payload->cmd.cncMsgPayloadHeader.cncActionData.result = 0;
            switch (p_msgPayload->cncMsgPayloadHeader.peripheral) {
            case PER_MCU:
                switch (p_msgPayload->cncMsgPayloadHeader.action) {
                case CNC_ACTION_WRITE: {
                    osStatus rc;
                    registerInfo_t registerInfo = {.mbId = p_msgPayload->cncMsgPayloadHeader.addr, .u.dataUint = p_msgPayload->value};
                    if(registerGetType(p_msgPayload->cncMsgPayloadHeader.addr, &registerInfo.type) != RETURN_OK) {
                        p_msgPayload->cncMsgPayloadHeader.cncActionData.result = RETURN_ERR_PARAM;
                    } else {
                        char dataString[DATA_STRING_SZ];
                        if (registerInfo.type == DATA_STRING) {
                            registerInfo.u.dataString = dataString;
                            registerInfo.size = DATA_STRING_SZ;
                            registerInfo.type = DATA_STRING;
                        }
                        rc = registerRead(&registerInfo);
                        if (rc != RETURN_OK) {
                            p_msgPayload->cncMsgPayloadHeader.cncActionData.result = RETURN_ERR_PARAM;
                        } else {
                            if (registerInfo.type == DATA_STRING) {
                                registerInfo.u.dataString = p_msgPayload->str;
                                registerInfo.size = strlen(p_msgPayload->str);
                            } else {
                                registerInfo.u.dataUint = p_msgPayload->value;
                            }

                            if ((rc = registerWrite(&registerInfo)) == RETURN_OK) {
                                p_cncInfo->payload.cmd.cncMsgPayloadHeader.addr = registerInfo.mbId;
                                if (registerInfo.type == DATA_STRING) {
                                    strlcpy(p_cncInfo->payload.cmd.str,
                                            registerInfo.u.dataString,
                                            sizeof(p_cncInfo->payload.cmd.str));
                                } else {
                                    p_cncInfo->payload.cmd.value = registerInfo.u.dataUint;
                                }
                                p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = 0;
                            } else {
                                p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = rc;
                            }
                        }
                    }
                } // case CNC_ACTION_WRITE:
                break;
                case CNC_ACTION_READ: {
                    registerInfo_t registerInfo = {.mbId = p_msgPayload->cncMsgPayloadHeader.addr};
                    osStatus rc;

                    if(registerGetType(p_msgPayload->cncMsgPayloadHeader.addr, &registerInfo.type) != RETURN_OK) {
                        p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = RETURN_ERR_PARAM;
                    } else {
                        char dataString[DATA_STRING_SZ];
                        if (registerInfo.type == DATA_STRING) {
                            registerInfo.u.dataString = dataString;
                            registerInfo.size = sizeof(dataString);
                        }

                        if ((rc = registerRead(&registerInfo)) == RETURN_OK) {
                            if (registerInfo.type == DATA_STRING) {
                                strlcpy(p_cncInfo->payload.cmd.str,
                                        registerInfo.u.dataString,
                                        sizeof(p_cncInfo->payload.cmd.str));
                            } else {
                                p_cncInfo->payload.cmd.value = registerInfo.u.dataUint;
                            }
                            p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = 0;
                        } else {
                            p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = rc;
                        }
                    }

                } break;
                default:
                    p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = osErrorParameter;
                    break;
                } //switch (p_msgPayload->cncMsgPayloadHeader.action)
                break;
            case PER_PWR:
                switch(p_cncInfo->payload.cmd.cncMsgPayloadHeader.action) {
                case (CNC_ACTION_READ):
                    p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = osOK;
                    p_cncInfo->payload.cmd.value = isPowerModeEn();
                    break;
                case (CNC_ACTION_WRITE):
                    powerModeSet(p_cncInfo->payload.cmd.value);
                    p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = osOK;
                    break;
                default:
                    p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = osErrorParameter;
                    cmdResponse.cmdResponse = VALUE_ERROR;
                    DPRINTF_ERROR("Unhandled case %d for action under fan\r\n", p_cncInfo->payload.cmd.cncMsgPayloadHeader.action);
                    return;
                }
                DPRINTF("*** value = %d\r\n", p_cncInfo->payload.cmd.value);
                break;
                break;

            case PER_FAN:
                fanCtrlValue_t fanCtrlValue;
                switch(p_cncInfo->payload.cmd.cncMsgPayloadHeader.action) {
                case (CNC_ACTION_READ):
                    p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = fanCtrlReadHelper(p_cncInfo->payload.cmd.cncMsgPayloadHeader.addr, &fanCtrlValue);
                    break;
                case (CNC_ACTION_WRITE):
                    fanCtrlValue.u8Value = p_cncInfo->payload.cmd.value;
                    p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = fanCtrlWriteHelper(p_cncInfo->payload.cmd.cncMsgPayloadHeader.addr, &fanCtrlValue);
                    break;
                default:
                    p_cncInfo->payload.cmd.cncMsgPayloadHeader.cncActionData.result = RETURN_ERR_PARAM;
                    cmdResponse.cmdResponse = VALUE_ERROR;
                    DPRINTF_ERROR("Unhandled case %d for action under fan\r\n", p_cncInfo->payload.cmd.cncMsgPayloadHeader.action);
                    break;
                }
                break;

            case PER_DBG: {
                char *buf = (char *)largeBufferGet();
                uint32_t bufSz = largeBufferSize_Bytes();
                cmdResponse.cmdResponse = NO_ERROR;
                p_payload->cmd.cncMsgPayloadHeader.cncActionData.result = osOK;
                switch(p_cncInfo->payload.cmd.cncMsgPayloadHeader.addr) {
                case PER_DBG_TEST_WDOG:
                    char *nxt = buf;
                    for (uint8_t i = 0; i < WDT_NUM_TASKS; i++) {
                        nxt += snprintf((char *)nxt,
                                        bufSz - (nxt - buf),
                                        "%2d - %-16s %5ld %5ld\r\n",
                                        i,
                                        watchdogGetTaskName(i),
                                        watchdogGetHighWatermark(i),
                                        watchdogGetPeriod(i));
                    }
                    break;
                case PER_DBG_TEST_TASK:
                    vTaskGetCustomRunTimeStats(buf);
                    break;
                case PER_DBG_DBPROC_0:
                    // fall through
                case PER_DBG_DBPROC_1:
                    // fall through
                case PER_DBG_DBPROC_2:
                    // fall through
                case PER_DBG_DBPROC_3:
                    // fall through
                case PER_DBG_DBPROC_4:
                    // fall through
                case PER_DBG_DBPROC_5:
                    // fall through
                case PER_DBG_DBPROC_6:
                    // fall through
                case PER_DBG_DBPROC_7:
                    // fall through
                case PER_DBG_DBPROC_8:
                    // fall through
                case PER_DBG_DBPROC_9:
                    // fall through
                case PER_DBG_DBPROC_10:
                    // fall through
                case PER_DBG_DBPROC_11:
                    // fall through
                case PER_DBG_DBPROC_12:
                    // fall through
                case PER_DBG_DBPROC_13:
                    // fall through
                case PER_DBG_DBPROC_14:
                    // fall through
                case PER_DBG_DBPROC_15:
                    // fall through
                case PER_DBG_DBPROC_16:
                    // fall through
                case PER_DBG_DBPROC_17:
                    // fall through
                case PER_DBG_DBPROC_18:
                    // fall through
                case PER_DBG_DBPROC_19:
                    // fall through
                case PER_DBG_DBPROC_20:
                    // fall through
                case PER_DBG_DBPROC_21:
                    // fall through
                case PER_DBG_DBPROC_22:
                    // fall through
                case PER_DBG_DBPROC_23:
                    appendDbprocStatsToBuffer(buf, bufSz, p_cncInfo->payload.cmd.cncMsgPayloadHeader.addr-PER_DBG_DBPROC_0);
                    break;
                case PER_DBG_TEST_STACK:
                    TaskStatus_t *taskStatus = NULL;
                    volatile UBaseType_t numTasks = uxTaskGetNumberOfTasks();
                    taskStatus = pvPortMalloc(numTasks * sizeof(TaskStatus_t));
                    numTasks = uxTaskGetSystemState(taskStatus, numTasks, NULL);
                    if (taskStatus != NULL) {
                        char *nxt = buf;
                        for (int i = 0; i < numTasks; i++) {
                            if (taskStatus != NULL) {
                                nxt += snprintf((char *)nxt,
                                                bufSz - (nxt - buf),
                                                "%-16s - %6d\r\n",
                                                taskStatus[i].pcTaskName,
                                                taskStatus[i].usStackHighWaterMark);
                            }
                        }
                        vPortFree(taskStatus);
                    }
                    break;

                case PER_DBG_TEST_HEAP:
                    snprintf((char *)buf,
                             bufSz,
                             "Free Heap = 0x%x (%u) Bytes\r\n",
                             xPortGetFreeHeapSize(),
                             xPortGetFreeHeapSize());
                    break;
                case PER_DBG_SYSTEM_STATUS: {
                    char *nxt = buf;
                    if (testForPeripheralError()) {
                        nxt += snprintf((char *)nxt, bufSz - (nxt - buf), "Peripheral Error Present\r\n");
                        peripheralErrorAppend((char *)nxt, bufSz - (nxt - buf));
                    } else {
                        nxt += snprintf((char *)nxt, bufSz - (nxt - buf), "No Errors\r\n");
                    }
                } break;
                case PER_DBG_REGISTER_LIST: {
                    copyToBuffer_listreg(buf, bufSz);
                } break;
                case PER_DBG_SPI_3:
                    // fall through
                case PER_DBG_SPI_2:
                    // fall through
                case PER_DBG_SPI_1: {
                    appendSpiStatsToBuffer(buf, bufSz, p_cncInfo->payload.cmd.cncMsgPayloadHeader.addr-PER_DBG_SPI_1);
                } break;

                default:
                    p_payload->cmd.cncMsgPayloadHeader.cncActionData.result = osErrorResource;
                    cmdResponse.cmdResponse = VALUE_ERROR;
                    DPRINTF_ERROR("Unknown address %d\r\n", p_msgPayload->cncMsgPayloadHeader.addr);
                    snprintf((char *)buf, bufSz, "Unknown address %ld\r\n", p_msgPayload->cncMsgPayloadHeader.addr);
                    break;
                }
                break; // of case PER_DBG:
            }
            default:
                assert(0);
                break;
            }// switch (p_msgPayload->cncMsgPayloadHeader.peripheral)

            if (p_cncInfo->cbFnPtr) {
                p_cncInfo->cbFnPtr(p_cncInfo->cmd, p_cncInfo->cbId, p_cncInfo->xInfo, cmdResponse, p_payload);
            }
        } // end of if((handledPeripheral(p_msgPayload->cncMsgPayloadHeader.peripheral) == false){} else
    } else { // DESTINATION is a sensor board
        if (dbProcSendMsg(p_cncInfo->destination, p_cncInfo->cmd, p_cncInfo->cbFnPtr, p_cncInfo->cbId, p_payload) !=
            osOK) {
            if (p_cncInfo->cbFnPtr) {
                p_payload->cmd.cncMsgPayloadHeader.cncActionData.result = osErrorResource;
                spiDbMbPacketCmdResponse_t cmdResponse = {.cmdResponse = UNKNOWN_COMMAND, .cmdUid = CMD_UID_DONT_CARE};
                p_cncInfo->cbFnPtr(p_cncInfo->cmd, p_cncInfo->cbId, p_cncInfo->xInfo, cmdResponse, p_payload);
            }
        }
    }
}

webResponse_tp webCnc(const char *jsonStr, int strLen) {
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, destination, obj, json_object_get_int);
    if (!testDestinationValue(p_webResponse, destination)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    PERIPHERAL_e periEnum;
    GET_REQ_KEY_VALUE(const char *, peripheral, obj, json_object_get_string);
    if (!getPeripheralEnum(p_webResponse, destination, peripheral, &periEnum)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    CNC_ACTION_e commandEnum;
    GET_REQ_KEY_VALUE(const char *, command, obj, json_object_get_string);
    if (!getCommandEnum(p_webResponse, command, &commandEnum)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(int, address, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, value, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleWebCncRequest(p_webResponse, destination, periEnum, commandEnum, address, value, uid);

    return p_webResponse;
}

bool testDestinationValue(webResponse_tp p_webResponse, int destination) {
    if (destination >= MAX_CS_ID && (destination != MAIN_BOARD_ID)) {
        DPRINTF_WARN("Destination %d out of range\r\n", destination);
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
        char buf[44];
        json_object *jsonResult = json_object_new_string("destination value error");
        snprintf(buf, sizeof(buf), "Value %d out of range [0-%d | %d]", destination, MAX_CS_ID - 1, MAIN_BOARD_ID);
        json_object *jsonDestError = json_object_new_string(buf);
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonDestError, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return false;
    }
    return true;
}

bool getCommandEnum(webResponse_tp p_webResponse, const char *command, CNC_ACTION_e *cmdEnum) {
    for (int i = 0; i < CNC_WEB_CMD_MAX; i++) {
        *cmdEnum = i;
        if (strncasecmp(commandLUT[i].name, command, strlen(commandLUT[i].name)) == 0) {
            return true;
        }
    }
    p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
    char buf[40];
    json_object *jsonResult = json_object_new_string("command value error");

    snprintf(buf, sizeof(buf), "Invalid command %s", command);
    json_object *jsonError = json_object_new_string(buf);
    json_object *jsonArray = json_object_new_array();
    for (int i = 0; i < CNC_WEB_CMD_MAX; i++) {
        json_object *next = json_object_new_string(commandLUT[i].name);
        json_object_array_add(jsonArray, next);
    }

    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonError, JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(p_webResponse->jsonResponse, "allowed", jsonArray, JSON_C_OBJECT_KEY_IS_CONSTANT);
    return false;
}

bool getPeripheralEnum(webResponse_tp p_webResponse, int destination, const char *peripheral, PERIPHERAL_e *periEnum) {
    assert(destination < MAX_CS_ID || destination == MAIN_BOARD_ID);

    char buf[ERROR_MSG_LENGTH];
    if (destination == MAIN_BOARD_ID) {
        int cmp = strncasecmp(peripheral, peripheralLUT[PER_MCU].name, strlen(peripheralLUT[PER_MCU].name));
        if (cmp != 0) {
            p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;

            json_object *jsonResult = json_object_new_string("peripheral value error on main board");
            snprintf(buf, sizeof(buf), "Only %s peripheral is allowed on main board", peripheralLUT[PER_MCU].name);
            json_object *jsonDestError = json_object_new_string(buf);
            json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
            json_object_object_add_ex(
                p_webResponse->jsonResponse, "detail", jsonDestError, JSON_C_OBJECT_KEY_IS_CONSTANT);
            return false;
        }
        *periEnum = PER_MCU;
        return true;
    } else {
        for (int i = 0; i < PER_MAX; i++) {
            *periEnum = i;
            if (strncasecmp(peripheralLUT[i].name, peripheral, strlen(peripheralLUT[i].name)) == 0) {
                return true;
            }
        }
    }
    p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
    json_object *jsonResult = json_object_new_string("peripheral value error");
    snprintf(buf, sizeof(buf), "Invalid peripheral %s", peripheral);
    json_object *jsonError = json_object_new_string(buf);
    json_object *jsonArray = json_object_new_array();
    for (int i = 0; i < PER_MAX; i++) {
        json_object *next = json_object_new_string(peripheralLUT[i].name);
        json_object_array_add(jsonArray, next);
    }

    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonError, JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(p_webResponse->jsonResponse, "allowed", jsonArray, JSON_C_OBJECT_KEY_IS_CONSTANT);
    return false;
}

osStatus webCncRequestCB(spiDbMbCmd_e spiDbMbCmd,
                         uint32_t cbId,
                         uint8_t xInfo,
                         spiDbMbPacketCmdResponse_t cmdResponse,
                         cncPayload_tp p_payload) {
    webCncCbId_tp p_webCncCbId = (webCncCbId_tp)cbId;
    p_webCncCbId->p_payload = p_payload;
    p_webCncCbId->xInfo = xInfo;
    p_webCncCbId->cmdResponse = cmdResponse.cmdResponse;
    xTracePrintF(imuMsg,"webCncRequestCB peripheral=%d, action=%d ResponseStr=%s",p_payload->cmd.cncMsgPayloadHeader.peripheral, p_payload->cmd.cncMsgPayloadHeader.action,
            p_payload->cmd.str);
    xTaskNotifyGive(p_webCncCbId->taskHandle);
    return osOK;
}

void handleWebCncRequest(webResponse_tp p_webResponse,
                         int destination,
                         PERIPHERAL_e periEnum,
                         CNC_ACTION_e action,
                         uint32_t address,
                         uint32_t value,
                         uint32_t uid) {
    CNC_ACTION_e commandEnum = action;
    cncMsgPayload_t cncPayload = {
        .cncMsgPayloadHeader.peripheral = periEnum, .cncMsgPayloadHeader.action = commandEnum, .cncMsgPayloadHeader.addr = address, .value = value, .cncMsgPayloadHeader.cncActionData.result = 0xFF};
    DPRINTF("Destination=%d\r\n", destination);
    DPRINTF("CNC PAYLOAD = .per = %d, .action=%d, addr=%d, value=%d\r\n", periEnum, commandEnum, address, value);
    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};

    cncSendMsg(destination, SPICMD_CNC, (cncPayload_tp)&cncPayload, uid, webCncRequestCB, (uint32_t)&webCncCbId);
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);

    if (osResult == TASK_NOTIFY_OK) {
        char buf[40];
        if (webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result != 0) {
            p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
            json_object *jsonResult = json_object_new_string("cnc error");
            snprintf(buf, sizeof(buf), "ErrorCode %ld", webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);

            json_object *jsonError = json_object_new_string(buf);
            json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
            json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonError, JSON_C_OBJECT_KEY_IS_CONSTANT);

        } else {
            p_webResponse->httpCode = HTTP_OK;
            json_object *jsonResult = json_object_new_string("success");
            json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);

            json_object *jsonDestination = json_object_new_int(destination);
            json_object_object_add_ex(
                p_webResponse->jsonResponse, "destination", jsonDestination, JSON_C_OBJECT_KEY_IS_CONSTANT);

            snprintf(buf, sizeof(buf), "%s", peripheralLUT[webCncCbId.p_payload->cmd.cncMsgPayloadHeader.peripheral].name);
            json_object *jsonPeripheral = json_object_new_string(buf);
            json_object_object_add_ex(
                p_webResponse->jsonResponse, "peripheral", jsonPeripheral, JSON_C_OBJECT_KEY_IS_CONSTANT);

            json_object *jsonCommand = json_object_new_string(commandLUT[action].name);

            json_object_object_add_ex(
                p_webResponse->jsonResponse, "command", jsonCommand, JSON_C_OBJECT_KEY_IS_CONSTANT);

            json_object *jsonAddress = json_object_new_int(webCncCbId.p_payload->cmd.cncMsgPayloadHeader.addr);
            json_object_object_add_ex(
                p_webResponse->jsonResponse, "address", jsonAddress, JSON_C_OBJECT_KEY_IS_CONSTANT);

            json_object *jsonValue = json_object_new_int(webCncCbId.p_payload->cmd.value);
            json_object_object_add_ex(p_webResponse->jsonResponse, "value", jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);
        }
    } else {
        assert(false);
        // we cannot timeout here as this thread is temporary. must rely on
        // CNC task to timeout for us
    }
}
