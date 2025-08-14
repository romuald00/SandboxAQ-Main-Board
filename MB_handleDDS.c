/*
 * MB_handleDDS.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 8, 2023
 *      Author: jzhang
 */

#define CREATE_STRING_DDS_LOOKUP_TABLE
#include "saqTarget.h"
#undef CREATE_STRING_DDS_LOOKUP_TABLE

#include "MB_cncHandleMsg.h"
#include "MB_gatherTask.h"
#include "ctrlSpiCommTask.h"
#include "ddsTrigTask.h"
#include "debugPrint.h"
#include "peripherals/MB_handleDAC.h"
#include "peripherals/MB_handleDDS.h"
#include "webCliMisc.h"
#include "webCmdHandler.h"

#include <stdio.h>
#include <string.h>

#define CNC_BUF_SZ_BYTES 70
#define JSON_BUFFER_SZ_BYTES 20
// 4 bits are used for Dwell, Interval and wait clocks values, value 0 = 16 clocks, 1=1...15=15 clocks
#define MAX_WAVEFORM_PARAMETER 17
#define MIN_WAVEFORM_PARAMETER 1
#define DAC_VALUE_NOT_PRESENT (-1)

extern const strLUT_t peripheralLUT[PER_MAX];
extern __DTCMRAM__ dbCommThreadInfo_t dbCommThreads[MAX_CS_ID];

typedef struct {
    const char *ddsType;
    const char *ddsUnit;
} ddsReturnFields_t;

typedef enum {
    DDS_FREQUENCY_KEY,
    DDS_AMPLITUDE_KEY,
    DDS_PHASE_KEY,
    DDS_WAVEFORM_KEY,
    DDS_ELONGATE_KEY,
    DDS_INTERVAL_KEY,
    DDS_DELAY_KEY,
    NUM_DDS_RETURN_TYPES
} ddsReturnFields_e;

#define MAP_DDS_KEY_TO_REGISTER_ADDR(i) (i + DDS_MAX_REGISTER_ADDR)
#define MAP_REGISTER_ADDR_TO_DDS_KEY(i) (i - DDS_MAX_REGISTER_ADDR)

ddsReturnFields_t ddsFieldsLookup[NUM_DDS_RETURN_TYPES] = {
    {.ddsType = "frequency", .ddsUnit = "hz"},
    {.ddsType = "amplitude", .ddsUnit = "m%"},
    {.ddsType = "phase", .ddsUnit = "millidegrees"},
    {.ddsType = "waveform", .ddsUnit = "NA"},
    {.ddsType = "elongate", .ddsUnit = "CLK_PERIODS"},
    {.ddsType = "interval", .ddsUnit = "CLK_PERIODS"},
    {.ddsType = "delay", .ddsUnit = "CLK_PERIODS"},

};

