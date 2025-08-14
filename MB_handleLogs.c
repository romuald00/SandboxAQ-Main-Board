/*
 * MB_HandleLogs.c
 *
 *  Copyright Nuvation Research Corporation 2018-2025. All Rights Reserved.
 *
 *  Created on: Jan 22, 2025
 *      Author: rlegault
 */

#include "MB_cncHandleMsg.h"
#include "cli/cli_print.h"
#include "cli/cli_uart.h"
#include "cmdAndCtrl.h"
#include "ctrlSpiCommTask.h"
#include "dbCommTask.h"
#include "debugPrint.h"
#include "gpioMB.h"
#include "json.h"
#include "json_object_private.h"
#include "json_types.h"
#include "largeBuffer.h"
#include "peripherals/MB_handleLED.h"
#include "ramLog.h"
#include "registerParams.h"
#include "webCliMisc.h"
#include "webCmdHandler.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "perseioTrace.h"

extern SPI_HandleTypeDef hspi1;

#define MAX_JSON_ERR_BUF_BYTES 160

#define LARGEBUFFER_TIMEOUT_MS 1000

#define UNKNOWN_RESULT 0xFF

typedef struct {
    char *typeStr;
    uint32_t addrValue;
} statAddr_t;

#define DBPROC(x) "dbproc_" #x, PER_DBG_DBPROC_##x

static const statAddr_t statAddrTable[] = {{"watchdog", PER_DBG_TEST_WDOG},
                                           {"task", PER_DBG_TEST_TASK},
                                           {"stack", PER_DBG_TEST_STACK},
                                           {"heap", PER_DBG_TEST_HEAP},
                                           {"system", PER_DBG_SYSTEM_STATUS},
                                           {"reglist", PER_DBG_REGISTER_LIST},
                                           {"spi_db", PER_DBG_SPI_DB},
                                           {"spi_1", PER_DBG_SPI_1},
                                           {"spi_2", PER_DBG_SPI_2},
                                           {"spi_3", PER_DBG_SPI_3},
                                           {DBPROC(0)},
                                           {DBPROC(1)},
                                           {DBPROC(2)},
                                           {DBPROC(3)},
                                           {DBPROC(4)},
                                           {DBPROC(5)},
                                           {DBPROC(6)},
                                           {DBPROC(7)},
                                           {DBPROC(8)},
                                           {DBPROC(9)},
                                           {DBPROC(10)},
                                           {DBPROC(11)},
                                           {DBPROC(12)},
                                           {DBPROC(13)},
                                           {DBPROC(14)},
                                           {DBPROC(15)},
                                           {DBPROC(16)},
                                           {DBPROC(17)},
                                           {DBPROC(18)},
                                           {DBPROC(19)},
                                           {DBPROC(20)},
                                           {DBPROC(21)},
                                           {DBPROC(22)},
                                           {DBPROC(23)},
                                           {NULL, PER_DBG_MAX}};

/**
 * @fn
 *
 * @brief      find the associated addr value for the statType string
 *
 * @param[in]  string stat type to find match for
 *
 * @param[out] addrValue place the associated addr value here
 *
 * @return      True on success, false on failure
 *
 */
static bool getStatAddress(const char *statType, uint32_t *addrValue) {
    for (int i = 0; i < PER_DBG_MAX; i++) {
        if (strcmp(statType, statAddrTable[i].typeStr) == 0) {
            *addrValue = statAddrTable[i].addrValue;
            return true;
        }
    }
    DPRINTF_ERROR("No match found for statType %s\r\n", statType)
    return false;
}

/**
 * @fn handleCncRequest
 *
 * @brief Using parameters prepare CNC request for sensor boards.
 *
 * @Note this will communicate with the destination and copy its debug contents
 * to the 21K holding area in memory D3. The mongooseHandler will write the data from
 * that location to the http connection.
 *
 * @param[out] p_webResponse: Web response
 * @param[in] destination: Command destination
 * @param[in] periEnum: sensor board peripheral, must be MCU for led control
 * @param[in] commandEnum: what command to perform on the peripheral
 * @param[in] perAction: action to take on the peripheral
 * @param[in] addr: subsequent info about action on peripheral.
 * @param[in] uid: Unique command identifier.
 *
 **/
