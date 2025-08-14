/*
 * MB_handleFanCtrl.c
 *
 *  Copyright Nuvation Research Corporation 2018-2025. All Rights Reserved.
 *  Created on: Jan 14, 2025
 *      Author: rlegault
 */
#include "mongooseHandler.h"
#include "saqTarget.h"

#include "MB_cncHandleMsg.h"
#include "cmdAndCtrl.h"
#include "debugPrint.h"
#include "fanCtrl.h"
#include "webCliMisc.h"
#include "webCmdHandler.h"

#define MAX_FAN_SPEED_PERC 100
#define XINFO_DONT_CARE 0

#define GET_FAN_CTRL_MUTEX                                                                                             \
    if (fanCtrlOpen(FANCTRL_WAIT_MS) != osOK) {                                                                        \
        DPRINTF_ERROR("Failed to get FAN CTRL Mutex\r\n");                                                             \
        json_object *jsonResult = json_object_new_string("OS Error Mutex Timeout");                                    \
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);   \
        return p_webResponse;                                                                                          \
    }

#define RELEASE_FAN_CTRL_MUTEX fanCtrlClose()

/**
 * Check if hwType is an allowed value
 * If fails then create a json response and return false
 * else return true
 *
 * @param[out] p_webResponse p_webResponse has a JSON HTTP response that is returned
 * @param[in]  speed         value to test
 *
 * @ret bool True if value is allowed
 */
static bool testSpeedValue(webResponse_tp p_webResponse, int speed) {
    if (speed <= MAX_FAN_SPEED_PERC) {
        return true;
    }

    p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
    char buf[HTTP_BUFFER_SIZE];
    json_object *jsonResult = json_object_new_string("Speed value error");
    snprintf(buf, sizeof(buf), "Value %d out of range [0:%d]", speed, MAX_FAN_SPEED_PERC);
    json_object *jsonDestError = json_object_new_string(buf);
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonDestError, JSON_C_OBJECT_KEY_IS_CONSTANT);
    return false;
}

/**
 * Check if Fan id is an allowed value
 * If fails then create a json response and return false
 * else return true
 *
 * @param[out] p_webResponse p_webResponse has a JSON HTTP response that is returned
 * @param[in]  fan         value to test
 *
 * @ret bool True if value is allowed
 */
static bool testFanIdValue(webResponse_tp p_webResponse, int fan) {
    if (fan <= MAX_FAN_ID) {
        return true;
    }

    p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
    char buf[HTTP_BUFFER_SIZE];
    json_object *jsonResult = json_object_new_string("fan value error");
    snprintf(buf, sizeof(buf), "Value %d out of range [0:100]", fan);
    json_object *jsonDestError = json_object_new_string(buf);
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonDestError, JSON_C_OBJECT_KEY_IS_CONSTANT);
    return false;
}

/**
 * Check if address value is valid
 * If fails then create a json response and return false
 * else return true
 *
 * @param[out] p_webResponse p_webResponse has a JSON HTTP response that is returned
 * @param[in]  addr         value to test
 *
 * @ret bool True if value is allowed
 */
static bool testRegisterAddr(webResponse_tp p_webResponse, int addr) {
    if (addr < MAX31760_MAX_REGADDR) {
        return true;
    }

    p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
    char buf[HTTP_BUFFER_SIZE];
    json_object *jsonResult = json_object_new_string("Address error");
    snprintf(buf, sizeof(buf), "addr 0x%x out of range [0:0x%x]", addr, MAX31760_MAX_REGADDR - 1);
    json_object *jsonDestError = json_object_new_string(buf);
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonDestError, JSON_C_OBJECT_KEY_IS_CONSTANT);
    return false;
}