#define WEB_DDS_WAVEFORM_X_SET(NAME, JKEY)                                                                             \
    webResponse_tp webDDSWaveform##NAME##Set(const char *jsonStr, int strLen) {                                        \
        int destinationVal = 0;                                                                                        \
        WEB_CMD_PARAM_SETUP(jsonStr, strLen);                                                                          \
                                                                                                                       \
        GET_DESTINATION(destinationVal)                                                                                \
        GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);                                                        \
        if (!testSensorSlotValue(p_webResponse, slot)) {                                                               \
            WEB_CMD_PARAM_CLEANUP                                                                                      \
            return p_webResponse;                                                                                      \
        }                                                                                                              \
        PERIPHERAL_e peripheral = PER_MCG_DDS(slot);                                                                   \
                                                                                                                       \
        GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);                                                         \
                                                                                                                       \
        GET_REQ_KEY_VALUE(int, JKEY, obj, json_object_get_int);                                                        \
        WEB_CMD_PARAM_CLEANUP                                                                                          \
        if (!testWaveformParameterValue(p_webResponse, JKEY)) {                                                        \
            return p_webResponse;                                                                                      \
        }                                                                                                              \
                                                                                                                       \
        handleCncGenericRequest(p_webResponse,                                                                         \
                                destinationVal,                                                                        \
                                CNC_ACTION_WRITE,                                                                      \
                                peripheral,                                                                            \
                                DDS_WAVEFORM_##NAME##_BASE_ADDR,                                                       \
                                JKEY,                                                                                  \
                                uid,                                                                                   \
                                0,                                                                                     \
                                DDS_##NAME##_KEY,                                                                      \
                                false,                                                                                 \
                                false);                                                                                \
        return p_webResponse;                                                                                          \
    }                                                                                                                  \
    webResponse_tp webCoilDDSWaveform##NAME##Set(const char *jsonStr, int strLen) {                                    \
        int destinationVal = 0;                                                                                        \
        WEB_CMD_PARAM_SETUP(jsonStr, strLen);                                                                          \
                                                                                                                       \
        GET_DESTINATION(destinationVal)                                                                                \
        GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);                                                        \
        if (!testCoilDDSValue(p_webResponse, slot)) {                                                                  \
            WEB_CMD_PARAM_CLEANUP                                                                                      \
            return p_webResponse;                                                                                      \
        }                                                                                                              \
        PERIPHERAL_e peripheral = PER_MCG_DDS(slot);                                                                   \
                                                                                                                       \
        GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);                                                         \
                                                                                                                       \
        GET_REQ_KEY_VALUE(int, JKEY, obj, json_object_get_int);                                                        \
        WEB_CMD_PARAM_CLEANUP                                                                                          \
        if (!testWaveformParameterValue(p_webResponse, JKEY)) {                                                        \
            return p_webResponse;                                                                                      \
        }                                                                                                              \
                                                                                                                       \
        handleCncGenericRequest(p_webResponse,                                                                         \
                                destinationVal,                                                                        \
                                CNC_ACTION_WRITE,                                                                      \
                                peripheral,                                                                            \
                                DDS_WAVEFORM_##NAME##_BASE_ADDR,                                                       \
                                JKEY,                                                                                  \
                                uid,                                                                                   \
                                0,                                                                                     \
                                DDS_##NAME##_KEY,                                                                      \
                                false,                                                                                 \
                                true);                                                                                 \
        return p_webResponse;                                                                                          \
    }

#define WEB_DDS_WAVEFORM_X_GET(NAME)                                                                                   \
    webResponse_tp webDDSWaveform##NAME##Get(const char *jsonStr, int strLen) {                                        \
        int destinationVal = 0;                                                                                        \
        WEB_CMD_PARAM_SETUP(jsonStr, strLen);                                                                          \
                                                                                                                       \
        GET_DESTINATION(destinationVal)                                                                                \
        GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);                                                        \
        if (!testSensorSlotValue(p_webResponse, slot)) {                                                               \
            WEB_CMD_PARAM_CLEANUP                                                                                      \
            return p_webResponse;                                                                                      \
        }                                                                                                              \
        PERIPHERAL_e peripheral = PER_MCG_DDS(slot);                                                                   \
                                                                                                                       \
        GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);                                                         \
        WEB_CMD_PARAM_CLEANUP                                                                                          \
                                                                                                                       \
        handleCncGenericRequest(p_webResponse,                                                                         \
                                destinationVal,                                                                        \
                                CNC_ACTION_READ,                                                                       \
                                peripheral,                                                                            \
                                DDS_WAVEFORM_##NAME##_BASE_ADDR,                                                       \
                                0,                                                                                     \
                                uid,                                                                                   \
                                0,                                                                                     \
                                DDS_##NAME##_KEY,                                                                      \
                                false,                                                                                 \
                                false);                                                                                \
        return p_webResponse;                                                                                          \
    }                                                                                                                  \
    webResponse_tp webCoilDDSWaveform##NAME##Get(const char *jsonStr, int strLen) {                                    \
        int destinationVal = 0;                                                                                        \
        WEB_CMD_PARAM_SETUP(jsonStr, strLen);                                                                          \
                                                                                                                       \
        GET_DESTINATION(destinationVal)                                                                                \
        GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);                                                        \
        if (!testCoilDDSValue(p_webResponse, slot)) {                                                                  \
            WEB_CMD_PARAM_CLEANUP                                                                                      \
            return p_webResponse;                                                                                      \
        }                                                                                                              \
        PERIPHERAL_e peripheral = PER_MCG_DDS(slot);                                                                   \
                                                                                                                       \
        GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);                                                         \
        WEB_CMD_PARAM_CLEANUP                                                                                          \
                                                                                                                       \
        handleCncGenericRequest(p_webResponse,                                                                         \
                                destinationVal,                                                                        \
                                CNC_ACTION_READ,                                                                       \
                                peripheral,                                                                            \
                                DDS_WAVEFORM_##NAME##_BASE_ADDR,                                                       \
                                0,                                                                                     \
                                uid,                                                                                   \
                                0,                                                                                     \
                                DDS_##NAME##_KEY,                                                                      \
                                false,                                                                                 \
                                true);                                                                                 \
        return p_webResponse;                                                                                          \
    }

