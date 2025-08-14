/*
 * MB_handleReg.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Jan 16, 2024
 *      Author: jzhang
 */

#include "peripherals/MB_handleReg.h"
#include "MB_cncHandleMsg.h"
#include "cmdAndCtrl.h"
#include "ctrlSpiCommTask.h"
#include "dbCommTask.h"
#include "debugPrint.h"
#include "registerParams.h"
#include "taskWatchdog.h"
#include "webCliMisc.h"
#include "webCmdHandler.h"
#include <stdio.h>
#include <string.h>

#define HTTP_RESPONSE_BUFFER_SIZE_BYTES 80
#define SERIAL_NUMBER_BUFFER_SIZE_BYTES 18
extern __DTCMRAM__ dbCommThreadInfo_t dbCommThreads[MAX_CS_ID];

static const char *SN_STR = "Serial Number String";
static const char *LOOPBACK_STR = "loopback";
static const char *HW_REV_STR = "hw_rev";
static const char *HW_TYPE_STR = "hw_type";
/**
 * Send a request to the individual or all sensor boards to handle
 * the Command And Control command
 * Can only handle UINT32 and INT32 values
 *
 * @param[out]    p_webResponse p_webResponse has a JSON HTTP response that is returned
 * @param[in]     dbId         on MB this identifies the DB to be communicated with, can be ALL
 * @param[in]     commandEnum  identifies the CNC command
 * @param[in]     addr      Register Addr on destination
 * @param[in]     value     If a write command then this contains the UINT value
 * @param[in]     uid       unique identifier
 * @param[in]     valueName used in JSON response to identify the value read or written.
 *
 * @ret osStatus osOK or osErrorNoMemory
 */
static void handleCncRequest(webResponse_tp p_webResponse,
                             int destination,
                             CNC_ACTION_e commandEnum,
                             uint32_t addr,
                             uint32_t value,
                             uint32_t uid,
                             const char *valueName);

/**
 * Send a request to the individual or all sensor boards to handle
 * the Command And Control command
 * Can only handle String values and it must be null terminated.
 *
 * @param[out]    p_webResponse p_webResponse has a JSON HTTP response that is returned
 * @param[in]     dbId         on MB this identifies the DB to be communicated with, can be ALL
 * @param[in]     commandEnum  identifies the CNC command
 * @param[in]     addr      Register Addr on destination
 * @param[in]     str     If a write command then this contains the UINT value
 * @param[in]     uid       unique identifier
 * @param[in]     valueName used in JSON response to identify the value read or written.
 *
 * @ret osStatus osOK or osErrorNoMemory
 */
static void handleCncRequestStr(webResponse_tp p_webResponse,
                                int dbId,
                                CNC_ACTION_e commandEnum,
                                uint32_t addr,
                                const char *str,
                                uint32_t uid,
                                const char *valueName);

/**
 * Check if hwType is an allowed value
 * If fails then create a json response and return false
 * else return true
 *
 * @param[out] p_webResponse p_webResponse has a JSON HTTP response that is returned
 * @param[in]  hwType         value to test
 *
 * @ret bool True if value is allowed
 */
static bool testHwTypeValue(webResponse_tp p_webResponse, int hwType) {
    if (hwType == MAIN_BOARD_ID) {
        return true;
    }
    if (hwType < BOARDTYPE_MCG || hwType > BOARDTYPE_IMU_COIL) {
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
        char buf[HTTP_BUFFER_SIZE];
        json_object *jsonResult = json_object_new_string("HW TYPE value error");
        snprintf(buf,
                 sizeof(buf),
                 "Value %d out of range [%d:%d, %d]",
                 hwType,
                 BOARDTYPE_MCG,
                 BOARDTYPE_IMU_COIL,
                 MAIN_BOARD_ID);
        json_object *jsonDestError = json_object_new_string(buf);
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonDestError, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return false;
    }
    return true;
}

