/*
 * MB_handleADC.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 19, 2023
 *      Author: jzhang
 */

#include "peripherals/MB_handleADC.h"

#define CREATE_LEAD_LOOKUP_TABLES
#include "ads1298.h"
#undef CREATE_LEAD_LOOKUP_TABLES
#include "MB_cncHandleMsg.h"
#include "ctrlSpiCommTask.h"
#include "debugPrint.h"
#include "pwm.h"
#include "pwmPinConfig.h"
#include "webCliMisc.h"
#include "webCmdHandler.h"

#include <stdio.h>
#include <string.h>

#define CNC_BUF_SZ_BYTES 70
#define JSON_BUFFER_SZ_BYTES 20

#define MAX_ADC_FREQ_NOT_INCLUSIVE 1001 // max is 1Khz
#define MAX_DUTY_PERC_NOT_INCLUSIVE 101
#define FIX_POINT_DEC_100 100
#define MAX_DETECTION_MODE_VALUE 2

typedef enum {
    ADC1298_FREQUENCY_TYPE_KEY,
    ADC1298_NEG_LEAD_OFF_TYPE_KEY,
    ADC1298_POS_LEAD_OFF_TYPE_KEY,
    ADC1298_ILEAD_OFF_CURRENT_KEY,
    ADS1298_LEAD_OFF_DETECTION_KEY,
    ADS1298_LEAD_OFF_COMPARATOR_THRESHOLD_KEY,
    ADS1298_RLD_SENSE_FN_ENABLE_KEY,
    ADS1298_RLD_SENSE_FN_STATUS_KEY,
    ADS1298_RLD_CONNECT_STATUS_KEY,
    ADS1298_STATUS_24BIT_KEY,
    ADS1298_PACE_DETECT_RESET_KEY,
    ADS1298_PACE_DETECT_GET_KEY,
    NUM_ADC1298_RETURN_TYPES
} adc1298ReturnFields_e;

#define ADD_J_OBJECT(jobj, keyName, str)                                                                               \
    {                                                                                                                  \
        json_object *jstr = json_object_new_string(str);                                                               \
        json_object_object_add_ex(jobj, keyName, jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);                                 \
    }

extern __DTCMRAM__ dbCommThreadInfo_t dbCommThreads[MAX_CS_ID];
bool testDutyValue(webResponse_tp p_webResponse, int destination);
bool testFreqValue(webResponse_tp p_webResponse, int destination);

bool testDutyValue(webResponse_tp p_webResponse, int duty) {
    TEST_VALUE(duty, 0, MAX_DUTY_PERC_NOT_INCLUSIVE)
}

bool testFreqValue(webResponse_tp p_webResponse, int freq) {
    TEST_VALUE(freq, 0, MAX_ADC_FREQ_NOT_INCLUSIVE)
}

bool testDetectionModeValue(webResponse_tp p_webResponse, int mode) {
    // Detection mode is either 0 current or 1 resistor.
    TEST_VALUE(mode, 0, MAX_DETECTION_MODE_VALUE)
}

bool testFreqTypeValue(webResponse_tp p_webResponse, int type) {
    if ((type != LOFFREG_FLEAD_OFF_AC_DETECTION) && (type != LOFFREG_FLEAD_OFF_DC_DETECTION)) {
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
        char buf[HTTP_BUFFER_SIZE];
        json_object *jsonResult = json_object_new_string("type value error");
        snprintf(buf,
                 sizeof(buf),
                 "Value %d out of range [%d,%d]",
                 type,
                 LOFFREG_FLEAD_OFF_AC_DETECTION,
                 LOFFREG_FLEAD_OFF_DC_DETECTION);
        json_object *jsonDestError = json_object_new_string(buf);
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonDestError, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return false;
    }
    return true;
}

webResponse_tp webAdcStart(const char *jsonStr, int strLen) {
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    HAL_GPIO_WritePin(pwmMap[ADC_PIN].gpioPort, pwmMap[ADC_PIN].gpioPin, 1);

    p_webResponse->httpCode = HTTP_OK;
    struct json_object *jsonResult = json_object_new_string("success");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    WEB_CMD_PARAM_CLEANUP;

    return p_webResponse;
}