void handleStartStop(webResponse_tp p_webResponse, CNC_ACTION_e commandEnum);

#define PER_MCG_DDS(slot) (slot == 0 ? PER_DDS0_A : PER_DDS1_A)
#define PER_COIL_DDS(slot) (PER_DDS0_A + slot)

bool printSpi = false;

static void ddsRequestHelper(webResponse_tp p_webResponse,
                             json_object *jsonValuesArray,
                             int destination,
                             PERIPHERAL_e peripheral,
                             uint8_t dac,
                             webCncCbId_t webCncCbId,
                             ddsReturnFields_e lookupIdx,
                             bool coilBoard) {
    json_object *jsonDacRespObj = json_object_new_object();
    char jsonBuffer[JSON_BUFFER_SZ_BYTES];
    json_object *jsonDds = NULL;

    if (coilBoard) {
        int slot = peripheral - PER_DDS0_A;
        json_object *jsonSlot = json_object_new_int(slot);
        json_object_object_add_ex(jsonDacRespObj, "slot", jsonSlot, JSON_C_OBJECT_KEY_IS_CONSTANT);
        jsonDds = json_object_new_string(DDS_ID_e_Strings[slot]);
        json_object_object_add_ex(jsonDacRespObj, "dds", jsonDds, JSON_C_OBJECT_KEY_IS_CONSTANT);
    } else {
        int slot = peripheral == PER_DDS0_A ? 0 : 1;
        json_object *jsonSlot = json_object_new_int(slot);
        json_object_object_add_ex(jsonDacRespObj, "slot", jsonSlot, JSON_C_OBJECT_KEY_IS_CONSTANT);
        if (dac != DAC_VALUE_NOT_PRESENT) {
            json_object *jsonDac = json_object_new_int(dac);
            json_object_object_add_ex(jsonDacRespObj, "dac", jsonDac, JSON_C_OBJECT_KEY_IS_CONSTANT);
        }
        switch (dac) {
        case MCG_DDS_DAC_EXC: // Excitation Index
            snprintf(jsonBuffer, sizeof(jsonBuffer), "EXC%d", slot);
            jsonDds = json_object_new_string(jsonBuffer);
            json_object_object_add_ex(jsonDacRespObj, "dds", jsonDds, JSON_C_OBJECT_KEY_IS_CONSTANT);
            break;
        case MCG_DDS_DAC_COMP_A: // Comparator DDS Channel A
            snprintf(jsonBuffer, sizeof(jsonBuffer), "COMP%d_A", slot);
            json_object *jsonDds = json_object_new_string(jsonBuffer);
            json_object_object_add_ex(jsonDacRespObj, "dds", jsonDds, JSON_C_OBJECT_KEY_IS_CONSTANT);
            break;
        case MCG_DDS_DAC_COMP_B: // Comparator DDS Channel A
            snprintf(jsonBuffer, sizeof(jsonBuffer), "COMP%d_B", slot);
            jsonDds = json_object_new_string(jsonBuffer);
            json_object_object_add_ex(jsonDacRespObj, "dds", jsonDds, JSON_C_OBJECT_KEY_IS_CONSTANT);
            break;
        case MCG_DDS_DAC_COMP_C: // Comparator DDS Channel A
            snprintf(jsonBuffer, sizeof(jsonBuffer), "COMP%d_C", slot);
            jsonDds = json_object_new_string(jsonBuffer);
            json_object_object_add_ex(jsonDacRespObj, "dds", jsonDds, JSON_C_OBJECT_KEY_IS_CONSTANT);

        default:
            break;
        }
    }
    json_object *jsonDest = json_object_new_int(destination);
    json_object_object_add_ex(jsonDacRespObj, "destination", jsonDest, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonValue = json_object_new_int(webCncCbId.p_payload->cmd.value);
    json_object_object_add_ex(
        jsonDacRespObj, ddsFieldsLookup[lookupIdx].ddsType, jsonValue, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *jsonUnit = json_object_new_string(ddsFieldsLookup[lookupIdx].ddsUnit);
    json_object_object_add_ex(jsonDacRespObj, "unit", jsonUnit, JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_object *next = jsonDacRespObj;
    json_object_array_add(jsonValuesArray, next);
}

static void handleCncCommon(webResponse_tp p_webResponse,
                            uint32_t destination,
                            CNC_ACTION_e commandEnum,
                            PERIPHERAL_e peripheral,
                            uint32_t address,
                            uint32_t value,
                            uint32_t xinfo,
                            uint32_t baseDacOffsetValue,
                            ddsReturnFields_e returnField,
                            bool dacValuePresent,
                            bool coilBoard,
                            spiDbMbCmd_e cncCmd) {
    cncMsgPayload_t cncPayload = {
         .cncMsgPayloadHeader.peripheral = peripheral, .cncMsgPayloadHeader.action = commandEnum, .cncMsgPayloadHeader.addr = address, .value = value, .cncMsgPayloadHeader.cncActionData.result = 0xFF};

    webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};

    json_object *jsonValuesArray = json_object_new_array();
    json_object *jsonResult = NULL;

    char buf[CNC_BUF_SZ_BYTES];
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
            DPRINTF_CMD_STREAM_VERBOSE("%s, command = %d destination = %d, peripheral =%s addr = %d, value = %d\r\n",
                                       (cncCmd == SPICMD_CNC) ? "SPICMD_CNC" : "SPICMD_CNC_SHORT",
                                       commandEnum,
                                       destination,
                                       PERIPHERAL_e_Strings[commandEnum],
                                       address,
                                       value);
            printSpi = true;
            webCncCbId.cmdResponse = UNKNOWN_COMMAND;
            cncSendMsg(i, cncCmd, (cncPayload_tp)&cncPayload, xinfo, webCncRequestCB, (uint32_t)&webCncCbId);
            uint32_t startTick = HAL_GetTick();
            uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);

            if (osResult == TASK_NOTIFY_OK) {
                 uint16_t result = (cncCmd==SPICMD_CNC) ? webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result : webCncCbId.cmdResponse;
                 if (result == NO_ERROR) {
                     uint8_t dac = DAC_VALUE_NOT_PRESENT;
                     if(dacValuePresent) {
                         dac = (cncCmd==SPICMD_CNC) ?
                               webCncCbId.p_payload->cmd.cncMsgPayloadHeader.addr - baseDacOffsetValue :
                               address - baseDacOffsetValue;
                     }

                    if (cncCmd != SPICMD_CNC) {
                        webCncCbId.p_payload->cmd.value = value;
                    }
                    DPRINTF_CMD_STREAM_VERBOSE("Value received: %d\r\n", webCncCbId.p_payload->cmd.value);
                    ddsRequestHelper(
                        p_webResponse, jsonValuesArray, i, peripheral, dac, webCncCbId, returnField, coilBoard);

                } else {
                    uint32_t delta = HAL_GetTick() - startTick;
                     snprintf(buf, sizeof(buf), "cnc error: board %d did not respond in time, delta %lu msec result=%ld", i, delta, webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);
                     jsonResult = json_object_new_string(buf);
                 }
             } else {
                 // Must not reach here. THe CNC task must timeout first as this task is transient
                 assert(false);
             }
             printSpi = false;
         }
     }
     json_object_object_add_ex(p_webResponse->jsonResponse, "values", jsonValuesArray, JSON_C_OBJECT_KEY_IS_CONSTANT);

    CHECK_ERRORS
}