webResponse_tp webFanSpeedGet(const char *jsonStr, int strLen) {

    json_object *jsonResult = NULL;
    char buf[HTTP_BUFFER_SIZE];
    cncPayload_t cncMsg = {.cmd.cncMsgPayloadHeader.peripheral = PER_FAN, .cmd.cncMsgPayloadHeader.addr = FANCTRL_SPEEDCTRL_REGADDR, .cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ};
    cncCbFnPtr p_cbFn = webCncRequestCB;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = XINFO_DONT_CARE};
    GET_FAN_CTRL_MUTEX;
    cncSendMsg(MAIN_BOARD_ID, SPICMD_UNUSED, (cncPayload_tp)&cncMsg, uid, p_cbFn, (uint32_t)&webCncCbId);

    uint32_t startTick = HAL_GetTick();
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
    RELEASE_FAN_CTRL_MUTEX;
    if (osResult == TASK_NOTIFY_OK) {
         uint16_t result = webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;
         if (result == NO_ERROR) {
             DPRINTF_CMD_STREAM_VERBOSE("Value received: %d\r\n", webCncCbId.p_payload->cmd.value);
             json_object *jsonUid = json_object_new_int(uid);
             json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonUid, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonValue = json_object_new_int(webCncCbId.p_payload->cmd.value);
             json_object_object_add_ex(p_webResponse->jsonResponse, "speed", jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonUnit = json_object_new_string("%");
             json_object_object_add_ex(p_webResponse->jsonResponse, "unit", jsonUnit, JSON_C_OBJECT_KEY_IS_CONSTANT);

         } else {
             uint32_t delta = HAL_GetTick() - startTick;
             snprintf(buf, sizeof(buf), "cnc error: Fan Speed did not respond in time, delta %lu msec result=%ld", delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);

         }
     } else {
         // Must not reach here. THe CNC task must timeout first as this task is transient
         assert(false);
     }
     CHECK_ERRORS

    return p_webResponse;
}