#define FAIL_IF_DESTINATION_MAINBOARD(destinationVal)                                                                  \
    if (destinationVal == MAIN_BOARD_ID) {                                                                             \
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;                                                              \
        char buf[HTTP_BUFFER_SIZE];                                                                                    \
        json_object *jsonResult = json_object_new_string("destination value error");                                   \
        snprintf(buf, sizeof(buf), "Value %d out of range [%d:%d]", destinationVal, 0, 24 - 1);                        \
        json_object *jsonDestError = json_object_new_string(buf);                                                      \
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);   \
        json_object_object_add_ex(                                                                                     \
            p_webResponse->jsonResponse, "detail", jsonDestError, JSON_C_OBJECT_KEY_IS_CONSTANT);                      \
        return p_webResponse;                                                                                          \
    }

/**
 * If MFG_WRITE_EN flag is true it returns true
 * else if writes an HTTP error response to p_webResponse and returns false
 *
 * @param[out] p_webResponse p_webResponse has a JSON HTTP response that is returned if test fails
 *
 * @ret bool if mfg_write_en register is !0
 */
static bool testWriteMfgEnRegister(webResponse_tp p_webResponse) {
    assert(p_webResponse != NULL);
    registerInfo_t regInfo = {.mbId = MFG_WRITE_EN, .type = DATA_UINT};
    registerRead(&regInfo);
    if (regInfo.u.dataUint != false) {
        return true;
    }

    p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;

    json_object *jsonResult = json_object_new_string("FAIL: MFG_WRITE_EN is disabled");
    json_object *jsonError = json_object_new_string("MFG_WRITE_EN must be true before writing to this register");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonError, JSON_C_OBJECT_KEY_IS_CONSTANT);
    return false;
}

webResponse_tp webLoopbackGetDb(const char *jsonStr, const int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    FAIL_IF_DESTINATION_MAINBOARD(destinationVal);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_READ, SB_LOOPBACK_SENSOR, 0, uid, LOOPBACK_STR);

    return p_webResponse;
}

webResponse_tp webLoopbackSetDb(const char *jsonStr, const int strLen) {
    int destinationVal = 0;

    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    FAIL_IF_DESTINATION_MAINBOARD(destinationVal);

    GET_REQ_KEY_VALUE(bool, loopback, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, SB_LOOPBACK_SENSOR, loopback, uid, LOOPBACK_STR);

    return p_webResponse;
}