webResponse_tp webAdcStop(const char *jsonStr, int strLen) {
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    HAL_GPIO_WritePin(pwmMap[ADC_PIN].gpioPort, pwmMap[ADC_PIN].gpioPin, 0);

    p_webResponse->httpCode = HTTP_OK;
    struct json_object *jsonResult = json_object_new_string("success");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    WEB_CMD_PARAM_CLEANUP;

    return p_webResponse;
}

webResponse_tp webAdcGetDuty(const char *jsonStr, int strLen) {

    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    // Main board ADC duty register
    registerInfo_t regInfo = {.mbId = ADC_READ_DUTY, .type = DATA_UINT};

    RETURN_CODE rc = registerRead(&regInfo);
    assert(rc == 0);

    json_object *jsonObjectInt = json_object_new_int(regInfo.u.dataUint);
    json_object_object_add_ex(p_webResponse->jsonResponse, "duty", jsonObjectInt, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonUnitStr = json_object_new_string("%");
    json_object_object_add_ex(p_webResponse->jsonResponse, "unit", jsonUnitStr, JSON_C_OBJECT_KEY_IS_CONSTANT);
    p_webResponse->httpCode = HTTP_OK;
    struct json_object *jsonResult = json_object_new_string("success");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    WEB_CMD_PARAM_CLEANUP;
    return p_webResponse;
}
webResponse_tp webAdcGetFreq(const char *jsonStr, int strLen) {
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    // Main board ADC frequency register
    registerInfo_t regInfo = {.mbId = ADC_READ_RATE, .type = DATA_UINT};

    RETURN_CODE rc = registerRead(&regInfo);
    assert(rc == 0);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    json_object *jsonObjectInt = json_object_new_int(uid);
    json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonObjectInt, JSON_C_OBJECT_KEY_IS_CONSTANT);

    jsonObjectInt = json_object_new_int(regInfo.u.dataUint / FIX_POINT_DEC_100);
    json_object_object_add_ex(p_webResponse->jsonResponse, "frequency", jsonObjectInt, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonUnitStr = json_object_new_string("hZ");
    json_object_object_add_ex(p_webResponse->jsonResponse, "unit", jsonUnitStr, JSON_C_OBJECT_KEY_IS_CONSTANT);

    p_webResponse->httpCode = HTTP_OK;
    struct json_object *jsonResult = json_object_new_string("success");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    WEB_CMD_PARAM_CLEANUP;

    return p_webResponse;
}
webResponse_tp webAdcSetDuty(const char *jsonStr, int strLen) {
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    json_object *jsonObjectInt = json_object_new_int(uid);
    json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonObjectInt, JSON_C_OBJECT_KEY_IS_CONSTANT);

    GET_REQ_KEY_VALUE(int, duty, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP;
    if (!testDutyValue(p_webResponse, duty)) {
        return p_webResponse;
    }

    // Main board ADC duty register
    registerInfo_t regInfo = {.mbId = ADC_READ_DUTY, .type = DATA_UINT};
    regInfo.u.dataUint = duty;

    RETURN_CODE rc = registerWrite(&regInfo);
    assert(rc == 0);

    jsonObjectInt = json_object_new_int(duty);
    json_object_object_add_ex(p_webResponse->jsonResponse, "duty", jsonObjectInt, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonUnitStr = json_object_new_string("%");
    json_object_object_add_ex(p_webResponse->jsonResponse, "unit", jsonUnitStr, JSON_C_OBJECT_KEY_IS_CONSTANT);

    p_webResponse->httpCode = HTTP_OK;
    struct json_object *jsonResult = json_object_new_string("success");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);

    return p_webResponse;
}

webResponse_tp webAdcSetFreq(const char *jsonStr, int strLen) {
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    json_object *jsonObjectInt = json_object_new_int(uid);
    json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonObjectInt, JSON_C_OBJECT_KEY_IS_CONSTANT);

    GET_REQ_KEY_VALUE(int, frequency, obj, json_object_get_int);
    if (!testFreqValue(p_webResponse, frequency)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    uint32_t freqScaled = frequency * 100;

    // Main board ADC frequency register
    registerInfo_t regInfo = {.mbId = ADC_READ_RATE, .type = DATA_UINT};
    regInfo.u.dataUint = freqScaled;

    RETURN_CODE rc = registerWrite(&regInfo);
    assert(rc == 0);

    jsonObjectInt = json_object_new_int(frequency);
    json_object_object_add_ex(p_webResponse->jsonResponse, "frequency", jsonObjectInt, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonUnitStr = json_object_new_string("hZ");
    json_object_object_add_ex(p_webResponse->jsonResponse, "unit", jsonUnitStr, JSON_C_OBJECT_KEY_IS_CONSTANT);

    p_webResponse->httpCode = HTTP_OK;
    struct json_object *jsonResult = json_object_new_string("success");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    WEB_CMD_PARAM_CLEANUP

    return p_webResponse;
}

typedef struct {
    const char *adcType;
    const char *adcUnit;
} returnFields_t;

returnFields_t adc1298FieldsLookup[NUM_ADC1298_RETURN_TYPES] = {
    [ADC1298_FREQUENCY_TYPE_KEY] = {.adcType = "type", .adcUnit = NULL},
    [ADC1298_NEG_LEAD_OFF_TYPE_KEY] = {.adcType = "type", .adcUnit = NULL},
    [ADC1298_POS_LEAD_OFF_TYPE_KEY] = {.adcType = "type", .adcUnit = NULL},
    [ADC1298_ILEAD_OFF_CURRENT_KEY] = {.adcType = "magnitude", .adcUnit = "nA"},
    [ADS1298_LEAD_OFF_DETECTION_KEY] = {.adcType = "mode", .adcUnit = NULL},
    [ADS1298_LEAD_OFF_COMPARATOR_THRESHOLD_KEY] = {.adcType = "threshold", .adcUnit = "m%"},
    [ADS1298_RLD_SENSE_FN_ENABLE_KEY] = {.adcType = "enable", .adcUnit = NULL},
    [ADS1298_RLD_CONNECT_STATUS_KEY] = {.adcType = "Right Leg Drive Connected", .adcUnit = NULL},
    [ADS1298_STATUS_24BIT_KEY] = {.adcType = "connected", .adcUnit = NULL},
    [ADS1298_PACE_DETECT_RESET_KEY] = {.adcType = "action", .adcUnit = NULL},
    [ADS1298_PACE_DETECT_GET_KEY] = {.adcType = "Pace Detect", .adcUnit = NULL}};

static void ads1298RequestHelper(webResponse_tp p_webResponse,
                                 json_object *jsonValuesArray,
                                 int destination,
                                 PERIPHERAL_e peripheral,
                                 webCncCbId_t webCncCbId,
                                 adc1298ReturnFields_e lookupIdx) {
    json_object *jsonDacRespObj = json_object_new_object();
    json_object *jsonDest = json_object_new_int(destination);
    json_object_object_add_ex(p_webResponse->jsonResponse, "destination", jsonDest, JSON_C_OBJECT_KEY_IS_CONSTANT);

    const char **lookupTable = NULL;

    switch (lookupIdx) {
    case ADS1298_RLD_CONNECT_STATUS_KEY:
        json_object *jsonValue = json_object_new_int(webCncCbId.p_payload->cmd.value);
        json_object_object_add_ex(
            jsonDacRespObj, adc1298FieldsLookup[lookupIdx].adcType, jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

        if (adc1298FieldsLookup[lookupIdx].adcUnit != NULL) {
            json_object *jsonUnit = json_object_new_string(adc1298FieldsLookup[lookupIdx].adcUnit);
            json_object_object_add_ex(jsonDacRespObj, "unit", jsonUnit, JSON_C_OBJECT_KEY_IS_CONSTANT);
        }
        json_object_array_add(jsonValuesArray, jsonDacRespObj);
        break;

    case ADS1298_STATUS_24BIT_KEY:
        uint32_t statusBitValue = webCncCbId.p_payload->cmd.value;
        DPRINTF_CMD_STREAM_VERBOSE("ADS1298_STATUS_24BIT_KEY value = 0x%x\r\n", statusBitValue)

        ADD_J_OBJECT(
            jsonDacRespObj, "GPIO 4", (statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_GPIO4]) ? "ON" : "OFF");
        ADD_J_OBJECT(
            jsonDacRespObj, "GPIO 5", (statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_GPIO5]) ? "ON" : "OFF");
        ADD_J_OBJECT(
            jsonDacRespObj, "GPIO 6", (statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_GPIO6]) ? "ON" : "OFF");
        ADD_J_OBJECT(
            jsonDacRespObj, "GPIO 7", (statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_GPIO7]) ? "ON" : "OFF");
        ADD_J_OBJECT(jsonDacRespObj,
                     "ECG V1",
                     !(statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_ECG_V1]) ? "CONNECTED" : "DISCONNECTED");
        ADD_J_OBJECT(jsonDacRespObj,
                     "ECG V2",
                     !(statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_ECG_V2]) ? "CONNECTED" : "DISCONNECTED");
        ADD_J_OBJECT(jsonDacRespObj,
                     "ECG V3",
                     !(statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_ECG_V3]) ? "CONNECTED" : "DISCONNECTED");
        ADD_J_OBJECT(jsonDacRespObj,
                     "ECG V4",
                     !(statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_ECG_V4]) ? "CONNECTED" : "DISCONNECTED");
        ADD_J_OBJECT(jsonDacRespObj,
                     "ECG V5",
                     !(statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_ECG_V5]) ? "CONNECTED" : "DISCONNECTED");
        ADD_J_OBJECT(jsonDacRespObj,
                     "ECG V6",
                     !(statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_ECG_V6]) ? "CONNECTED" : "DISCONNECTED");
        ADD_J_OBJECT(jsonDacRespObj,
                     "ECG LL",
                     !(statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_ECG_LL]) ? "CONNECTED" : "DISCONNECTED");
        ADD_J_OBJECT(jsonDacRespObj,
                     "ECG LA",
                     !(statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_ECG_LA]) ? "CONNECTED" : "DISCONNECTED");
        ADD_J_OBJECT(jsonDacRespObj,
                     "ECG RA",
                     !(statusBitValue & STATUS_BIT_LOCATION_MAP[STATUSBIT_ECG_RA]) ? "CONNECTED" : "DISCONNECTED");

        json_object_array_add(jsonValuesArray, jsonDacRespObj);

        break;
    case ADS1298_PACE_DETECT_GET_KEY:
        // fall through
    case ADS1298_RLD_SENSE_FN_ENABLE_KEY:
        // fall through
    case ADS1298_LEAD_OFF_COMPARATOR_THRESHOLD_KEY:
        // fall through
    case ADC1298_FREQUENCY_TYPE_KEY:
        // fall through
    case ADS1298_LEAD_OFF_DETECTION_KEY:
        // fall through
    case ADC1298_ILEAD_OFF_CURRENT_KEY: {
        json_object *jsonValue = json_object_new_int(webCncCbId.p_payload->cmd.value);
        json_object_object_add_ex(
            jsonDacRespObj, adc1298FieldsLookup[lookupIdx].adcType, jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

        if (adc1298FieldsLookup[lookupIdx].adcUnit != NULL) {
            json_object *jsonUnit = json_object_new_string(adc1298FieldsLookup[lookupIdx].adcUnit);
            json_object_object_add_ex(jsonDacRespObj, "unit", jsonUnit, JSON_C_OBJECT_KEY_IS_CONSTANT);
        }
        json_object_array_add(jsonValuesArray, jsonDacRespObj);
    } break;
    case ADC1298_NEG_LEAD_OFF_TYPE_KEY:

        lookupTable = lookupNegativeLeads;
        // fall through
    case ADC1298_POS_LEAD_OFF_TYPE_KEY:
        if (lookupTable == NULL) {
            lookupTable = lookupPositiveLeads;
        }
        uint8_t pvalue = webCncCbId.p_payload->cmd.value;
        for (int i = 0; i < BITS_IN_BYTE; i++) {
            json_object *jstate = json_object_new_string(((pvalue & (1 << i)) == 0) ? "ON" : "OFF");
            json_object_object_add_ex(jsonDacRespObj, lookupTable[i], jstate, JSON_C_OBJECT_KEY_IS_CONSTANT);
        }
        json_object_array_add(jsonValuesArray, jsonDacRespObj);

        break;
    case ADS1298_PACE_DETECT_RESET_KEY: {
        json_object *jsonValue = json_object_new_string("complete");
        json_object_object_add_ex(
            jsonDacRespObj, adc1298FieldsLookup[lookupIdx].adcType, jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object_array_add(jsonValuesArray, jsonDacRespObj);
    } break;

    default:
        assert(0);
        break;
    }
}