static void handleCncGenericRequest(webResponse_tp p_webResponse,
                                    uint32_t destination,
                                    CNC_ACTION_e commandEnum,
                                    PERIPHERAL_e peripheral,
                                    uint32_t address,
                                    uint32_t value,
                                    uint32_t xinfo,
                                    uint32_t baseDacOffsetValue,
                                    ddsReturnFields_e returnField,
                                    bool dacValuePresent,
                                    bool coilBoard) {

    handleCncCommon(p_webResponse,
                    destination,
                    commandEnum,
                    peripheral,
                    address,
                    value,
                    xinfo,
                    baseDacOffsetValue,
                    returnField,
                    dacValuePresent,
                    coilBoard,
                    SPICMD_CNC);
}

static void handleCncShortResponseRequest(webResponse_tp p_webResponse,
                                          uint32_t destination,
                                          CNC_ACTION_e commandEnum,
                                          PERIPHERAL_e peripheral,
                                          uint32_t address,
                                          uint32_t value,
                                          uint32_t xinfo,
                                          uint32_t baseDacOffsetValue,
                                          ddsReturnFields_e returnField,
                                          bool dacValuePresent,
                                          bool coilBoard) {
    handleCncCommon(p_webResponse,
                    destination,
                    commandEnum,
                    peripheral,
                    address,
                    value,
                    xinfo,
                    baseDacOffsetValue,
                    returnField,
                    dacValuePresent,
                    coilBoard,
                    SPICMD_CNC_SHORT);
}
webResponse_tp webDDSAmpGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_MCG_DDS(slot);

    GET_REQ_KEY_VALUE(int, dac, obj, json_object_get_int);
    if (!testDacValue(p_webResponse, dac)) {
        json_tokener_free(jsonTok);
        json_object_put(obj);
        return p_webResponse;
    }
    uint32_t address = dac + DDS_AMPLITUDE_BASE_ADDR;

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_READ,
                            peripheral,
                            address,
                            0,
                            uid,
                            DDS_AMPLITUDE_BASE_ADDR,
                            DDS_AMPLITUDE_KEY,
                            true,
                            false);
    return p_webResponse;
}