webResponse_tp webLoopbackIntervalSetDb(const char *jsonStr, const int strLen) {
    int destinationVal = 0;
    RETURN_CODE rc;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    FAIL_IF_DESTINATION_MAINBOARD(destinationVal)

    GET_REQ_KEY_VALUE(int, loopback_interval_us, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    handleCncRequest(p_webResponse,
                     destinationVal,
                     CNC_ACTION_WRITE,
                     SB_LOOPACK_SENSOR_INTERVAL,
                     loopback_interval_us,
                     uid,
                     "loopback_interval_us");

    registerInfo_t regInfo_stream = {.mbId = STREAM_INTERVAL_US, .type = DATA_UINT, .u.dataUint = loopback_interval_us};
    rc = registerWrite(&regInfo_stream);
    if (rc != RETURN_OK) {
        const char *name;
        registerGetName(&regInfo_stream, &name);
        DPRINTF_ERROR("read register %s failed\r\n", name);

        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
        json_object *jsonObjectDetail = json_object_new_string("Failed to write stream interval register");
        json_object *jsonObjectResult = json_object_new_string("Fail");
        json_object_object_add_ex(
            p_webResponse->jsonResponse, "result", jsonObjectResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object_object_add_ex(
            p_webResponse->jsonResponse, "detail", jsonObjectDetail, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return p_webResponse;
    }

    registerInfo_t regInfo_spi = {.mbId = DB_SPI_INTERVAL_US, .type = DATA_UINT, .u.dataUint = loopback_interval_us};
    rc = registerWrite(&regInfo_spi);
    if (rc != RETURN_OK) {
        const char *name;
        registerGetName(&regInfo_spi, &name);
        DPRINTF_ERROR("write register %s failed\r\n", name);

        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
        json_object *jsonObjectDetail = json_object_new_string("Failed to write to SPI interval register");
        json_object *jsonObjectResult = json_object_new_string("Fail");
        json_object_object_add_ex(
            p_webResponse->jsonResponse, "result", jsonObjectResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object_object_add_ex(
            p_webResponse->jsonResponse, "detail", jsonObjectDetail, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return p_webResponse;
    }

    return p_webResponse;
}

webResponse_tp webLoopbackIntervalGetDb(const char *jsonStr, const int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    FAIL_IF_DESTINATION_MAINBOARD(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    handleCncRequest(
        p_webResponse, destinationVal, CNC_ACTION_READ, SB_LOOPACK_SENSOR_INTERVAL, 0, uid, "loopback_interval_ms");

    return p_webResponse;
}

webResponse_tp webSerialNumberGetDb(const char *jsonStr, const int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    uint32_t addr = (destinationVal == MAIN_BOARD_ID) ? SERIAL_NUMBER : SB_SERIAL_NUMBER;
    char snBuffer[SERIAL_NUMBER_BUFFER_SIZE_BYTES];
    handleCncRequestStr(p_webResponse, destinationVal, CNC_ACTION_READ, addr, snBuffer, uid, SN_STR);

    return p_webResponse;
}

webResponse_tp webSerialNumberSetDb(const char *jsonStr, const int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);
    if (testWriteMfgEnRegister(p_webResponse) == false) {
        DPRINTF_WARN("MFG Write EN test failed\r\n");
        DPRINTF_ERROR("httpCode = %d\r\n%s\r\n",
                      p_webResponse->httpCode,
                      json_object_to_json_string(p_webResponse->jsonResponse));
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(const char *, serialNumber_str, obj, json_object_get_string);

    WEB_CMD_PARAM_CLEANUP

    if (destinationVal != MAIN_BOARD_ID) {
        handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, SB_CFG_ALLOWED, 1, uid, NULL);
        handleCncRequestStr(
            p_webResponse, destinationVal, CNC_ACTION_WRITE, SB_SERIAL_NUMBER, serialNumber_str, uid, SN_STR);
        handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, SB_CFG_ALLOWED, 0, uid, NULL);
    } else {
        handleCncRequestStr(
            p_webResponse, destinationVal, CNC_ACTION_WRITE, SERIAL_NUMBER, serialNumber_str, uid, SN_STR);
    }

    return p_webResponse;
}

webResponse_tp webHwRevGetDb(const char *jsonStr, const int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    uint32_t addr = (destinationVal == MAIN_BOARD_ID) ? HW_VERSION : SB_HW_VERSION;
    handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_READ, addr, 0, uid, HW_REV_STR);

    return p_webResponse;
}

webResponse_tp webHwRevSetDb(const char *jsonStr, const int strLen) {
    int destinationVal = 0;

    WEB_CMD_PARAM_SETUP(jsonStr, strLen);
    if (testWriteMfgEnRegister(p_webResponse) == false) {
        return p_webResponse;
    }

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(uint32_t, hw_rev, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(uint32_t, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    uint32_t addr = (destinationVal == MAIN_BOARD_ID) ? HW_VERSION : SB_HW_VERSION;
    handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, addr, hw_rev, uid, HW_REV_STR);

    if (destinationVal != MAIN_BOARD_ID) {
        handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, SB_CFG_ALLOWED, 1, uid, NULL);
        handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, SB_HW_VERSION, hw_rev, uid, HW_REV_STR);
        handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, SB_CFG_ALLOWED, 0, uid, NULL);
    } else {
        handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, HW_VERSION, hw_rev, uid, HW_REV_STR);
    }

    return p_webResponse;
}

webResponse_tp webHwTypeGetDb(const char *jsonStr, const int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    uint32_t addr = (destinationVal == MAIN_BOARD_ID) ? HW_TYPE : SB_HW_TYPE;
    handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_READ, addr, 0, uid, HW_TYPE_STR);

    return p_webResponse;
}

webResponse_tp webHwTypeSetDb(const char *jsonStr, const int strLen) {
    int destinationVal = 0;

    WEB_CMD_PARAM_SETUP(jsonStr, strLen);
    if (testWriteMfgEnRegister(p_webResponse) == false) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(uint32_t, hw_type, obj, json_object_get_int);
    if (!testHwTypeValue(p_webResponse, hw_type)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(uint32_t, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    if (destinationVal != MAIN_BOARD_ID) {
        handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, SB_CFG_ALLOWED, 1, uid, NULL);
        handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, SB_HW_TYPE, hw_type, uid, HW_TYPE_STR);
        handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, SB_CFG_ALLOWED, 0, uid, NULL);
    } else {
        handleCncRequest(p_webResponse, destinationVal, CNC_ACTION_WRITE, HW_TYPE, hw_type, uid, HW_TYPE_STR);
    }
    return p_webResponse;
}

