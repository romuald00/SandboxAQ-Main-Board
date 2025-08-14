/*
 * MB_handleLED.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 18, 2023
 *      Author: jzhang
 */

#include "peripherals/MB_handleLED.h"
#include "dbCommTask.h"

#include "MB_cncHandleMsg.h"
#include "cmdAndCtrl.h"
#include "ctrlSpiCommTask.h"
#include "debugPrint.h"
#include "json.h"
#include "json_object_private.h"
#include "json_types.h"
#include "registerParams.h"
#include "webCliMisc.h"
#include "webCmdHandler.h"
#include <stdio.h>
#include <string.h>

#define MAX_JSON_ERR_BUF_BYTES 70
#define UNKNOWN_RESULT 0xFF

extern __DTCMRAM__ dbCommThreadInfo_t dbCommThreads[MAX_CS_ID];

/**
 * @fn handleCncRequest
 *
 * @brief Using parameters prepare CNC request for sensor boards.
 *
 * @param[out] p_webResponse: Web response
 * @param[in] destination: Command destination
 * @param[in] periEnum: sensor board peripheral, must be MCU for led control
 * @param[in] commandEnum: what command to preform on the peripheral
 * @param[in] led : string name of the led GREEN | RED
 * @param[in] onState: if Write command then set the led to this state
 * @param[in] uid: Unique command identifier.
 *
 **/
static void handleCncRequest(webResponse_tp p_webResponse,
                             int destination,
                             PERIPHERAL_e periEnum,
                             CNC_ACTION_e commandEnum,
                             const char *led,
                             uint32_t onState,
                             uint32_t uid);

/**
 * @fn handleCNCLocalLED
 *
 * @brief Using parameters prepare read the led state register
 *
 * @param[out] p_webResponse: Web response
 * @param[in] periEnum: sensor board peripheral, must be MCU for led control
 * @param[in] commandEnum: what command to preform on the peripheral
 * @param[in] led : string name of the led GREEN | RED
 * @param[in] onState: if Write command then set the led to this state
 * @param[in] uid: Unique command identifier.
 *
 **/

static void handleCNCLocalLED(webResponse_tp p_webResponse,
                              PERIPHERAL_e periEnum,
                              CNC_ACTION_e commandEnum,
                              const char *led,
                              uint32_t onState,
                              uint32_t uid);

webResponse_tp webLedStateGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(const char *, led, obj, json_object_get_string);
    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    handleCncRequest(p_webResponse, destinationVal, PER_MCU, CNC_ACTION_READ, led, 0, uid);

    return p_webResponse;
}

webResponse_tp webLedStateSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(const char *, led, obj, json_object_get_string);
    GET_REQ_KEY_VALUE(bool, state, obj, json_object_get_boolean);
    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    handleCncRequest(p_webResponse, destinationVal, PER_MCU, CNC_ACTION_WRITE, led, state, uid);

    return p_webResponse;
}

void ledRequestHelper(
    webResponse_tp p_webResponse, json_object *jsonValuesArray, int destination, const char *led, uint32_t boolOn) {
    json_object *jsonValueObj = json_object_new_object();

    json_object *jsonDest = json_object_new_int(destination);
    json_object_object_add_ex(jsonValueObj, "destination", jsonDest, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonOnState = json_object_new_boolean(boolOn);
    // The string off the stack (led) gets corrupted before the JSON is sent so lets use
    // a static name
    if (stricmp(led, "RED") == 0) {
        json_object_object_add_ex(jsonValueObj, "RED", jsonOnState, JSON_C_OBJECT_KEY_IS_CONSTANT);
    } else if (stricmp(led, "GREEN") == 0) {
        json_object_object_add_ex(jsonValueObj, "GREEN", jsonOnState, JSON_C_OBJECT_KEY_IS_CONSTANT);
    }
    json_object *next = jsonValueObj;
    json_object_array_add(jsonValuesArray, next);
}

static void handleCNCLocalLED(webResponse_tp p_webResponse,
                              PERIPHERAL_e periEnum,
                              CNC_ACTION_e commandEnum,
                              const char *led,
                              uint32_t onState,
                              uint32_t uid) {

    char buf[MAX_JSON_ERR_BUF_BYTES];
    registerInfo_t regInfo = {.mbId = GREEN_LED_ON, .type = DATA_UINT, .u.dataUint = onState};
    if (strcmp(led, "GREEN") == 0) {
        regInfo.mbId = GREEN_LED_ON;
    } else if (strcmp(led, "RED") == 0) {
        regInfo.mbId = RED_LED_ON;
    } else {
        // error
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;

        json_object *jsonResult = json_object_new_string("Value Error");
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);

        snprintf(buf, sizeof(buf), "led value must be either RED or GREEN");
        json_object *jsonError = json_object_new_string(buf);
        json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonError, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return;
    }
    if (commandEnum == CNC_ACTION_READ) {
        registerRead(&regInfo);

    } else if (commandEnum == CNC_ACTION_WRITE) {
        registerWrite(&regInfo);
    } else {
        assert(0);
    }

    json_object *jsonValuesArray = json_object_new_array();

    json_object *jsonResult = json_object_new_string("success");

    onState = regInfo.u.dataUint;

    json_object_object_add_ex(p_webResponse->jsonResponse, "values", jsonValuesArray, JSON_C_OBJECT_KEY_IS_CONSTANT);
    ledRequestHelper(p_webResponse, jsonValuesArray, MAIN_BOARD_ID, led, onState);

    p_webResponse->httpCode = HTTP_OK;

    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
}