static void handleCncCommon(webResponse_tp p_webResponse,
                            uint32_t destination,
                            CNC_ACTION_e commandEnum,
                            PERIPHERAL_e peripheral,
                            uint32_t address,
                            uint32_t value,
                            uint32_t xinfo,
                            spiDbMbCmd_e cncCmd,
                            adc1298ReturnFields_e lookupIdx) {
    cncMsgPayload_t cncPayload = {
         .cncMsgPayloadHeader.peripheral = peripheral, .cncMsgPayloadHeader.action = commandEnum, .cncMsgPayloadHeader.addr = address, .value = value, .cncMsgPayloadHeader.cncActionData.result = 0xFF};

    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};

    json_object *jsonValuesArray = json_object_new_array();
    json_object *jsonResult = NULL;

    char buf[CNC_BUF_SZ_BYTES];

    if (dbCommThreads[destination].dbCommState.enabled == true) {
        DPRINTF_CMD_STREAM_VERBOSE("%s, command = %d destination = %d, peripheral =%s addr = %d, value = %d\r\n",
                                   (cncCmd == SPICMD_CNC) ? "SPICMD_CNC" : "SPICMD_CNC_SHORT",
                                   commandEnum,
                                   destination,
                                   PERIPHERAL_e_Strings[commandEnum],
                                   address,
                                   value);

        webCncCbId.cmdResponse = UNKNOWN_COMMAND;
        cncSendMsg(destination, cncCmd, (cncPayload_tp)&cncPayload, xinfo, webCncRequestCB, (uint32_t)&webCncCbId);
        uint32_t startTick = HAL_GetTick();
        uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);

        if (osResult == TASK_NOTIFY_OK) {
            uint16_t result = (cncCmd==SPICMD_CNC) ? webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result : webCncCbId.cmdResponse;
            if (result == NO_ERROR) {

                if (cncCmd != SPICMD_CNC) {
                    webCncCbId.p_payload->cmd.value = value;
                }
                DPRINTF_CMD_STREAM_VERBOSE("Value received: %d\r\n", webCncCbId.p_payload->cmd.value);
                ads1298RequestHelper(p_webResponse, jsonValuesArray, destination, peripheral, webCncCbId, lookupIdx);

            } else {
                uint32_t delta = HAL_GetTick() - startTick;
                snprintf(buf, sizeof(buf), "cnc error: board %ld did not respond in time, delta %lu msec result=%ld", destination, delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
                jsonResult = json_object_new_string(buf);
            }
        } else {
            // Must not reach here. THe CNC task must timeout first as this task is transient
            assert(false);
        }
    }

    json_object_object_add_ex(p_webResponse->jsonResponse, "values", jsonValuesArray, JSON_C_OBJECT_KEY_IS_CONSTANT);

    CHECK_ERRORS
}

webResponse_tp webRLDriveConnectedStatusGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    PER_MCU,
                    SB_ECG_LEG_LEAD_CONNECT,
                    0,
                    uid,
                    SPICMD_CNC,
                    ADS1298_RLD_CONNECT_STATUS_KEY);
    return p_webResponse;
}

webResponse_tp webRLDriveFunctionGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    PER_ADC,
                    ADS1298_RLD_SENSE_FN_ENABLE,
                    0,
                    uid,
                    SPICMD_CNC,
                    ADS1298_RLD_SENSE_FN_ENABLE_KEY);

    return p_webResponse;
}

webResponse_tp webRLDriveFunctionSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(bool, enable, obj, json_object_get_boolean);
    if (!testDetectionModeValue(p_webResponse, enable)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    WEB_CMD_PARAM_CLEANUP
    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_WRITE,
                    PER_ADC,
                    ADS1298_RLD_SENSE_FN_ENABLE,
                    enable,
                    uid,
                    SPICMD_CNC,
                    ADS1298_RLD_SENSE_FN_ENABLE_KEY);

    return p_webResponse;
}

webResponse_tp webLeadOffComparatorThresholdSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, threshold, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP
    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_WRITE,
                    PER_ADC,
                    ADS1298_LEAD_OFF_COMPARATOR_THRESHOLD,
                    threshold,
                    uid,
                    SPICMD_CNC,
                    ADS1298_LEAD_OFF_COMPARATOR_THRESHOLD_KEY);

    return p_webResponse;
}