static void handleCncRequest(webResponse_tp p_webResponse,
                             int destination,
                             PERIPHERAL_e periEnum,
                             CNC_ACTION_e commandEnum,
                             CNC_ACTION_e perAction,
                             uint32_t addr,
                             uint32_t uid);

/**
 * @fn handleCNCLocalLog
 *
 * @brief Using parameters prepare http response.
 * @note the actual sending is handled in mongooseHandler
 *
 * @param[out] p_webResponse: Web response
 * @param[in] periEnum: sensor board peripheral, must be MCU for led control
 * @param[in] commandEnum: what command to preform on the peripheral
 * @param[in] uid: Unique command identifier.
 *
 **/
static void
handleCNCLocalLog(webResponse_tp p_webResponse, PERIPHERAL_e periEnum, CNC_ACTION_e commandEnum, uint32_t uid) {

    json_object *jsonResult = json_object_new_string("success");

    p_webResponse->httpCode = HTTP_OK;

    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
}

webResponse_tp webDebugLogGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);
    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    GET_DESTINATION(destinationVal);
    xTracePrintCompactF1(urlLogTxMsg, "%d LOG GET START", destinationVal);
    WEB_CMD_PARAM_CLEANUP;
    if (destinationVal == MAIN_BOARD_ID) {
        handleCNCLocalLog(p_webResponse, PER_LOG, CNC_ACTION_READ, uid);
    } else {
        handleCncRequest(p_webResponse,
                         destinationVal,
                         PER_LOG,
                         CNC_ACTION_READ,
                         CNC_ACTION_READ_LARGE_BUFFER,
                         PER_LOG_ADDR_READ,
                         uid);
    }

    xTracePrintCompactF1(urlLogTxMsg, "%d LOG GET END", destinationVal);

    return p_webResponse;
}

webResponse_tp webDebugLogClear(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);
    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    GET_DESTINATION(destinationVal);

    WEB_CMD_PARAM_CLEANUP;

    handleCncRequest(
        p_webResponse, destinationVal, PER_LOG, CNC_ACTION_WRITE, CNC_ACTION_WRITE, PER_LOG_ADDR_CLEAR, uid);

    return p_webResponse;
}

webResponse_tp webDebugStatsMb(const char *jsonStr, int strLen) {
    int destinationVal = MAIN_BOARD_ID;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);
    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    GET_REQ_KEY_VALUE(const char *, type, obj, json_object_get_string);
    WEB_CMD_PARAM_CLEANUP;
    uint32_t addrValue;
    if (getStatAddress(type, &addrValue)) {
        handleCncRequest(
            p_webResponse, destinationVal, PER_DBG, CNC_ACTION_READ, CNC_ACTION_READ_MED_BUFFER, addrValue, uid);
    }
    return p_webResponse;
}

webResponse_tp webDebugStats(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);
    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    GET_DESTINATION(destinationVal);
    GET_REQ_KEY_VALUE(const char *, type, obj, json_object_get_string);
    WEB_CMD_PARAM_CLEANUP;
    uint32_t addrValue;
    if (getStatAddress(type, &addrValue)) {
        handleCncRequest(
            p_webResponse, destinationVal, PER_DBG, CNC_ACTION_READ, CNC_ACTION_READ_MED_BUFFER, addrValue, uid);
    }

    return p_webResponse;
}