static void handleCncRequest(webResponse_tp p_webResponse,
                             int destination,
                             PERIPHERAL_e periEnum,
                             CNC_ACTION_e commandEnum,
                             const char *led,
                             uint32_t onState,
                             uint32_t xinfo) {
    char buf[MAX_JSON_ERR_BUF_BYTES];
    cncMsgPayload_t cncPayloadCmd = {
        .cncMsgPayloadHeader.peripheral = periEnum, .cncMsgPayloadHeader.action = commandEnum, .value = onState, .cncMsgPayloadHeader.cncActionData.result = UNKNOWN_RESULT};

    if (strcmp(led, "GREEN") == 0) {
        cncPayloadCmd.cncMsgPayloadHeader.addr = SB_GREEN_LED_ON;
    } else if (strcmp(led, "RED") == 0) {
        cncPayloadCmd.cncMsgPayloadHeader.addr = SB_RED_LED_ON;
    } else {
        // error

        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;

        json_object *jsonResult = json_object_new_string("Value Error");
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);

        snprintf(buf, sizeof(buf), "led value must be either RED or GREEN");
        json_object *jsonError = json_object_new_string(buf);
        json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonError, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return;
    }
    webCncCbId_t webCncCbIdCmd = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};

    json_object *jsonResult = NULL;
    uint32_t onResult = 0;
    uint32_t minDest = 0;
    uint32_t maxDest = 0;

    if (destination == MAIN_BOARD_ID) {
        return handleCNCLocalLED(p_webResponse, periEnum, commandEnum, led, onState, xinfo);
    }

    if (destination == DESTINATION_ALL) {
        minDest = 0;
        maxDest = MAX_CS_ID;
    } else {
        minDest = destination;
        maxDest = destination + 1;
    }
    json_object *jsonValuesArray = json_object_new_array();
    for (int i = minDest; i < maxDest; i++) {
        if (sensorBoardPresent(i) == true) {
            cncSendMsg(i, SPICMD_CNC, (cncPayload_tp)&cncPayloadCmd, xinfo, webCncRequestCB, (uint32_t)&webCncCbIdCmd);
            uint32_t startTick = HAL_GetTick();
            uint32_t osResultCmd = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
            uint32_t cncResult = webCncCbIdCmd.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;
            uint32_t onState = webCncCbIdCmd.p_payload->cmd.value;
            onResult = webCncCbIdCmd.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result;

            if (osResultCmd == TASK_NOTIFY_OK) {
                if (cncResult == 0) {
                    ledRequestHelper(p_webResponse, jsonValuesArray, i, led, onState);
                } else {
                    uint32_t delta = HAL_GetTick() - startTick;
                    snprintf(buf, sizeof(buf), "cnc error: board %d did not respond in time, delta %lu msec result=%ld", i, delta, webCncCbIdCmd.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
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

    json_object_object_add_ex(p_webResponse->jsonResponse, "values", jsonValuesArray, JSON_C_OBJECT_KEY_IS_CONSTANT);

    if (jsonResult != NULL) {
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
        snprintf(buf, sizeof(buf), "ErrorCodes %ld (onState) ", onResult);
        json_object *jsonError = json_object_new_string(buf);
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonError, JSON_C_OBJECT_KEY_IS_CONSTANT);
    } else {
        p_webResponse->httpCode = HTTP_OK;
        json_object *jsonResult = json_object_new_string("success");
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    }
}