webResponse_tp webLeadOffComparatorThresholdGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP
    PERIPHERAL_e peripheral = PER_ADC;

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    peripheral,
                    ADS1298_LEAD_OFF_COMPARATOR_THRESHOLD,
                    0,
                    uid,
                    SPICMD_CNC,
                    ADS1298_LEAD_OFF_COMPARATOR_THRESHOLD_KEY);

    return p_webResponse;
}

webResponse_tp webLeadOffDetectionModeSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, mode, obj, json_object_get_int);
    if (!testDetectionModeValue(p_webResponse, mode)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_ADC;
    WEB_CMD_PARAM_CLEANUP
    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_WRITE,
                    peripheral,
                    ADS1298_LEAD_OFF_DETECTION_MODE,
                    mode,
                    uid,
                    SPICMD_CNC,
                    ADS1298_LEAD_OFF_DETECTION_KEY);

    return p_webResponse;
}

webResponse_tp webLeadOffDetectionModeGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP
    PERIPHERAL_e peripheral = PER_ADC;

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    peripheral,
                    ADS1298_LEAD_OFF_DETECTION_MODE,
                    0,
                    uid,
                    SPICMD_CNC,
                    ADS1298_LEAD_OFF_DETECTION_KEY);

    return p_webResponse;
}

webResponse_tp webIleadOffCurrentGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP
    PERIPHERAL_e peripheral = PER_ADC;

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    peripheral,
                    ADS1298_ILEAD_OFF_CURRENT_TYPE,
                    0,
                    uid,
                    SPICMD_CNC,
                    ADC1298_ILEAD_OFF_CURRENT_KEY);

    return p_webResponse;
}
webResponse_tp webIleadOffCurrentSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, magnitude, obj, json_object_get_int);

    PERIPHERAL_e peripheral = PER_ADC;
    WEB_CMD_PARAM_CLEANUP
    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_WRITE,
                    peripheral,
                    ADS1298_ILEAD_OFF_CURRENT_TYPE,
                    magnitude,
                    uid,
                    SPICMD_CNC,
                    ADC1298_ILEAD_OFF_CURRENT_KEY);

    return p_webResponse;
}
webResponse_tp webEcg12FleadOffFreqSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, type, obj, json_object_get_int);
    if (!testFreqTypeValue(p_webResponse, type)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    PERIPHERAL_e peripheral = PER_ADC;
    WEB_CMD_PARAM_CLEANUP
    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_WRITE,
                    peripheral,
                    ADS1298_LEAD_OFF_FREQ_TYPE,
                    type,
                    uid,
                    SPICMD_CNC,
                    ADC1298_FREQUENCY_TYPE_KEY);

    return p_webResponse;
}