webResponse_tp webReboot(const char *jsonStr, const int strLen) {

    int cnt = 0;
    webResponse_tp p_webResponse = NULL;
    while (cnt++ < 10) {
        p_webResponse = osPoolAlloc(webResponsePoolId);
        if (p_webResponse != NULL) {
            break;
        }
        osDelay(10);
    }
    if (p_webResponse == NULL) {
        return NULL;
    }

    json_object *jsonResponse = json_object_new_object();
    if (jsonResponse == NULL) {
        osPoolFree(webResponsePoolId, p_webResponse);
        return NULL;
    }

    p_webResponse->jsonResponse = jsonResponse;

    struct json_tokener *jsonTok = json_tokener_new();
    struct json_object *obj;
    obj = json_tokener_parse_ex(jsonTok, jsonStr, strLen);
    if (obj == NULL) {
        struct json_object *jsonResult = json_object_new_string("No Json Structure in body");
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(bool, reboot, obj, json_object_get_boolean);

    GET_REQ_KEY_VALUE(int, destination, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP
    if (reboot) {
        if (destination == MAIN_BOARD_ID) {
            p_webResponse->httpCode = HTTP_OK;
            struct json_object *jsonResult = json_object_new_string("Success");
            json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
            watchdogReboot(0);

        } else if (destination < MAX_CS_ID) {
            handleCncRequest(p_webResponse, destination, CNC_ACTION_WRITE, SB_REBOOT_FLAG, reboot, 0, "reboot");

        } else {
            p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
            char buf[HTTP_RESPONSE_BUFFER_SIZE_BYTES];
            json_object *jsonResult = json_object_new_string("destination value error");
            snprintf(buf, sizeof(buf), "Value %d out of range [%d:%d, %d]", destination, 0, MAX_CS_ID, MAIN_BOARD_ID);
            json_object *jsonDestError = json_object_new_string(buf);
            json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
            json_object_object_add_ex(
                p_webResponse->jsonResponse, "detail", jsonDestError, JSON_C_OBJECT_KEY_IS_CONSTANT);
        }
    } else {
        p_webResponse->httpCode = HTTP_OK;
        struct json_object *jsonResult = json_object_new_string("Success");
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        jsonResult = json_object_new_string("Reboot Flag was false, no action occuring");
        json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    }
    return p_webResponse;
}

void regRequestHelper(webResponse_tp p_webResponse,
                      json_object *jsonValuesArray,
                      int destination,
                      webCncCbId_t webCncCbId,
                      const char *valueName) {
    json_object *jsonDacRespObj = json_object_new_object();

    json_object *jsonDest = json_object_new_int(destination);
    json_object_object_add_ex(jsonDacRespObj, "destination", jsonDest, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonValue = json_object_new_int(webCncCbId.p_payload->cmd.value);
    json_object_object_add_ex(jsonDacRespObj, valueName, jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *next = jsonDacRespObj;
    json_object_array_add(jsonValuesArray, next);
}

void regRequestHelperStr(webResponse_tp p_webResponse,
                         json_object *jsonValuesArray,
                         int destination,
                         webCncCbId_t webCncCbId,
                         const char *valueName) {
    json_object *jsonDacRespObj = json_object_new_object();

    json_object *jsonDest = json_object_new_int(destination);
    json_object_object_add_ex(jsonDacRespObj, "destination", jsonDest, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonValue = json_object_new_string(webCncCbId.p_payload->cmd.str);
    json_object_object_add_ex(jsonDacRespObj, valueName, jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *next = jsonDacRespObj;
    json_object_array_add(jsonValuesArray, next);
}

void handleCncRequest(webResponse_tp p_webResponse,
                      int destination,
                      CNC_ACTION_e commandEnum,
                      uint32_t addr,
                      uint32_t value,
                      uint32_t uid,
                      const char *valueName) {
    cncMsgPayload_t cncPayload = {
        .cncMsgPayloadHeader.peripheral = PER_MCU, .cncMsgPayloadHeader.action = commandEnum, .cncMsgPayloadHeader.addr = addr, .value = value, .cncMsgPayloadHeader.cncActionData.result = 0xFF};

    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};

    json_object *jsonValuesArray = json_object_new_array();
    json_object *jsonResult = NULL;
    char buf[HTTP_RESPONSE_BUFFER_SIZE_BYTES];
    uint32_t minDest = 0;
    uint32_t maxDest = 0;

    if (destination == DESTINATION_ALL) {
        minDest = 0;
        maxDest = MAX_CS_ID;
    } else {
        minDest = destination;
        maxDest = destination + 1;
    }

    for (int i = minDest; i < maxDest; i++) {
        if (sensorBoardPresent(i) == true) {
            cncSendMsg(i, SPICMD_CNC, (cncPayload_tp)&cncPayload, uid, webCncRequestCB, (uint32_t)&webCncCbId);
            uint32_t startTick = HAL_GetTick();
            uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);

            if (osResult == TASK_NOTIFY_OK) {
                if (webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result == 0) {
                    regRequestHelper(p_webResponse, jsonValuesArray, i, webCncCbId, valueName);
                } else {
                    uint32_t delta = HAL_GetTick() - startTick;
                    snprintf(buf, sizeof(buf), "cnc error: board %d did not respond in time, delta %lu msec result=%ld", i, delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
                }
            } else {
                // Should never reach this. The CommThread must time out and respond with an error because
                // this code is called from a transient thread.
                assert(false);
            }
            // if there is only one board being accessed and it is not available, fail the request.
        } else if ((maxDest - minDest) == 1) {
            // board not present
            json_object *jsonResult = json_object_new_string("error");
            p_webResponse->httpCode = HTTP_ERROR_PRECONDITION_FAILED;
            json_object *jsonDetail = json_object_new_string("Board is disabled or missing");
            json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
            json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonDetail, JSON_C_OBJECT_KEY_IS_CONSTANT);
            return;
        }
    }
    json_object_object_add_ex(p_webResponse->jsonResponse, "values", jsonValuesArray, JSON_C_OBJECT_KEY_IS_CONSTANT);

    CHECK_ERRORS
}

void handleCncRequestStr(webResponse_tp p_webResponse,
                         int destination,
                         CNC_ACTION_e commandEnum,
                         uint32_t addr,
                         const char *str,
                         uint32_t uid,
                         const char *valueName) {
    char buf[HTTP_RESPONSE_BUFFER_SIZE_BYTES];
    uint32_t minDest = 0;
    uint32_t maxDest = 0;

    cncMsgPayload_t cncPayload = {.cncMsgPayloadHeader.peripheral = PER_MCU, .cncMsgPayloadHeader.action = commandEnum, .cncMsgPayloadHeader.addr = addr, .cncMsgPayloadHeader.cncActionData.result = 0xFF};
    if ((commandEnum == CNC_ACTION_WRITE) && (strlen(str) > (sizeof(cncPayload.str) - sizeof('\0')))) {
        DPRINTF_ERROR("handleCncRequestStr str is too long (%u)\r\n", strlen(str));
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
        snprintf(buf,
                 sizeof(buf),
                 "String value is too long (%u) max (%u)",
                 strlen(str),
                 (sizeof(cncPayload.str) - sizeof("\0")));
        json_object *jsonError = json_object_new_string(buf);
        json_object *jsonResult = json_object_new_string("error");
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonError, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return;
    }
    strlcpy(cncPayload.str, str, sizeof(cncPayload.str));

    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};

    json_object *jsonValuesArray = json_object_new_array();
    json_object *jsonResult = NULL;

    if (destination == DESTINATION_ALL) {
        minDest = 0;
        maxDest = MAX_CS_ID;
    } else {
        minDest = destination;
        maxDest = destination + 1;
    }

    for (int i = minDest; i < maxDest; i++) {
        if ((i == MAIN_BOARD_ID) || (sensorBoardPresent(i) == true)) {
            cncSendMsg(i, SPICMD_CNC, (cncPayload_tp)&cncPayload, uid, webCncRequestCB, (uint32_t)&webCncCbId);
            uint32_t startTick = HAL_GetTick();
            uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
            DPRINTF_CMD_STREAM(
                "%s %d i=%d web payload=%s\r\n", __FUNCTION__, __LINE__, i, (char *)(&webCncCbId.p_payload->cmd.str));
            DPRINTF_CMD_STREAM("%s %d i=%d cnc payload=%s\r\n", __FUNCTION__, __LINE__, i, (cncPayload.str));
            if (osResult == TASK_NOTIFY_OK) {
                if (webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result == 0) {
                    regRequestHelperStr(p_webResponse, jsonValuesArray, i, webCncCbId, valueName);
                } else {
                    uint32_t delta = HAL_GetTick() - startTick;
                    snprintf(buf, sizeof(buf), "cnc error: board %d did not respond in time, delta %lu msec result=%ld", i, delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
                }
            } else {
                // Should never reach this. The CommThread must time out and respond with an error because
                // this code is called from a transient thread.
                assert(false);
            }
            // if there is only one board being accessed and it is not available, fail the request.
        } else if ((maxDest - minDest) == 1) {
            // board not present
            json_object *jsonResult = json_object_new_string("error");
            p_webResponse->httpCode = HTTP_ERROR_PRECONDITION_FAILED;
            json_object *jsonDetail = json_object_new_string("Board is disabled or missing");
            json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
            json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonDetail, JSON_C_OBJECT_KEY_IS_CONSTANT);
            return;
        }
    }
    json_object_object_add_ex(p_webResponse->jsonResponse, "values", jsonValuesArray, JSON_C_OBJECT_KEY_IS_CONSTANT);

    CHECK_ERRORS
}

osStatus handleCncRequestStrJson(json_object *jsonResponse,
                                 int destination,
                                 CNC_ACTION_e commandEnum,
                                 uint32_t addr,
                                 const char *str,
                                 uint32_t uid,
                                 char *valueName) {
    assert(jsonResponse != NULL);

    cncMsgPayload_t cncPayload = {.cncMsgPayloadHeader.peripheral = PER_MCU, .cncMsgPayloadHeader.action = commandEnum, .cncMsgPayloadHeader.addr = addr, .cncMsgPayloadHeader.cncActionData.result = 0xFF};
    if ((commandEnum == CNC_ACTION_WRITE) && (strlen(str) > (sizeof(cncPayload.str) - sizeof('\0')))) {
        DPRINTF_ERROR("handleCncRequestStr str is too long (%u)\r\n", strlen(str));
        json_object *jstr = json_object_new_string("NOT AVAILABLE");
        json_object_object_add_ex(jsonResponse, valueName, jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return osErrorParameter;
    }
    strlcpy(cncPayload.str, str, sizeof(cncPayload.str));

    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};

    if (dbCommThreads[destination].dbCommState.enabled == true) {
        cncSendMsg(destination, SPICMD_CNC, (cncPayload_tp)&cncPayload, uid, webCncRequestCB, (uint32_t)&webCncCbId);
        uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
        DPRINTF_CMD_STREAM("%s %d payload=%s\r\n", __FUNCTION__, __LINE__, (char *)(&webCncCbId.p_payload->cmd.str));
        if (osResult == TASK_NOTIFY_OK) {
            if (webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result == 0) {
                json_object *jstr = json_object_new_string(webCncCbId.p_payload->cmd.str);
                json_object_object_add_ex(jsonResponse, valueName, jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);
                return osOK;
            } else {
                json_object *jstr = json_object_new_string("NOT AVAILABLE");
                json_object_object_add_ex(jsonResponse, valueName, jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);
                return osErrorTimeoutResource;
            }
        } else {
            // Should never reach this. The CommThread must time out and respond with an error because
            // this code is called from a transient thread.
            assert(false);
        }
    }
    json_object *jstr = json_object_new_string("NOT AVAILABLE");
    json_object_object_add_ex(jsonResponse, valueName, jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);
    return osErrorTimeoutResource;
}