webResponse_tp webDDSAmpSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_MCG_DDS(slot);

    GET_REQ_KEY_VALUE(int, dac, obj, json_object_get_int);
    if (!testDacValue(p_webResponse, dac)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    uint32_t address = dac + DDS_AMPLITUDE_BASE_ADDR;

    GET_REQ_KEY_VALUE(int, amplitude, obj, json_object_get_int);
    if (!testAmplitudeValue(p_webResponse, amplitude)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, shortResponse, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP
    if (shortResponse == 0) {
        handleCncGenericRequest(p_webResponse,
                                destinationVal,
                                CNC_ACTION_WRITE,
                                peripheral,
                                address,
                                amplitude,
                                uid,
                                DDS_AMPLITUDE_BASE_ADDR,
                                DDS_AMPLITUDE_KEY,
                                true,
                                false);
    } else {
        handleCncShortResponseRequest(p_webResponse,
                                      destinationVal,
                                      CNC_ACTION_WRITE,
                                      peripheral,
                                      address,
                                      amplitude,
                                      uid,
                                      DDS_AMPLITUDE_BASE_ADDR,
                                      DDS_AMPLITUDE_KEY,
                                      true,
                                      false);
    }
    return p_webResponse;
}

webResponse_tp webDDSFreqGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_MCG_DDS(slot);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_READ,
                            peripheral,
                            DDS_FREQUENCY_BASE_ADDR,
                            0,
                            uid,
                            0,
                            DDS_FREQUENCY_KEY,
                            false,
                            false);
    return p_webResponse;
}