webResponse_tp webEcg12FleadOffFreqGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP
    PERIPHERAL_e peripheral = PER_ADC;

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    peripheral,
                    ADS1298_LEAD_OFF_FREQ_TYPE,
                    destinationVal,
                    uid,
                    SPICMD_CNC,
                    ADC1298_FREQUENCY_TYPE_KEY);
    return p_webResponse;
}

webResponse_tp webLeadOffNegativeStateGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    PERIPHERAL_e peripheral = PER_ADC;

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    peripheral,
                    ADS1209_LEAD_OFF_NEGATIVE_STATE,
                    0,
                    uid,
                    SPICMD_CNC,
                    ADC1298_NEG_LEAD_OFF_TYPE_KEY);

    return p_webResponse;
}

webResponse_tp webLeadOffPositiveStateGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    PERIPHERAL_e peripheral = PER_ADC;

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    peripheral,
                    ADS1209_LEAD_OFF_POSITIVE_STATE,
                    0,
                    uid,
                    SPICMD_CNC,
                    ADC1298_POS_LEAD_OFF_TYPE_KEY);

    return p_webResponse;
}

webResponse_tp webLeadConnectionStatesGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    PER_MCU,
                    SB_ECG_STAT_DATA,
                    0,
                    uid,
                    SPICMD_CNC,
                    ADS1298_STATUS_24BIT_KEY);
    return p_webResponse;
}

webResponse_tp webPaceDetectReset(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    PER_ADC,
                    ADS1298_PACE_DETECT_RESET,
                    0,
                    uid,
                    SPICMD_CNC,
                    ADS1298_PACE_DETECT_RESET_KEY);
    return p_webResponse;
}

webResponse_tp webPaceDetectGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncCommon(p_webResponse,
                    destinationVal,
                    CNC_ACTION_READ,
                    PER_ADC,
                    ADS1298_PACE_DETECT_GET,
                    0,
                    uid,
                    SPICMD_CNC,
                    ADS1298_PACE_DETECT_GET_KEY);
    return p_webResponse;
}
