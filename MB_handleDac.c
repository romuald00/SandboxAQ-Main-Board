/*
 * MB_handleDac.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 7, 2023
 *      Author: jzhang
 */

#include "peripherals/MB_handleDac.h"
#include "MB_cncHandleMsg.h"
#include "cmdAndCtrl.h"
#include "ctrlSpiCommTask.h"
#include "dbCommTask.h"
#include "webCliMisc.h"
#include "webCmdHandler.h"
#include <stdio.h>
#include <string.h>

extern __DTCMRAM__ dbCommThreadInfo_t dbCommThreads[MAX_CS_ID];

#define JSON_STR_BUFFER_SZ_BYTES 160

/**
 * @fn
 *
 * @brief Handle the Command and control request. May iterate over all sensor boards.
 *
 * @param p_webResponse: fill with json result
 * @param destination: sensor board identifier
 * @param slot: identify which sensor group on board.
 * @param periEnum: identify the peripheral
 * @param commandEnum: identify the peripheral command
 * @param value: value only used on write requests
 * @param xinfo: User defined identifier.
 **/
static void handleCncRequest(webResponse_tp p_webResponse,
                             int destination,
                             int slot,
                             PERIPHERAL_e periEnum,
                             CNC_ACTION_e commandEnum,
                             uint32_t value,
                             uint32_t xinfo);

#define PER_DAC_COMPENSATION(slot) ((slot == 0) ? PER_DAC_0_COMPENSATION : PER_DAC_1_COMPENSATION)
#define PER_DAC_EXCITATION(slot) ((slot == 0) ? PER_DAC_0_EXCITATION : PER_DAC_1_EXCITATION)

bool testSensorSlotValue(webResponse_tp p_webResponse, int slot){TEST_VALUE(slot, 0, MAX_SLOT_ID)}

webResponse_tp webDacCompGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        json_tokener_free(jsonTok);
        json_object_put(obj);
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_DAC_COMPENSATION(slot);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    json_tokener_free(jsonTok);
    json_object_put(obj);

    handleCncRequest(p_webResponse, destinationVal, slot, peripheral, CNC_ACTION_READ, 0, uid);

    return p_webResponse;
}

webResponse_tp webDacCompSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal);
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        json_tokener_free(jsonTok);
        json_object_put(obj);
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_DAC_COMPENSATION(slot);

    GET_REQ_KEY_VALUE(int, voltage_mv, obj, json_object_get_int);
    if (!testVoltageValueComp(p_webResponse, voltage_mv)) {
        json_tokener_free(jsonTok);
        json_object_put(obj);
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    json_tokener_free(jsonTok);
    json_object_put(obj);

    handleCncRequest(p_webResponse, destinationVal, slot, peripheral, CNC_ACTION_WRITE, voltage_mv, uid);

    return p_webResponse;
}

webResponse_tp webDacExcGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        json_tokener_free(jsonTok);
        json_object_put(obj);
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_DAC_EXCITATION(slot);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    json_tokener_free(jsonTok);
    json_object_put(obj);

    handleCncRequest(p_webResponse, destinationVal, slot, peripheral, CNC_ACTION_READ, 0, uid);

    return p_webResponse;
}

webResponse_tp webDacExcSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        json_tokener_free(jsonTok);
        json_object_put(obj);
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_DAC_EXCITATION(slot);

    GET_REQ_KEY_VALUE(int, voltage_mv, obj, json_object_get_int);
    if (!testVoltageValueExc(p_webResponse, voltage_mv)) {
        json_tokener_free(jsonTok);
        json_object_put(obj);
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    json_tokener_free(jsonTok);
    json_object_put(obj);

    handleCncRequest(p_webResponse, destinationVal, slot, peripheral, CNC_ACTION_WRITE, voltage_mv, uid);

    return p_webResponse;
}

bool testVoltageValueExc(webResponse_tp p_webResponse, int voltage_mv) {
    TEST_VALUE(voltage_mv, 0, DAC_MAX5717_VREF)
}
bool testVoltageValueComp(webResponse_tp p_webResponse, int voltage_mv) {
    TEST_VALUE(voltage_mv, -DAC_MAX5717_VREF, DAC_MAX5717_VREF)
}

/**
 * @fn
 *
 * @brief fill jsonValues array with passed in data.
 *
 * @param jsonValuesArray: update array with data from passed in parameters
 * @param destination: sensor board identifier
 * @param slot: identify which sensor group on board.
 * @param webCncCbId_t: contains dac results
 **/

void dacRequestHelper(json_object *jsonValuesArray, int destination, int slot, webCncCbId_t webCncCbId) {
    json_object *jsonDacRespObj = json_object_new_object();

    json_object *jsonDest = json_object_new_int(destination);
    json_object_object_add_ex(jsonDacRespObj, "destination", jsonDest, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonSlot = json_object_new_int(slot);
    json_object_object_add_ex(jsonDacRespObj, "slot", jsonSlot, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonVoltage = json_object_new_int(webCncCbId.p_payload->cmd.value);
    json_object_object_add_ex(jsonDacRespObj, "voltage", jsonVoltage, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonUnit = json_object_new_string("mV");
    json_object_object_add_ex(jsonDacRespObj, "unit", jsonUnit, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *next = jsonDacRespObj;
    json_object_array_add(jsonValuesArray, next);
}

void handleCncRequest(webResponse_tp p_webResponse,
                      int destination,
                      int slot,
                      PERIPHERAL_e periEnum,
                      CNC_ACTION_e commandEnum,
                      uint32_t value,
                      uint32_t xinfo) {

    cncMsgPayload_t cncPayload = {
        .cncMsgPayloadHeader.peripheral = periEnum, .cncMsgPayloadHeader.action = commandEnum,.cncMsgPayloadHeader.addr = 0, .value = value, .cncMsgPayloadHeader.cncActionData.result = 0xFF};

    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = xinfo};

    json_object *jsonValuesArray = json_object_new_array();
    json_object *jsonResult = NULL;
    char buf[JSON_STR_BUFFER_SZ_BYTES];
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
            uint32_t startTick = HAL_GetTick();
            cncSendMsg(i, SPICMD_CNC, (cncPayload_tp)&cncPayload, xinfo, webCncRequestCB, (uint32_t)&webCncCbId);
            uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);

            if (osResult == TASK_NOTIFY_OK) {
                if (webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result == 0) {
                    dacRequestHelper(jsonValuesArray, i, slot, webCncCbId);
                } else {
                    uint32_t delta = HAL_GetTick() - startTick;
                    snprintf(buf, sizeof(buf), "cnc error %s: board %d did not respond in time, delta %lu msec result=%ld",__FUNCTION__, i, delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
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

    CHECK_ERRORS
}