webResponse_tp webFanSpeedSet(const char *jsonStr, int strLen) {

    json_object *jsonResult = NULL;
    char buf[HTTP_BUFFER_SIZE];
    cncCbFnPtr p_cbFn = webCncRequestCB;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    GET_REQ_KEY_VALUE(int, speed, obj, json_object_get_int);

    if (!testSpeedValue(p_webResponse, speed)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    WEB_CMD_PARAM_CLEANUP

    cncPayload_t cncMsg = {.cmd.cncMsgPayloadHeader.peripheral = PER_FAN, .cmd.cncMsgPayloadHeader.addr = FANCTRL_SPEEDCTRL_REGADDR, .cmd.cncMsgPayloadHeader.action = CNC_ACTION_WRITE,
            .cmd.value = speed};

    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = XINFO_DONT_CARE};
    GET_FAN_CTRL_MUTEX;
    cncSendMsg(MAIN_BOARD_ID, SPICMD_UNUSED, (cncPayload_tp)&cncMsg, uid, p_cbFn, (uint32_t)&webCncCbId);

    uint32_t startTick = HAL_GetTick();
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
    RELEASE_FAN_CTRL_MUTEX;
    if (osResult == TASK_NOTIFY_OK) {
         uint16_t result = webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;
         if (result == NO_ERROR) {
             DPRINTF_CMD_STREAM_VERBOSE("Value received: %d\r\n", webCncCbId.p_payload->cmd.value);
             json_object *jsonUid = json_object_new_int(uid);
             json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonUid, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonValue = json_object_new_int(webCncCbId.p_payload->cmd.value);
             json_object_object_add_ex(p_webResponse->jsonResponse, "speed", jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonUnit = json_object_new_string("%");
             json_object_object_add_ex(p_webResponse->jsonResponse, "unit", jsonUnit, JSON_C_OBJECT_KEY_IS_CONSTANT);

         } else {
             uint32_t delta = HAL_GetTick() - startTick;
             snprintf(buf, sizeof(buf), "cnc error: Fan Speed did not respond in time, delta %lu msec result=%ld", delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
         }
     } else {
         // Must not reach here. THe CNC task must timeout first as this task is transient
         assert(false);
     }
     CHECK_ERRORS

     return p_webResponse;
}

webResponse_tp webFanTachometerGet(const char *jsonStr, int strLen) {

    json_object *jsonResult = NULL;
    char buf[HTTP_BUFFER_SIZE];
    cncPayload_t cncMsg = {.cmd.cncMsgPayloadHeader.peripheral = PER_FAN};
    cncCbFnPtr p_cbFn = webCncRequestCB;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    GET_REQ_KEY_VALUE(int, fan, obj, json_object_get_int);
    if (!testFanIdValue(p_webResponse, fan)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    WEB_CMD_PARAM_CLEANUP

    cncMsg.cmd.cncMsgPayloadHeader.addr = FANCTRL_TACHOMETER_1_REGADDR + fan;
    cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ;
    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = XINFO_DONT_CARE};
    GET_FAN_CTRL_MUTEX;
    cncSendMsg(MAIN_BOARD_ID, SPICMD_UNUSED, (cncPayload_tp)&cncMsg, uid, p_cbFn, (uint32_t)&webCncCbId);

    uint32_t startTick = HAL_GetTick();
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
    RELEASE_FAN_CTRL_MUTEX;
    if (osResult == TASK_NOTIFY_OK) {
         uint16_t result = webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;
         if (result == NO_ERROR) {
             DPRINTF_CMD_STREAM_VERBOSE("Value received: %d\r\n", webCncCbId.p_payload->cmd.value);
             json_object *jsonUid = json_object_new_int(uid);
             json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonUid, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonValue = json_object_new_double(webCncCbId.p_payload->cmd.fvalue);
             json_object_object_add_ex(p_webResponse->jsonResponse, "speed", jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonUnit = json_object_new_string("RPM");
             json_object_object_add_ex(p_webResponse->jsonResponse, "unit", jsonUnit, JSON_C_OBJECT_KEY_IS_CONSTANT);

         } else {
             uint32_t delta = HAL_GetTick() - startTick;
             snprintf(buf, sizeof(buf), "cnc error: Fan Tachometer did not respond in time, delta %lu msec result=%ld", delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);

         }
     } else {
         // Must not reach here. THe CNC task must timeout first as this task is transient
         assert(false);
     }
     CHECK_ERRORS

    return p_webResponse;
}

webResponse_tp webFanTemperatureGet(const char *jsonStr, int strLen) {

    json_object *jsonResult = NULL;
    char buf[HTTP_BUFFER_SIZE];
    cncPayload_t cncMsg = {.cmd.cncMsgPayloadHeader.peripheral = PER_FAN};
    cncCbFnPtr p_cbFn = webCncRequestCB;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    cncMsg.cmd.cncMsgPayloadHeader.addr = FANCTRL_TEMPERATURE_REGADDR;
    cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ;
    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = XINFO_DONT_CARE};
    GET_FAN_CTRL_MUTEX;
    cncSendMsg(MAIN_BOARD_ID, SPICMD_UNUSED, (cncPayload_tp)&cncMsg, uid, p_cbFn, (uint32_t)&webCncCbId);

    uint32_t startTick = HAL_GetTick();
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
    RELEASE_FAN_CTRL_MUTEX;
    if (osResult == TASK_NOTIFY_OK) {
         uint16_t result = webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;
         if (result == NO_ERROR) {
             DPRINTF_CMD_STREAM_VERBOSE("Value received: %d\r\n", webCncCbId.p_payload->cmd.value);
             json_object *jsonUid = json_object_new_int(uid);
             json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonUid, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonValue = json_object_new_double(webCncCbId.p_payload->cmd.fvalue);
             json_object_object_add_ex(p_webResponse->jsonResponse, "value", jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

         } else {
             uint32_t delta = HAL_GetTick() - startTick;
             snprintf(buf, sizeof(buf), "cnc error: Fan Temperature did not respond in time, delta %lu msec result=%ld", delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
         }
     } else {
         // Must not reach here. THe CNC task must timeout first as this task is transient
         assert(false);
     }
     CHECK_ERRORS

    return p_webResponse;
}

webResponse_tp webFanRegisterGet(const char *jsonStr, int strLen) {

    json_object *jsonResult = NULL;
    char buf[HTTP_BUFFER_SIZE];
    cncPayload_t cncMsg = {.cmd.cncMsgPayloadHeader.peripheral = PER_FAN};
    cncCbFnPtr p_cbFn = webCncRequestCB;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, addr, obj, json_object_get_int);

    if (!testRegisterAddr(p_webResponse, addr)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    WEB_CMD_PARAM_CLEANUP

    cncMsg.cmd.cncMsgPayloadHeader.addr = addr;
    cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ;
    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = XINFO_DONT_CARE};
    GET_FAN_CTRL_MUTEX;
    cncSendMsg(MAIN_BOARD_ID, SPICMD_UNUSED, (cncPayload_tp)&cncMsg, uid, p_cbFn, (uint32_t)&webCncCbId);

    uint32_t startTick = HAL_GetTick();
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
    RELEASE_FAN_CTRL_MUTEX;
    if (osResult == TASK_NOTIFY_OK) {
         uint16_t result = webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;
         if (result == NO_ERROR) {
             DPRINTF_CMD_STREAM_VERBOSE("Value received: %d\r\n", webCncCbId.p_payload->cmd.value);
             json_object *jsonUid = json_object_new_int(uid);
             json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonUid, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonAddr = json_object_new_int(webCncCbId.p_payload->cmd.cncMsgPayloadHeader.addr);
             json_object_object_add_ex(p_webResponse->jsonResponse, "addr", jsonAddr, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonValue = json_object_new_int(webCncCbId.p_payload->cmd.value);
             json_object_object_add_ex(p_webResponse->jsonResponse, "value", jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

         } else {
             uint32_t delta = HAL_GetTick() - startTick;
             snprintf(buf, sizeof(buf), "cnc error: Fan Register Get not respond in time, delta %lu msec result=%ld", delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);

         }
     } else {
         // Must not reach here. THe CNC task must timeout first as this task is transient
         assert(false);
     }
     CHECK_ERRORS

    return p_webResponse;
}

webResponse_tp webFanRegisterSet(const char *jsonStr, int strLen) {

    json_object *jsonResult = NULL;
    char buf[HTTP_BUFFER_SIZE];
    cncPayload_t cncMsg = {.cmd.cncMsgPayloadHeader.peripheral = PER_FAN};
    cncCbFnPtr p_cbFn = webCncRequestCB;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    GET_REQ_KEY_VALUE(int, addr, obj, json_object_get_int);

    if (!testRegisterAddr(p_webResponse, addr)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(int, value, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    cncMsg.cmd.cncMsgPayloadHeader.addr = addr;
    cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_WRITE;
    cncMsg.cmd.value = value;
    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = XINFO_DONT_CARE};
    GET_FAN_CTRL_MUTEX;
    cncSendMsg(MAIN_BOARD_ID, SPICMD_UNUSED, (cncPayload_tp)&cncMsg, uid, p_cbFn, (uint32_t)&webCncCbId);

    uint32_t startTick = HAL_GetTick();
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
    RELEASE_FAN_CTRL_MUTEX;
    if (osResult == TASK_NOTIFY_OK) {
         uint16_t result = webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;
         if (result == NO_ERROR) {
             DPRINTF_CMD_STREAM_VERBOSE("Value received: %d\r\n", webCncCbId.p_payload->cmd.value);
             json_object *jsonUid = json_object_new_int(uid);
             json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonUid, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonAddr = json_object_new_int(webCncCbId.p_payload->cmd.cncMsgPayloadHeader.addr);
             json_object_object_add_ex(p_webResponse->jsonResponse, "addr", jsonAddr, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonValue = json_object_new_int(webCncCbId.p_payload->cmd.value);
             json_object_object_add_ex(p_webResponse->jsonResponse, "value", jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

         } else {
             uint32_t delta = HAL_GetTick() - startTick;
             snprintf(buf, sizeof(buf), "cnc error: Fan Register did not respond in time, delta %lu msec result=%ld", delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
         }
     } else {
         // Must not reach here. THe CNC task must timeout first as this task is transient
         assert(false);
     }
     CHECK_ERRORS

    return p_webResponse;
}