osStatus cliCncRequestCB(spiDbMbCmd_e spiDbMbCmd,
                         uint32_t cbId,
                         uint8_t xInfo,
                         spiDbMbPacketCmdResponse_t cmdResponse,
                         cncPayload_tp p_payload) {
    webCncCbId_tp p_webCncCbId = (webCncCbId_tp)cbId;
    p_webCncCbId->p_payload = p_payload;
    p_webCncCbId->xInfo = xInfo;
    p_webCncCbId->cmdResponse = cmdResponse.cmdResponse;
    xTaskNotifyGive(p_webCncCbId->taskHandle);
    return osOK;
}

uint32_t cliSensorLogGet(CLI *hCli, uint32_t destination) {
    uint32_t uid = 0;
    uint32_t success = 0;
    assert(destination < MAX_CS_ID);
    osStatus rc = largeBufferLock(LARGEBUFFER_TIMEOUT_MS);
    if (rc != osOK) {
        return CLI_RESULT_ERROR;
    }

    uint32_t ramLogSz = largeBufferSize_Bytes();
    uint8_t *largeBufferPtr = largeBufferGet();



    cncPayload_t cncPayload = {.cmd.cncMsgPayloadHeader.peripheral = PER_LOG, .cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ_LARGE_BUFFER,
            .cmd.cncMsgPayloadHeader.cncActionData.largeBufferSz = ramLogSz, .cmd.largeBufferAddr = largeBufferPtr, .cmd.cncMsgPayloadHeader.addr = PER_LOG_ADDR_READ};

    webCncCbId_t cncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0, .hCli = hCli};

    cncSendMsg(destination, SPICMD_CNC, &cncPayload, uid, cliCncRequestCB, (uint32_t)&cncCbId);
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);

    if (osResult == TASK_NOTIFY_OK) {
        if (cncCbId.p_payload == NULL) {
            CliPrintf(hCli, "FAILED cmd.result is NULL\r\n");
        } else if(cncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result == 0) {
            // Translate buffer to printable logs
            uint32_t len = strnlen((char *)&largeBufferPtr[RAM_LOG_HEADER_SIZE], ramLogSz - RAM_LOG_HEADER_SIZE);
            CliWriteOnUart(hCli->cliUart, (uint8_t *)&largeBufferPtr[RAM_LOG_HEADER_SIZE], len);
            success = 1;
        } else {
            CliPrintf(hCli, "FAILED cmd.result=%d\r\n", cncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
        }
    } else {
        CliPrintf(hCli, "cnc log msg get timedout\r\n");
        assert(false);
        // CNC task to timeout first.
    }

    largeBufferUnlock();
    return success;
}

uint32_t cliClearSensorLog(CLI *hCli, uint32_t destination) {
    uint32_t uid = 0;
    uint32_t success = 0;
    assert(destination < MAX_CS_ID);
    osStatus rc = largeBufferLock(LARGEBUFFER_TIMEOUT_MS);
    if (rc != osOK) {
        return CLI_RESULT_ERROR;
    }

    cncPayload_t cncPayload = {.cmd.cncMsgPayloadHeader.peripheral = PER_LOG, .cmd.cncMsgPayloadHeader.action = CNC_ACTION_WRITE,
            .cmd.cncMsgPayloadHeader.cncActionData.largeBufferSz = 0, .cmd.largeBufferAddr = NULL, .cmd.cncMsgPayloadHeader.addr = PER_LOG_ADDR_CLEAR};

    webCncCbId_t cncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0, .hCli = hCli};

    cncSendMsg(destination, SPICMD_CNC, &cncPayload, uid, cliCncRequestCB, (uint32_t)&cncCbId);
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);

    if (osResult == TASK_NOTIFY_OK) {
        if (cncCbId.p_payload == NULL) {
            CliPrintf(hCli, "FAILED cmd.result is NULL\r\n");
        } else if(cncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result == 0) {
            CliPrintf(hCli, "Clear Log successful\r\n");
            success = 1;
        } else {
            CliPrintf(hCli, "FAILED cmd.result=%d\r\n", cncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
        }
    } else {
        CliPrintf(hCli, "cnc log msg get timedout\r\n");
        assert(false);
        // CNC task to timeout first.
    }

    largeBufferUnlock();
    return success;
}

