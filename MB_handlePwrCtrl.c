/*
 * peripherals/MB_handlePwrCtrl.c
 *
 *  Copyright Nuvation Research Corporation 2018-2025. All Rights Reserved.
 *
 *  Created on: Feb 14, 2025
 *      Author: rlegault
 */

#include "peripherals/MB_handlePwrCtrl.h"

#include "MB_cncHandleMsg.h"
#include "cmdAndCtrl.h"
#include "dbTriggerTask.h"
#include "debugPrint.h"
#include "gpioMB.h"

#include "webCliMisc.h"
#include "webCmdHandler.h"

#define CLK_SETTLE_TIME_MS 3

#define XINFO_DONT_CARE 0

static bool powerModeEnable = false;

bool isPowerModeEn(void) {
    return (powerModeEnable);
}

void powerModeSet(bool enable) {
    powerModeEnable = enable;
    if (enable) {
        // ENABLE CLK to Sensor boards
        gpioSetOutput(REF_CLK_EN, GPIO_STATE_HIGH);
        osDelay(CLK_SETTLE_TIME_MS);
        // enable power to sensor boards
        gpioSetOutput(PWR_SW_EN, GPIO_STATE_HIGH);
        // Bring all boards up.
        // make sure powerModeEnable is true before calling dbTriggerEnableAll()
        assert(powerModeEnable == true);
        dbTriggerEnableAll();
    } else {
        dbTriggerDisableAll();
        // disable power to sensor boards
        gpioSetOutput(PWR_SW_EN, GPIO_STATE_LOW);
        osDelay(CLK_SETTLE_TIME_MS);
        // disable CLK to Sensor boards
        gpioSetOutput(REF_CLK_EN, GPIO_STATE_LOW);
    }
}

webResponse_tp webPowerGet(const char *jsonStr, int strLen) {
    json_object *jsonResult = NULL;
    char buf[HTTP_BUFFER_SIZE];
    cncPayload_t cncMsg = {.cmd.cncMsgPayloadHeader.peripheral = PER_PWR};
    cncCbFnPtr p_cbFn = webCncRequestCB;
    uint32_t startTick;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_READ;
    cncMsg.cmd.cncMsgPayloadHeader.addr = 0;
    DPRINTF("** taskId = %p\r\n", xTaskGetCurrentTaskHandle());
    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = XINFO_DONT_CARE};

    cncSendMsg(MAIN_BOARD_ID, SPICMD_UNUSED, (cncPayload_tp)&cncMsg, uid, p_cbFn, (uint32_t)&webCncCbId);
    startTick = HAL_GetTick();
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);

    if (osResult == TASK_NOTIFY_OK) {
         uint16_t result = webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;

         if (result == NO_ERROR) {
             DPRINTF_CMD_STREAM_VERBOSE("Value received: %d\r\n", webCncCbId.p_payload->cmd.value);
             json_object *jsonUid = json_object_new_int(uid);
             json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonUid, JSON_C_OBJECT_KEY_IS_CONSTANT);

             json_object *jsonValue = json_object_new_boolean(webCncCbId.p_payload->cmd.value);
             json_object_object_add_ex(p_webResponse->jsonResponse, "on", jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

         } else {
             uint32_t delta = HAL_GetTick() - startTick;
             snprintf(buf, sizeof(buf), "cnc error: power control did not respond in time, delta %lu msec result=%ld", delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
         }
     } else {
         // Must not reach here. THe CNC task must timeout first as this task is transient
         assert(false);
     }
     CHECK_ERRORS

    return p_webResponse;
}

webResponse_tp webPowerSet(const char *jsonStr, int strLen) {
    json_object *jsonResult = NULL;
    char buf[HTTP_BUFFER_SIZE];
    uint32_t startTick = HAL_GetTick();
    cncPayload_t cncMsg = {.cmd.cncMsgPayloadHeader.peripheral = PER_PWR};
    cncCbFnPtr p_cbFn = webCncRequestCB;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    GET_REQ_KEY_VALUE(bool, on, obj, json_object_get_boolean);

    WEB_CMD_PARAM_CLEANUP

    cncMsg.cmd.cncMsgPayloadHeader.action = CNC_ACTION_WRITE;
    cncMsg.cmd.value = on;
    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = XINFO_DONT_CARE};

    cncSendMsg(MAIN_BOARD_ID, SPICMD_UNUSED, (cncPayload_tp)&cncMsg, uid, p_cbFn, (uint32_t)&webCncCbId);

    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);

    if (osResult == TASK_NOTIFY_OK) {
         uint16_t result = webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;
        if (result == NO_ERROR) {
            DPRINTF_CMD_STREAM_VERBOSE("Value received: %d\r\n", webCncCbId.p_payload->cmd.value);
            json_object *jsonUid = json_object_new_int(uid);
            json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonUid, JSON_C_OBJECT_KEY_IS_CONSTANT);

            json_object *jsonValue = json_object_new_boolean(webCncCbId.p_payload->cmd.value);
            json_object_object_add_ex(p_webResponse->jsonResponse, "on", jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);
        } else {
            uint32_t delta = HAL_GetTick() - startTick;
            snprintf(buf, sizeof(buf), "cnc error: Power Control did not respond in time, delta %lu msec result=%ld", delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
        }
    } else {
        // Must not reach here. THe CNC task must timeout first as this task is transient
        assert(false);
    }
    CHECK_ERRORS

    return p_webResponse;
}