webResponse_tp webDDSFreqSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_MCG_DDS(slot);

    GET_REQ_KEY_VALUE(int, frequency, obj, json_object_get_int);
    if (!testFrequencyValue(p_webResponse, frequency)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, shortResponse, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    if (shortResponse == 0) {
        handleCncGenericRequest(p_webResponse,
                                destinationVal,
                                CNC_ACTION_WRITE,
                                peripheral,
                                DDS_FREQUENCY_BASE_ADDR,
                                frequency,
                                uid,
                                0,
                                DDS_FREQUENCY_KEY,
                                false,
                                false);
    } else {
        handleCncShortResponseRequest(p_webResponse,
                                      destinationVal,
                                      CNC_ACTION_WRITE,
                                      peripheral,
                                      DDS_FREQUENCY_BASE_ADDR,
                                      frequency,
                                      uid,
                                      0,
                                      DDS_FREQUENCY_KEY,
                                      false,
                                      false);
    }
    return p_webResponse;
}

webResponse_tp webDDSPhaseGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_MCG_DDS(slot);
    GET_REQ_KEY_VALUE(int, dac, obj, json_object_get_int);
    if (!testDacValue(p_webResponse, dac)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    uint32_t address = dac + DDS_PHASE_BASE_ADDR;

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_READ,
                            peripheral,
                            address,
                            0,
                            uid,
                            DDS_PHASE_BASE_ADDR,
                            DDS_PHASE_KEY,
                            true,
                            false);

    return p_webResponse;
}

webResponse_tp webDDSPhaseSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_MCG_DDS(slot);

    GET_REQ_KEY_VALUE(int, dac, obj, json_object_get_int);
    if (!testDacValue(p_webResponse, dac)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    uint32_t address = dac + DDS_PHASE_BASE_ADDR;

    GET_REQ_KEY_VALUE(int, phase_mDeg, obj, json_object_get_int);
    if (!testPhaseValue(p_webResponse, phase_mDeg)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, shortResponse, obj, json_object_get_int);

    WEB_CMD_PARAM_CLEANUP

    if (shortResponse == 0) {
        handleCncGenericRequest(p_webResponse,
                                destinationVal,
                                CNC_ACTION_WRITE,
                                peripheral,
                                address,
                                phase_mDeg,
                                uid,
                                DDS_PHASE_BASE_ADDR,
                                DDS_PHASE_KEY,
                                true,
                                false);
    } else {
        handleCncShortResponseRequest(p_webResponse,
                                      destinationVal,
                                      CNC_ACTION_WRITE,
                                      peripheral,
                                      address,
                                      phase_mDeg,
                                      uid,
                                      DDS_PHASE_BASE_ADDR,
                                      DDS_PHASE_KEY,
                                      true,
                                      false);
    }
    return p_webResponse;
}

webResponse_tp webDDSWaveformSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_MCG_DDS(slot);
    GET_REQ_KEY_VALUE(int, dac, obj, json_object_get_int);
    if (!testDacValue(p_webResponse, dac)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    uint32_t address = dac + DDS_WAVEFORM_BASE_ADDR;

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, waveform, obj, json_object_get_int);
    if (!testWaveformValue(p_webResponse, waveform)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_WRITE,
                            peripheral,
                            address,
                            waveform,
                            uid,
                            DDS_WAVEFORM_BASE_ADDR,
                            DDS_WAVEFORM_KEY,
                            true,
                            coilDriverBoard(destinationVal));
    return p_webResponse;
}