// DO NOT USE THIS CODE as an example, the json response value is mostly ignored and
// its data output is handled in the mongoose thread differently than most other examples.
// the log is written to a known memory area and then written out by the mongoose thread.
// see /debug/log uri handling in mongoose.c
static void handleCncRequest(webResponse_tp p_webResponse,
                             int destination,
                             PERIPHERAL_e periEnum,
                             CNC_ACTION_e commandEnum,
                             CNC_ACTION_e perAction,
                             uint32_t addr,
                             uint32_t xinfo) {
    char buf[MAX_JSON_ERR_BUF_BYTES];
    cncMsgPayload_t cncPayloadCmd = {
            .cncMsgPayloadHeader.peripheral = periEnum,
            .cncMsgPayloadHeader.action = perAction,
            .cncMsgPayloadHeader.addr = addr};

    cncPayloadCmd.largeBufferAddr = largeBufferGet();

    switch (perAction) {
    case CNC_ACTION_READ_SMALL_BUFFER:
            cncPayloadCmd.cncMsgPayloadHeader.cncActionData.largeBufferSz = sensorRamLogTxSize_Bytes(CNC_ACTION_READ_SMALL_BUFFER);
        break;
    case CNC_ACTION_READ_MED_BUFFER:
            cncPayloadCmd.cncMsgPayloadHeader.cncActionData.largeBufferSz = sensorRamLogTxSize_Bytes(CNC_ACTION_READ_MED_BUFFER);
        break;
    case CNC_ACTION_READ_LARGE_BUFFER:
            cncPayloadCmd.cncMsgPayloadHeader.cncActionData.largeBufferSz = sensorRamLogTxSize_Bytes(CNC_ACTION_READ_LARGE_BUFFER);
        break;
    default:
            cncPayloadCmd.cncMsgPayloadHeader.cncActionData.largeBufferSz = 0;
        cncPayloadCmd.largeBufferAddr = NULL;
        break;
    }

    webCncCbId_t webCncCbIdCmd = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};

    json_object *jsonResult = NULL;
    uint32_t logResult = 0;
    uint32_t minDest = 0;
    uint32_t maxDest = 0;

    if (destination == MAIN_BOARD_ID) {
        if ((perAction == CNC_ACTION_READ_MED_BUFFER) && (periEnum == PER_DBG)) {
            webCncCbIdCmd.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result=0;

            cncSendMsg(destination,
                       SPICMD_CNC,
                       (cncPayload_tp)&cncPayloadCmd,
                       xinfo,
                       webCncRequestCB,
                       (uint32_t)&webCncCbIdCmd);
            uint32_t startTick = HAL_GetTick();
            uint32_t osResultCmd = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
            uint32_t cncResult = webCncCbIdCmd.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;

            if (osResultCmd == TASK_NOTIFY_OK) {
                if (cncResult == 0) {
                    json_object *jsonResult = json_object_new_string("success");
                    p_webResponse->httpCode = HTTP_OK;
                    json_object_object_add_ex(
                        p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
                } else {
                    uint32_t delta = HAL_GetTick() - startTick;
                    snprintf(buf, sizeof(buf), "cnc error %s: main board did not respond in time, delta %lu msec result=%ld", __FUNCTION__, delta, webCncCbIdCmd.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
                    jsonResult = json_object_new_string(buf);
                }
            } else {
                // Must not reach here. THe CNC task must timeout first as this task is transient
                assert(false);
            }

            p_webResponse->httpCode = HTTP_OK;
            json_object *jsonResult = json_object_new_string("success");
            json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        } else if ((perAction == CNC_ACTION_WRITE) && (addr == PER_LOG_ADDR_CLEAR)) {
            if (ramLogEmpty() == 0) {
                p_webResponse->httpCode = HTTP_OK;
                json_object *jsonResult = json_object_new_string("success");
                json_object_object_add_ex(
                    p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
            } else {
                p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
                json_object *jsonResult = json_object_new_string("fail");
                json_object_object_add_ex(
                    p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
                json_object *jsonDetail = json_object_new_string("Software Error");
                json_object_object_add_ex(
                    p_webResponse->jsonResponse, "detail", jsonDetail, JSON_C_OBJECT_KEY_IS_CONSTANT);
            }
        } else {
            p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
            json_object *jsonResult = json_object_new_string("fail");
            json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
            json_object *jsonDetail = json_object_new_string("Unknown action or addr");
            json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonDetail, JSON_C_OBJECT_KEY_IS_CONSTANT);
        }
        return;
    }

    if (destination == DESTINATION_ALL) {
        minDest = 0;
        maxDest = MAX_CS_ID;
    } else {
        minDest = destination;
        maxDest = destination + 1;
    }

    for (int i = minDest; i < maxDest; i++) {
        if (sensorBoardPresent(i) == true) {
            xTracePrintCompactF(urlLogTxMsg,
                 cncPayloadCmd.cncMsgPayloadHeader.peripheral, cncPayloadCmd.cncMsgPayloadHeader.action, cncPayloadCmd.cncMsgPayloadHeader.addr);
            DPRINTF_CMD_STREAM("CNC Dest %d  Handling msg peripheral=0x%x addr=0x%x, value=0x%x, lrgBufsz=0x%x, sz=0x%x\r\n", destination, cncPayloadCmd.cncMsgPayloadHeader.peripheral, cncPayloadCmd.cncMsgPayloadHeader.addr, cncPayloadCmd.value, cncPayloadCmd.largeBufferAddr,
                    cncPayloadCmd.cncMsgPayloadHeader.cncActionData.largeBufferSz);
            cncSendMsg(i, SPICMD_CNC, (cncPayload_tp)&cncPayloadCmd, xinfo, webCncRequestCB, (uint32_t)&webCncCbIdCmd);
            uint32_t startTick = HAL_GetTick();
            uint32_t osResultCmd = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
            uint32_t cncResult = webCncCbIdCmd.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;
            xTracePrintCompactF2(urlLogTxMsg, "Task Notify Take: osResultCmd=%d result=%d", osResultCmd, cncResult);
            logResult = webCncCbIdCmd.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;

            if (osResultCmd == TASK_NOTIFY_OK) {
                if (cncResult == 0) {
                    p_webResponse->httpCode = HTTP_OK;
                    return;
                } else {
                    uint32_t delta = HAL_GetTick() - startTick;
                    snprintf(buf, sizeof(buf), "cnc error %s : board %d did not respond in time, delta %lu msec result=%ld", __FILE__, i, delta, webCncCbIdCmd.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
                    jsonResult = json_object_new_string(buf);
                }
            } else {
                // Must not reach here. THe CNC task must timeout first as this task is transient
                assert(false);
            }
        } else if (maxDest - minDest == 1) {
            // board not present
            json_object *jsonResult = json_object_new_string("error");
            p_webResponse->httpCode = HTTP_ERROR_PRECONDITION_FAILED;
            json_object *jsonDetail = json_object_new_string("Board is disabled or missing");
            json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
            json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonDetail, JSON_C_OBJECT_KEY_IS_CONSTANT);
            return;
        }
    }

    if (jsonResult != NULL) {
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
        snprintf(buf, sizeof(buf), "ErrorCodes %ld ", logResult);
        json_object *jsonError = json_object_new_string(buf);
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonError, JSON_C_OBJECT_KEY_IS_CONSTANT);
    } else {
        p_webResponse->httpCode = HTTP_OK;
    }
}

void dumpRxBuffer(CLI *hCli, uint32_t count) {
    uint8_t *largeBufferPtr = largeBufferGet();
    dumpMemory(hCli, largeBufferPtr, count);
}
