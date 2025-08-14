/*
 * MB_handleDDS.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 8, 2023
 *      Author: jzhang
 */

#include "peripherals/MB_handleIMU.h"
#include "MB_cncHandleMsg.h"
#include "ctrlSpiCommTask.h"
#include "ddsTrigTask.h"
#include "debugPrint.h"
#include "perseioTrace.h"
#include "webCliMisc.h"
#include "webCmdHandler.h"
#include <stdio.h>
#include <string.h>

extern const strLUT_t peripheralLUT[PER_MAX];
extern __DTCMRAM__ dbCommThreadInfo_t dbCommThreads[MAX_CS_ID];

#define ERRORBUFFER_SZ 70

typedef struct {
    const char *imuType;
} imuReturnFields_t;

typedef enum { IMU_STATE_KEY, IMU_CMD_RESPONSE_KEY, NUM_IMU_RETURN_TYPES } imuReturnFields_e;

imuReturnFields_t imuFieldsLookup[NUM_IMU_RETURN_TYPES] = {
    {.imuType = "state"},
    {.imuType = "cmdResponse"},
};

/**
 * @fn handleCncIMUCmd
 *
 * @brief Handle Command and control Text Commands to the IMU
 *
 * @param[in] webResponse_tp, location where result is written to
 * @param[in] destination, board that contains the imu
 * @param[in] commandEnum, CNC command read write etc
 * @param[in] peripheral, PER_IMU0 or PER_IMU1
 * @param[in] strCmd, text of command to write to IMU
 * @param[in] uid, unique message identifier, user specified, returned as is when command completes
 **/
void handleCncIMUCmd(webResponse_tp p_webResponse,
                     int destination,
                     CNC_ACTION_e commandEnum,
                     PERIPHERAL_e peripheral,
                     const char *strCmd,
                     uint32_t uid);

webResponse_tp webImuCmd(const char *body, const int bodyLen) {

    int destinationVal = 0;
    xTracePrintCompactF0(imuMsg, "IMU cmd");
    osDelay(1);
    WEB_CMD_PARAM_SETUP(body, bodyLen);
    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    GET_REQ_KEY_VALUE(const char *, cmdStr, obj, json_object_get_string);
    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP;
    PERIPHERAL_e peripheral = slot + PER_IMU0;
    xTracePrintF(imuMsg, "IMU %d cmd=%s", peripheral - PER_IMU0, cmdStr);
    DPRINTF("IMU %d cmd=%s\r\n", peripheral - PER_IMU0, cmdStr);
    handleCncIMUCmd(p_webResponse, destinationVal, CNC_IMU_CMD, peripheral, cmdStr, uid);
    return p_webResponse;
}

void imuRequestHelper(webResponse_tp p_webResponse,
                      json_object *jsonValuesArray,
                      int destination,
                      webCncCbId_t webCncCbId,
                      imuReturnFields_e lookupIdx) {

    uint32_t imuIdx = webCncCbId.p_payload->cmd.cncMsgPayloadHeader.peripheral - PER_IMU0;

    json_object *jsonImuRespObj = json_object_new_object();

    json_object *jsonDest = json_object_new_int(destination);
    json_object *jsonImuIdx = json_object_new_int(imuIdx);
    json_object *jsonImu = json_object_new_string(peripheralLUT[webCncCbId.p_payload->cmd.cncMsgPayloadHeader.peripheral].name);

    json_object_object_add_ex(jsonImuRespObj, "destination", jsonDest, JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(jsonImuRespObj, "imu", jsonImuIdx, JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(jsonImuRespObj, "peripheral", jsonImu, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonValue = json_object_new_string(webCncCbId.p_payload->cmd.str);
    json_object_object_add_ex(
        jsonImuRespObj, imuFieldsLookup[lookupIdx].imuType, jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object_array_add(jsonValuesArray, jsonImuRespObj);
}

void handleCncIMUCmd(webResponse_tp p_webResponse,
                     int destination,
                     CNC_ACTION_e commandEnum,
                     PERIPHERAL_e peripheral,
                     const char *imuCmd,
                     uint32_t xinfo) {
    cncMsgPayload_t cncMsgPayload = {.cncMsgPayloadHeader.peripheral = peripheral,.cncMsgPayloadHeader.action = commandEnum, .cncMsgPayloadHeader.cncActionData.result = 0xFF};
    strncpy(cncMsgPayload.str, imuCmd, sizeof(cncMsgPayload.str) - sizeof('\0'));

    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};

    json_object *jsonValuesArray = json_object_new_array();
    json_object *jsonResult = NULL;
    char buf[ERRORBUFFER_SZ];
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
        if (dbCommThreads[i].dbCommState.enabled == true) {
            xTracePrintF(imuMsg,
                         "handleCncImuCmd dest=%d, peripheral=%d, action=%d cmdStr=%s",
                         i,
                         peripheral,
                         commandEnum,
                         cncMsgPayload.str);
            cncSendMsg(i, SPICMD_CNC, (cncPayload_tp)&cncMsgPayload, xinfo, webCncRequestCB, (uint32_t)&webCncCbId);
            uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
            uint32_t startTick = HAL_GetTick();
            if (osResult == TASK_NOTIFY_OK) {
                xTracePrintF(imuMsg,"handleCncImuCmd result=%d",i,peripheral, commandEnum, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
                if (webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result == 0) {

                    imuRequestHelper(p_webResponse, jsonValuesArray, i, webCncCbId, IMU_CMD_RESPONSE_KEY);
                } else {
                    uint32_t delta = HAL_GetTick() - startTick;
                    snprintf(buf, sizeof(buf), "cnc error: board %d did not respond in time, delta %lu msec result=%ld", i, delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
                    jsonResult = json_object_new_string(buf);
                }
            } else {
                // Must not reach here. THe CNC task must timeout first as this task is transient
                assert(false);
            }
        }
    }

    json_object_object_add_ex(p_webResponse->jsonResponse, "values", jsonValuesArray, JSON_C_OBJECT_KEY_IS_CONSTANT);

    CHECK_ERRORS
}