webResponse_tp webDDSWaveformGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testSensorSlotValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_MCG_DDS(slot);

    GET_REQ_KEY_VALUE(int, dac, obj, json_object_get_int);
    if (!testDacValue(p_webResponse, dac)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    uint32_t address = dac + DDS_WAVEFORM_BASE_ADDR;

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_READ,
                            peripheral,
                            address,
                            0,
                            uid,
                            DDS_WAVEFORM_BASE_ADDR,
                            DDS_WAVEFORM_KEY,
                            true,
                            coilDriverBoard(destinationVal));
    return p_webResponse;
}

webResponse_tp webCoilDDSWaveformSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testCoilDDSValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_COIL_DDS(slot);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);

    GET_REQ_KEY_VALUE(int, waveform, obj, json_object_get_int);
    if (!testWaveformValue(p_webResponse, waveform)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_WRITE,
                            peripheral,
                            DDS_WAVEFORM_BASE_ADDR,
                            waveform,
                            uid,
                            DDS_WAVEFORM_BASE_ADDR,
                            DDS_WAVEFORM_KEY,
                            true,
                            true);
    return p_webResponse;
}

webResponse_tp webCoilDDSWaveformGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testCoilDDSValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_COIL_DDS(slot);

    uint32_t address = DDS_WAVEFORM_BASE_ADDR;

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_READ,
                            peripheral,
                            address,
                            0,
                            uid,
                            DDS_WAVEFORM_BASE_ADDR,
                            DDS_WAVEFORM_KEY,
                            true,
                            true);
    return p_webResponse;
}

webResponse_tp webCoilDDSAmpGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)

    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testCoilDDSValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_COIL_DDS(slot);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    uint32_t address = DDS_AMPLITUDE_BASE_ADDR;

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_READ,
                            peripheral,
                            address,
                            0,
                            uid,
                            DDS_AMPLITUDE_BASE_ADDR,
                            DDS_AMPLITUDE_KEY,
                            true,
                            true);
    return p_webResponse;
}

webResponse_tp webCoilDDSAmpSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testCoilDDSValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_COIL_DDS(slot);

    uint32_t address = DDS_AMPLITUDE_BASE_ADDR;

    GET_REQ_KEY_VALUE(int, amplitude, obj, json_object_get_int);
    if (!testAmplitudeValue(p_webResponse, amplitude)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_WRITE,
                            peripheral,
                            address,
                            amplitude,
                            uid,
                            DDS_AMPLITUDE_BASE_ADDR,
                            DDS_AMPLITUDE_KEY,
                            true,
                            true);
    return p_webResponse;
}

webResponse_tp webCoilDDSFreqGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testCoilDDSValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_COIL_DDS(slot);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_READ,
                            peripheral,
                            DDS_FREQUENCY_BASE_ADDR,
                            0,
                            uid,
                            0,
                            DDS_FREQUENCY_KEY,
                            false,
                            true);
    return p_webResponse;
}

webResponse_tp webCoilDDSFreqSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testCoilDDSValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_COIL_DDS(slot);

    GET_REQ_KEY_VALUE(int, frequency, obj, json_object_get_int);
    if (!testFrequencyValue(p_webResponse, frequency)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_WRITE,
                            peripheral,
                            DDS_FREQUENCY_BASE_ADDR,
                            frequency,
                            uid,
                            0,
                            DDS_FREQUENCY_KEY,
                            false,
                            true);
    return p_webResponse;
}

webResponse_tp webCoilDDSPhaseGet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testCoilDDSValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_COIL_DDS(slot);

    uint32_t address = DDS_PHASE_BASE_ADDR;

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_READ,
                            peripheral,
                            address,
                            0,
                            uid,
                            DDS_PHASE_BASE_ADDR,
                            DDS_PHASE_KEY,
                            false,
                            true);
    return p_webResponse;
}

webResponse_tp webCoilDDSPhaseSet(const char *jsonStr, int strLen) {
    int destinationVal = 0;
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_DESTINATION(destinationVal)
    GET_REQ_KEY_VALUE(int, slot, obj, json_object_get_int);
    if (!testCoilDDSValue(p_webResponse, slot)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    PERIPHERAL_e peripheral = PER_COIL_DDS(slot);

    uint32_t address = DDS_PHASE_BASE_ADDR;

    GET_REQ_KEY_VALUE(int, phase_mDeg, obj, json_object_get_int);
    if (!testPhaseValue(p_webResponse, phase_mDeg)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    WEB_CMD_PARAM_CLEANUP

    handleCncGenericRequest(p_webResponse,
                            destinationVal,
                            CNC_ACTION_WRITE,
                            peripheral,
                            address,
                            phase_mDeg,
                            uid,
                            DDS_PHASE_BASE_ADDR,
                            DDS_PHASE_KEY,
                            false,
                            true);

    return p_webResponse;
}

webResponse_tp webDDSStart(const char *jsonStr, int strLen) {
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    json_object *jsonObjectInt = json_object_new_int(uid);
    json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonObjectInt, JSON_C_OBJECT_KEY_IS_CONSTANT);

    handleStartStop(p_webResponse, CNC_DDS_START);

    p_webResponse->httpCode = HTTP_OK;
    struct json_object *jsonResult = json_object_new_string("success");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    WEB_CMD_PARAM_CLEANUP;

    return p_webResponse;
}

webResponse_tp webDDSStop(const char *jsonStr, int strLen) {
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);
    GET_REQ_KEY_VALUE(int, uid, obj, json_object_get_int);
    json_object *jsonObjectInt = json_object_new_int(uid);
    json_object_object_add_ex(p_webResponse->jsonResponse, "uid", jsonObjectInt, JSON_C_OBJECT_KEY_IS_CONSTANT);
    handleStartStop(p_webResponse, CNC_DDS_STOP);

    p_webResponse->httpCode = HTTP_OK;
    struct json_object *jsonResult = json_object_new_string("success");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
    WEB_CMD_PARAM_CLEANUP;
    return p_webResponse;
}

bool testCoilDDSValue(webResponse_tp p_webResponse, int dds) {
    TEST_VALUE(dds, 0, MAX_COIL_DDS_DEVICE_ID)
}

bool testDacValue(webResponse_tp p_webResponse, int dac) {
    TEST_VALUE(dac, 0, MAX_DDS_OUTPUT)
}

bool testWaveformValue(webResponse_tp p_webResponse, int waveform) {
    TEST_VALUE(waveform, 0, WAVEFORM_MAX)
}

bool testPhaseValue(webResponse_tp p_webResponse, int phase) {
    TEST_VALUE(phase, 0, MAX_DDS_PHASE_MDEG)
}

bool testAmplitudeValue(webResponse_tp p_webResponse, int amplitude) {
    TEST_VALUE(amplitude, 0, 100 * 1000) // percentage of total amplitude in Milli %
}

bool testFrequencyValue(webResponse_tp p_webResponse, int frequency) {
    return true;
}

bool testWaveformParameterValue(webResponse_tp p_webResponse, int waveformParameter) {
    TEST_VALUE(waveformParameter, MIN_WAVEFORM_PARAMETER, MAX_WAVEFORM_PARAMETER)
}

void handleStartStop(webResponse_tp p_webResponse, CNC_ACTION_e commandEnum) {

    if (commandEnum == CNC_DDS_STOP) {
        ddsTriggerDisable();
    } else if (commandEnum == CNC_DDS_START) {
        ddsTriggerEnable();
    } else if (commandEnum == CNC_DDS_SYNC) {
        ddsTriggerSync();
    }

    p_webResponse->httpCode = HTTP_OK;
    json_object *jsonResult = json_object_new_string("success");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
}

WEB_DDS_WAVEFORM_X_SET(ELONGATE, elongate)
WEB_DDS_WAVEFORM_X_GET(ELONGATE)

WEB_DDS_WAVEFORM_X_SET(INTERVAL, interval)
WEB_DDS_WAVEFORM_X_GET(INTERVAL)

WEB_DDS_WAVEFORM_X_SET(DELAY, delay)
WEB_DDS_WAVEFORM_X_GET(DELAY)
