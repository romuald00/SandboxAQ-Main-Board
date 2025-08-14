/*
 * dbCommTask.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 10, 2023
 *      Author: rlegault
 */

#include "dbCommTask.h"
#include "MB_gatherTask.h"
#include "cli/cli_print.h"
#include "cli_task.h"
#include "cmdAndCtrl.h"
#include "cmsis_os.h"
#include "ctrlSpiCommTask.h"
#include "dbTriggerTask.h"
#include "ddsTrigTask.h"
#include "debugPrint.h"
#include "perseioTrace.h"
#include "registerParams.h"
#include "saqTarget.h"
#include "stmTarget.h"
#include "taskWatchdog.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#define EXTRA_SENSORBOARD_RESPONSE_TIME_x2MS                                                                           \
    2 // each count gives additional 2msec for the sensor board to prepare the response in the large buffer tx area

#define SPI_READY_RETEST_DELAY_MS 25

#define INITIATE_RESET 1 // set value to 1 to cause the sensor board to resetV

#define BUF_SZ_BYTES 16
#define CLI_BUF_SZ_BYTES 60

#define MAX_DB_TASKS MAX_CS_ID

#if (MAX_DB_TASKS_TEST + DB_TASK_START_ID) > MAX_DB_TASKS
#error("MAX_DB_TASKS_TEST is larger than MAX_DB_TASKS")
#endif

#define DB_COMM_MSG_DEPTH 2
#define DBCOMM_EVENT_DELAY_MS 1000

#define CMD_IDX 1
#define ID_IDX 2
#define NO_ID_DATA_IDX 2
#define DATA_IDX 3
#define MATCH_IDX(x) (x + 1)

extern osPoolId spiMsgPool;
extern osPoolId cncMsgPool;

__DTCMRAM__ StaticQueue_t dbQMsgCtrl[MAX_DB_TASKS];
__DTCMRAM__ uint8_t dbMsgQ[MAX_DB_TASKS][DB_COMM_MSG_DEPTH * sizeof(cncInfo_tp)];

#define dbCommMessageQCreateCommon(ID)                                                                                 \
    dbCommThreads[ID].msgQId = osMessageCreate(osMessageQ(cbComm##ID##MsgQ), NULL);                                    \
    assert(dbCommThreads[ID].msgQId != NULL);                                                                          \
    dbCommThreads[ID].daughterBoardId = ID;                                                                            \
    dbCommThreads[ID].watchDogId = WDT_TASK_DBCOMM_##ID;                                                               \
    dbCommThreads[ID].dbCommState.enabled = false;                                                                     \
    dbCommThreads[ID].dbCommState.rxDataCnt = 0;                                                                       \
    dbCommThreads[ID].dbCommState.rxCmdsCnt = 0;

#define dbCommMessageQCreateStatic(ID)                                                                                 \
    if ((ID >= DB_TASK_START_ID) && (ID < MAX_DB_TASKS_END_ID)) {                                                      \
        osMessageQStaticDef(cbComm##ID##MsgQ, DB_COMM_MSG_DEPTH, cncInfo_tp, dbMsgQ[ID], &dbMsgCtrl[ID]);              \
        dbCommMessageQCreateCommon(ID)                                                                                 \
    }

#define dbCommMessageQCreate(ID)                                                                                       \
    if ((ID >= DB_TASK_START_ID) && (ID < MAX_DB_TASKS_END_ID)) {                                                      \
        osMessageQDef(cbComm##ID##MsgQ, DB_COMM_MSG_DEPTH, cncInfo_tp);                                                \
        dbCommMessageQCreateCommon(ID)                                                                                 \
    }

static void (*registerDbStateCbFnPtr)(int daughterBoardId, bool enabled) = NULL;

static cncPayload_t sensorData = {
    .cmd.cncMsgPayloadHeader.peripheral = PER_NOP,
    .cmd.cncMsgPayloadHeader.action = 0,
    .cmd.cncMsgPayloadHeader.addr = 0,
    .cmd.value = 0,
    .cmd.cncMsgPayloadHeader.cncActionData.result = 0,
};

static uint32_t noCncPoolMemoryCnt;
bool dbCommTasksInitComplete_ = false;

__DTCMRAM__ dbCommThreadInfo_t dbCommThreads[MAX_DB_TASKS] = {0};
__DTCMRAM__ uint32_t registerGPIOMask[MAX_DB_EVENT_GROUPS] = {0};

__DTCMRAM__ StaticTask_t dbTaskCtrlBlock[MAX_DB_TASKS];
__DTCMRAM__ StackType_t dbStack[MAX_DB_TASKS][DB_COMM_STACK_WORDS];

static void dbCommTaskThread(void const *argument);
static void handleDbMsg(dbCommThreadInfo_tp p_dbThread, cncInfo_tp p_data);
static osStatus dbProcRxSendMsg(spiDbMbCmd_e spiDbMbCmd,
                                uint32_t cbId,
                                uint8_t xInfo,
                                spiDbMbPacketCmdResponse_t cmdResponse,
                                cncPayload_tp payload);
static osStatus dbCommCliCallback(spiDbMbCmd_e spiDbMbCmd,
                                  uint32_t cbId,
                                  uint8_t xInfo,
                                  spiDbMbPacketCmdResponse_t cmdResponse,
                                  cncPayload_tp p_cncMsg);

static cncInfo_t doNotFreeCncInfo = {
    .destination = MAIN_BOARD_ID, // unused on the daughter board cnc.
    .cmd = SPICMD_TRIGGER,        // spiDbMbCmd_e, cannot specify size for enum so cast it when needed
    .xInfo = 0,
    .payload.u8Array = {0},
    .cbFnPtr = NULL,
    .cbId = 0};

static cncPayload_t timeoutPayload = {.cmd.cncMsgPayloadHeader.cncActionData.result = osEventTimeout};

static const uint32_t sineArray[] = {
    50009, 50019, 50029, 50038, 50047, 50056, 50064, 50071, 50078, 50084, 50089, 50093, 50096, 50098, 50099, 50099,
    50099, 50097, 50094, 50090, 50086, 50080, 50074, 50067, 50059, 50051, 50042, 50033, 50023, 50014, 50004, 49994,
    49984, 49974, 49964, 49955, 49947, 49938, 49931, 49924, 49918, 49912, 49908, 49904, 49902, 49900, 49900, 49900,
    49901, 49904, 49907, 49911, 49916, 49922, 49929, 49936, 49944, 49953, 49962, 49972, 49981, 49991, 50001};

void dbCommTaskEnableAll(void) {
    __disable_irq();
    for (int i = DB_TASK_START_ID; i < MAX_DB_TASKS_END_ID; i++) {
        if (dbCommThreads[i].dbCommState.enabled != true) {
            if (registerDbStateCbFnPtr != NULL) {
                registerDbStateCbFnPtr(i, true);
            }
            dbCommThreads[i].dbCommState.enabled = true;
        }
        dbCommThreads[i].dbCommState.disableCnt = 0;
    }
    dbTriggerEnableAll();
    __enable_irq();
}

void dbCommTaskEnable(uint32_t dbId, bool enable) {
    enable ? dbTriggerEnable(dbId) : dbTriggerDisable(dbId);
    dbCommThreads[dbId].dbCommState.disableCnt = 0;
    if (registerDbStateCbFnPtr != NULL) {
        registerDbStateCbFnPtr(dbId, enable);
    }
    dbCommThreads[dbId].dbCommState.enabled = enable;
}

bool dbCommTasksInitComplete(void) {
    return dbCommTasksInitComplete_;
}

// We create them in blocks to reduce stack usage
void createBlock0(void) {
    dbCommMessageQCreate(0);
    dbCommMessageQCreate(1);
    dbCommMessageQCreate(2);
    dbCommMessageQCreate(3);
    dbCommMessageQCreate(4);
    dbCommMessageQCreate(5);
    dbCommMessageQCreate(6);
    dbCommMessageQCreate(7);
}

void createBlock1(void) {
    dbCommMessageQCreate(8);
    dbCommMessageQCreate(9);
    dbCommMessageQCreate(10);
    dbCommMessageQCreate(11);
    dbCommMessageQCreate(12);
    dbCommMessageQCreate(13);
    dbCommMessageQCreate(14);
    dbCommMessageQCreate(15);
}
void createBlock2(void) {
    dbCommMessageQCreate(16);
    dbCommMessageQCreate(17);
    dbCommMessageQCreate(18);
    dbCommMessageQCreate(19);
    dbCommMessageQCreate(20);
    dbCommMessageQCreate(21);
    dbCommMessageQCreate(22);
    dbCommMessageQCreate(23);
}

bool sensorBoardPresent(int dbId) {
    if (dbCommThreads[dbId].dbCommState.enabled &&
        // Disable count can be 0 or if we are waiting for a response 1, anything higher and we may be going into a
        // disabled state
        (dbCommThreads[dbId].dbCommState.disableCnt <= 1) &&
        (dbCommThreads[dbId].dbCommState.rxDataCnt || dbCommThreads[dbId].dbCommState.rxCmdsCnt)) {
        return true;
    }
    return false;
}

void dbCommTaskInit(int priority, int stackSize) {
    DPRINTF_CCOMM("Ctrl Comm task initializing all spi tasks\r\n");
    assert(stackSize == DB_COMM_STACK_WORDS);

    memset(dbCommThreads, 0, sizeof(dbCommThreads));
    memset(registerGPIOMask, 0, sizeof(registerGPIOMask));
    registerDbStateCallback(setDaughterboardState);
    // create the MSG Q and assign them to the DB threads.
    createBlock0();
    createBlock1();
    createBlock2();
    char buf[BUF_SZ_BYTES];

    for (int i = DB_TASK_START_ID; i < MAX_DB_TASKS_END_ID; i++) {
        snprintf(buf, BUF_SZ_BYTES, "dbComm%d", i);
        const osThreadDef_t os_thread_def_dbCommThreadInst = {
            buf, (dbCommTaskThread), (priority), (0), (stackSize), dbStack[i], &dbTaskCtrlBlock[i]};
        dbCommThreads[i].dbCommState.sensorUID = 0xFF;
        dbCommThreads[i].dbCommState.disableCnt = 0;
        dbCommThreads[i].threadId = osThreadCreate(osThread(dbCommThreadInst), &dbCommThreads[i]);
        dbCommThreads[i].dbGroupEvtIdx = GET_GROUP_EVT_IDX(i);
        dbCommThreads[i].dbGroupEvtId = GET_GROUP_EVT_ID(i);
        dbCommThreads[i].cmdUid = CMD_UID_DONT_CARE + 1;
        dbCommThreads[i].dbCommState.largeBuffer = NULL;
        dbCommThreads[i].dbCommState.largeBufferSz = 0;

        assert(dbCommThreads[i].threadId != NULL);
        DPRINTF_INFO("DB %d TCB=0x%p\r\n", i, dbCommThreads[i].threadId);
        osDelay(INIT_DELAYS);
    }

    dbCommTaskEnableAll();
    dbCommTasksInitComplete_ = true;
}

__ITCMRAM__ void dbCommTaskThread(void const *argument) {
    dbCommThreadInfo_tp p_dbCommInfo = (dbCommThreadInfo_tp)argument;
    DPRINTF_DBCOMM("Ctrl Comm task for DB%d starting\r\n", p_dbCommInfo->daughterBoardId);

    watchdogAssignToCurrentTask(p_dbCommInfo->watchDogId);
    watchdogSetTaskEnabled(p_dbCommInfo->watchDogId, 1);
    while (!SPIIsReady(p_dbCommInfo->daughterBoardId)) {
        osDelay(SPI_READY_RETEST_DELAY_MS);
    }
    bool prevEnable = false;
    while (1) {
        watchdogKickFromTask(p_dbCommInfo->watchDogId);
        if (!prevEnable) {
            if (p_dbCommInfo->dbCommState.enabled) {
                p_dbCommInfo->dbCommState.procEnableAtTick = HAL_GetTick();
                p_dbCommInfo->dbCommState.enabledCnt++;
                prevEnable = true;
            }
        }
        // EventGroup asserts if all bits are 0 even with a timeout.
        if (dbTriggerEventGroup[p_dbCommInfo->dbGroupEvtIdx] == 0) {
            osDelay(DBCOMM_EVENT_DELAY_MS);
        } else {
            EventBits_t evBits = xEventGroupWaitBits(dbTriggerEventGroup[p_dbCommInfo->dbGroupEvtIdx],
                                                     p_dbCommInfo->dbGroupEvtId,
                                                     true,
                                                     true,
                                                     DBCOMM_EVENT_DELAY_MS);
            if (evBits & p_dbCommInfo->dbGroupEvtId) {
                handleDbMsg(p_dbCommInfo, &doNotFreeCncInfo);
            }
            // regardless lets handle any messages received
            osEvent evt = osMessageGet(p_dbCommInfo->msgQId, 0);
            while (evt.status == osEventMessage) {
                cncInfo_tp p_data = (cncInfo_tp)evt.value.p;
                handleDbMsg(p_dbCommInfo, p_data);
                evt = osMessageGet(p_dbCommInfo->msgQId, 0);
            }
        }
    }
}

void registerDbStateCallback(void (*cbFnPtr)(int daughterBoardId, bool enabled)) {
    registerDbStateCbFnPtr = cbFnPtr;
}

__ITCMRAM__ void handleDbMsg(dbCommThreadInfo_tp p_dbThread, cncInfo_tp p_data) {
    spiDbMbPacketCmdResponse_t cmdResponse = {.cmdUid = CMD_UID_DONT_CARE, .cmdResponse = UNKNOWN_COMMAND};
    if (p_dbThread->dbCommState.waitingForCNCResponse && p_data->cmd == SPICMD_TRIGGER) {
        if (p_dbThread->dbCommState.responseDelayCnt++ > DB_MAX_UNANSWERED_RESPONSE) {
            /* Although the CRC error rate is 0.0005/sec it is still enough that we are seeing
             * missed commands causing issues.
             * We will resend the missed Command once
             */
            if (p_dbThread->resendCnt == 0) {
                DPRINTF_INFO("DB %d lost CNC message resending\r\n", p_dbThread->daughterBoardId);
                p_dbThread->dbCommState.resendCNCCnt++;
                p_dbThread->resendCnt++;
                p_dbThread->dbCommState.waitingForCNCResponse = false; // going to resend it.
                p_data = &p_dbThread->copyLastCmd;
            } else {
                DPRINTF_ERROR("DB %d lost CNC message\r\n", p_dbThread->daughterBoardId);
                if (p_dbThread->dbCommState.cbFnPtr) {
                    p_dbThread->dbCommState.cbFnPtr(SPICMD_RESP_CNC,
                                                    p_dbThread->dbCommState.cbId,
                                                    p_dbThread->dbCommState.xInfoMatch,
                                                    cmdResponse,
                                                    &timeoutPayload);
                    p_dbThread->dbCommState.cbFnPtr = NULL;
                }

                p_dbThread->dbCommState.responseDelayCnt = 0;
                p_dbThread->dbCommState.waitingForCNCResponse = false;
                p_dbThread->cmdUid++;
            }
        }
    }

    switch (p_data->cmd) {
    case (SPICMD_TRIGGER): {
        if (p_dbThread->dbCommState.loopbackSensor) {
            p_dbThread->dbCommState.disableCnt = 0;
            static int sinLen = sizeof(sineArray) / sizeof(uint32_t);
            static uint8_t xInfo = 0;
            static rx_dbNopPayload_t payload = {0};
            static int32_t count = 0;

            xInfo++;
            count++;
            if (count > 0x7FFFFFFF / 2)
                count = 0;
            int32_t dbCount = count + p_dbThread->dbCommState.loopbackOffset;

            // These numbers were selected to give a known and differing pattern
            // to each sensor to test that the values received for each sensor
            // would be in the correct location.
            const uint32_t stepFn_high_0 = 1100000;
            const uint32_t stepFn_low_0 = 150000;
            const uint32_t stepFn_high_4 = 125000;
            const uint32_t stepFn_low_4 = 75000;
            const uint32_t freq_divider = 4;
            const uint32_t sine_amplitude_multiplier_6 = 20;
            const uint32_t sawtooth_duration = 20000;
            const uint32_t sawtooth_amplitude_multipler_3 = 100;
            const uint32_t sawtooth_amplitude_multipler_7 = 2;
            const uint32_t sawtooth_y_offset_3 = 50000;
            const uint32_t sawtooth_y_offset_7 = 65000;
#define STEP_FN_DURATION(i) ((i % 10) < 5)
            int32_t wtemp,rtemp;
            for (int i = 0; i < NUMBER_OF_SENSOR_READINGS; i++) {
                switch (i) {
                case (0):
                    rtemp = READ_XBITSVALUE(payload.adc[i].VALUE_X_BIT);
                    wtemp = (rtemp == stepFn_low_0) ? stepFn_high_0 : stepFn_low_0;
                    break;
                case (1):
                    wtemp = sineArray[(dbCount / freq_divider) % sinLen];
                    break;
                case (2):
                    wtemp= sineArray[dbCount % sinLen];
                    break;
                case (3):
                    wtemp =
                        (dbCount % sawtooth_duration) * sawtooth_amplitude_multipler_3 + sawtooth_y_offset_3;
                    break;
                case (4):
                    rtemp = READ_XBITSVALUE(payload.adc[STEP_FN_DURATION(i)].VALUE_X_BIT);
                    wtemp = (rtemp == stepFn_high_4) ? stepFn_low_4 : stepFn_high_4;
                    break;
                case (5):
                    wtemp = dbCount;
                    break;
                case (6):
                    wtemp = sineArray[(dbCount % sinLen)] * sine_amplitude_multiplier_6;
                    break;
                case (7):
                    wtemp =
                        (dbCount % sawtooth_duration) * sawtooth_amplitude_multipler_7 + sawtooth_y_offset_7;
                    break;
                default:
                    wtemp = 0;
                break;
                }

                WRITE_XBITVALUE(payload.adc[i].VALUE_X_BIT, wtemp);
            }
            dbProcRxSendMsg(SPICMD_STREAM_SENSOR, (uint32_t)p_dbThread, xInfo, cmdResponse, (cncPayload_tp)&payload);
        } else {

            // If we send 10 messages with no responses then set the daughter board as disabled
            if (p_dbThread->dbCommState.enabled &&
                p_dbThread->dbCommState.disableCnt++ > DB_MAX_UNANSWERED_RESPONSE_DISABLE) {
                DPRINTF_DBCOMM_VERBOSE("dbProc %d, disableCnt = %d sending Reboot cmd\r\n",
                                       p_dbThread->daughterBoardId,
                                       p_dbThread->dbCommState.disableCnt);
                // send a reset command incase we are receiving data and it is misaligned.
                cncMsgPayload_t payload = {
                    .cncMsgPayloadHeader.peripheral = PER_MCU, .cncMsgPayloadHeader.action = CNC_ACTION_WRITE, .cncMsgPayloadHeader.addr = REBOOT_FLAG, .value = INITIATE_RESET};
                spiSendMsg(p_dbThread->daughterBoardId, SPICMD_CNC, 0, CMD_UID_DONT_CARE, &payload, NULL, 0, 0);

                if (registerDbStateCbFnPtr != NULL) {
                    registerDbStateCbFnPtr(p_dbThread->daughterBoardId, false);
                }
                dbTriggerDisable(p_dbThread->daughterBoardId);
                p_dbThread->dbCommState.enabled = false;
            }

            if (p_dbThread->dbCommState.enabled) {
                // use static msg payload stored globally.
                spiSendMsg(p_dbThread->daughterBoardId,
                           SPICMD_NOP,
                           p_dbThread->dbCommState.sensorUID,
                           CMD_UID_DONT_CARE,
                           (cncMsgPayload_tp)&sensorData,
                           dbProcRxSendMsg,
                           (uint32_t)p_dbThread,
                           0);
            }
        }
    } break;
    case (SPICMD_STREAM_SENSOR): {
        // since this is fast path code, we will just handle it in the SPI thread.
        // no need to create a msg and send it to this task.
        assert(0 == 1);
    } break;
    case (SPICMD_CNC):
    // fall through
    case (SPICMD_CNC_SHORT): {
        xTracePrintCompactF2(urlLogTxMsg, "handleDbMsg dest=%d per=%d", p_dbThread->daughterBoardId, p_data->payload.cmd.cncMsgPayloadHeader.peripheral);
        if (p_dbThread->dbCommState.waitingForCNCResponse) {
            osThreadYield();
            // Now put the message back in the queue
            osStatus status;
            if ((status = osMessagePut(p_dbThread->msgQId, (uint32_t)p_data, 0)) != osOK) {
                // Q full drop message
                DPRINTF_ERROR("DB%d too many messages for one process\r\n", p_dbThread->daughterBoardId);
                osPoolFree(cncMsgPool, p_data);
            }
            return;
        }
        // If this is a new Command make a copy incase we need to resend.
        if (p_data != &p_dbThread->copyLastCmd) {
            memcpy(&p_dbThread->copyLastCmd, p_data, sizeof(cncInfo_t));
            p_dbThread->resendCnt = 0;
        }

        p_dbThread->dbCommState.cbFnPtr = p_data->cbFnPtr;
        p_dbThread->dbCommState.cbId = p_data->cbId;
        p_dbThread->dbCommState.responseDelayCnt = 0;
        p_dbThread->dbCommState.waitingForCNCResponse = true;
        p_dbThread->dbCommState.xInfoMatch++;

        p_data->payload.cmd.cncMsgPayloadHeader.shortCncId = p_dbThread->cmdUid;

        spiSendMsg(p_dbThread->daughterBoardId,
                   p_data->cmd,
                   p_dbThread->dbCommState.xInfoMatch,
                   p_dbThread->cmdUid,
                   (cncMsgPayload_tp)&p_data->payload,
                   dbProcRxSendMsg,
                   (uint32_t)p_dbThread,
                   0);

        // Only free a new command. A stored copy does not need to be freed
        if (p_data != &p_dbThread->copyLastCmd) {
            osPoolFree(cncMsgPool, p_data);
        }
    } break;
    case (SPICMD_RESP_CNC): {
        dbCommState_tp p_dbCommState = &p_dbThread->dbCommState;
        DPRINTF_DBCOMM("%d RX Command message xinfo=%d value=%ld result=%ld\r\n",
                       p_dbThread->daughterBoardId,
                       p_data->xInfo,
                       p_data->payload.cmd.value,
                       p_data->payload.cmd.result);
        if (p_dbCommState->waitingForCNCResponse) {
            if (p_dbCommState->xInfoMatch == p_data->xInfo) {
                if (p_dbCommState->cbFnPtr) {
                    p_dbCommState->cbFnPtr(
                        p_data->cmd, p_dbCommState->cbId, p_data->xInfo, cmdResponse, &p_data->payload);
                    p_dbCommState->cbFnPtr = NULL;
                    p_dbCommState->waitingForCNCResponse = false;
                } else {
                    DPRINTF_ERROR("%s:%d DB%d cbFn is null\r\n", __FUNCTION__, __LINE__, p_dbThread->daughterBoardId);
                }
            } else {
                DPRINTF_ERROR("unmatched xinfo expected %x msg %x\r\n", p_dbCommState->xInfoMatch, p_data->xInfo);
                if (p_dbCommState->cbFnPtr) {
                    p_dbCommState->cbFnPtr(
                        p_data->cmd, p_dbCommState->cbId, p_data->xInfo, cmdResponse, &p_data->payload);
                    p_dbCommState->cbFnPtr = NULL;
                    p_dbCommState->waitingForCNCResponse = false;
                } else {
                    DPRINTF_ERROR("%s:%d DB%d cbFn is null\r\n", __FUNCTION__, __LINE__, p_dbThread->daughterBoardId);
                }
            }
        } else {
            DPRINTF_ERROR("Unexpected command response\r\n")
        }
        osPoolFree(cncMsgPool, p_data);
    } break;

    // fall through
    case (SPICMD_STREAM_SENSOR_W_SHORTRESPONSE): {
        dbCommState_tp p_dbCommState = &p_dbThread->dbCommState;
        DPRINTF_CMD_STREAM(
            "** %s:%d  SHORT RESPONSE  cmdUid = %d\r\n", __FUNCTION__, __LINE__, p_data->cmdResponse.cmdUid);
        DPRINTF_DBCOMM("%d RX Command message xinfo=%d value=%ld result=%ld\r\n",
                       p_dbThread->daughterBoardId,
                       p_data->xInfo,
                       p_data->payload.cmd.value,
                       p_data->payload.cmd.result);
        if (p_dbCommState->waitingForCNCResponse) {

            if (p_dbCommState->cbFnPtr) {
                if (p_data->cmdResponse.cmdUid == p_dbThread->cmdUid) {
                    p_dbThread->cmdUid++;
                    p_dbThread->cmdUid =
                        (p_dbThread->cmdUid != CMD_UID_DONT_CARE) ? p_dbThread->cmdUid : CMD_UID_DONT_CARE + 1;
                    p_dbCommState->cbFnPtr(
                        p_data->cmd, p_dbCommState->cbId, p_data->xInfo, p_data->cmdResponse, &p_data->payload);
                    p_dbCommState->cbFnPtr = NULL;
                    p_dbCommState->waitingForCNCResponse = false;

                } else {
                    DPRINTF_ERROR("%s:%d DB%d cmdUid %d != thread->cmdUid= %d\r\n",
                                  __FUNCTION__,
                                  __LINE__,
                                  p_dbThread->daughterBoardId,
                                  p_data->cmdResponse.cmdUid,
                                  p_data->cmdResponse.cmdUid);
                }
            } else {
                DPRINTF_ERROR("%s:%d DB%d cbFn is null\r\n", __FUNCTION__, __LINE__, p_dbThread->daughterBoardId);
            }

        } else {
            DPRINTF_ERROR("Unexpected command response\r\n")
        }
        osPoolFree(cncMsgPool, p_data);
    } break;
    case (SPICMD_SET_SKIP_WAIT_FOR_RESPONSE):
        bool value = p_data->payload.cmd.value;
        if (p_data->payload.cmd.cncMsgPayloadHeader.action == CNC_ACTION_WRITE) {
            p_dbThread->dbCommState.loopbackSensor = value;
            p_data->payload.cmd.cncMsgPayloadHeader.cncActionData.result = 0;
            p_data->cbFnPtr(SPICMD_RESP_CNC, p_data->cbId, p_data->xInfo, cmdResponse, &p_data->payload);
        } else {
            p_data->payload.cmd.value = p_dbThread->dbCommState.loopbackSensor;
            p_data->payload.cmd.cncMsgPayloadHeader.cncActionData.result = 0;
            p_data->cbFnPtr(SPICMD_RESP_CNC, p_data->cbId, p_data->xInfo, cmdResponse, &p_data->payload);
        }

        break;
    case (SPICMD_SET_LOOPBACK_OFFSET):

        if (p_data->payload.cmd.cncMsgPayloadHeader.action == CNC_ACTION_WRITE) {
            p_dbThread->dbCommState.loopbackOffset = p_data->payload.cmd.value;
            p_data->payload.cmd.cncMsgPayloadHeader.cncActionData.result = 0;

            p_data->cbFnPtr(SPICMD_RESP_CNC, p_data->cbId, p_data->xInfo, cmdResponse, &p_data->payload);
        } else {
            p_data->payload.cmd.value = p_dbThread->dbCommState.loopbackOffset;
            p_data->payload.cmd.cncMsgPayloadHeader.cncActionData.result = 0;
            p_data->cbFnPtr(SPICMD_RESP_CNC, p_data->cbId, p_data->xInfo, cmdResponse, &p_data->payload);
        }
        break;
    default:
        break;
    }
}

__ITCMRAM__ static osStatus dbProcRxSendMsg(spiDbMbCmd_e spiDbMbCmd,
                                            uint32_t cbId,
                                            uint8_t xInfo,
                                            spiDbMbPacketCmdResponse_t cmdResponse,
                                            cncPayload_tp payload) {

    dbCommThreadInfo_tp p_dbThread = (dbCommThreadInfo_tp)cbId;
    assert(p_dbThread != NULL);
    if (p_dbThread->dbCommState.enabled == false) {
        p_dbThread->dbCommState.enabled = true;
        if (registerDbStateCbFnPtr != NULL) {
            registerDbStateCbFnPtr(p_dbThread->daughterBoardId, p_dbThread->dbCommState.enabled);
        }
        dbTriggerEnable(p_dbThread->daughterBoardId);
    }
    p_dbThread->dbCommState.disableCnt = 0;

    assert(payload != NULL);

    // We have added a path to allow certain write commands to have results reported in a flag field in header
    // So when the cmd field has the CMD_SHORTREPONSE flag set it contains both a sensor data update
    // and a Command response result.
    // So first we ignore the flag to see if the data fields need updating.
    // Then if it is set then we have to send the response onto the daughter card thread to
    // report the result.
    if (spiDbMbCmd != SPICMD_STREAM_SENSOR && spiDbMbCmd != SPICMD_DDS_TRIG) {
        DPRINTF("%s dbThread %d cmd=0x%x, xInfo=%lu cmdUid=%d\r\n",
                __FUNCTION__,
                p_dbThread->daughterBoardId,
                spiDbMbCmd,
                xInfo,
                cmdResponse.cmdUid);
    }
    if ((spiDbMbCmd & (~CMD_SHORTRSEPONSE)) != (SPICMD_STREAM_SENSOR)) {
        DPRINTF_DBCOMM_VERBOSE(
            "%s dbThread %d cmd=0x%x, xInfo=%lu\r\n", __FUNCTION__, p_dbThread->daughterBoardId, spiDbMbCmd, xInfo);
    } else {
        p_dbThread->dbCommState.rxDataCnt++;

        uint8_t v = xInfo - p_dbThread->dbCommState.sensorUID;
        if (v == MULTIPLER) {
            p_dbThread->dbCommState.delayBins[BIN_TARGET]++;
        } else if (v == MULTIPLER + 1) {
            p_dbThread->dbCommState.delayBins[BIN_GREATER1]++;
        } else if (v == MULTIPLER - 1) {
            p_dbThread->dbCommState.delayBins[BIN_LESS1]++;
        } else if (v > MULTIPLER + 1) {
            p_dbThread->dbCommState.delayBins[BIN_GREATER]++;
        }
#if MULTIPLER > 1
        else if (v < MULTIPLER - 1) {
            p_dbThread->dbCommState.delayBins[BIN_LESS]++;
        }
#endif

        //  want to handle this as efficiently as possible so update sensor data and return
        if (xInfo != p_dbThread->dbCommState.sensorUID) {
            updateSensorData(p_dbThread, payload->u8Array, sizeof(payload));
        }
        p_dbThread->dbCommState.sensorUID = xInfo;
        if (!(spiDbMbCmd & CMD_SHORTRSEPONSE)) {
            return osOK;
        }
    }

    if (spiDbMbCmd == SPICMD_DDS_TRIG) {
        ddsTriggerSync();
        return osOK;
    } else if (spiDbMbCmd != SPICMD_STREAM_SENSOR && spiDbMbCmd != SPICMD_RESP_CNC &&
               spiDbMbCmd != SPICMD_STREAM_SENSOR_W_SHORTRESPONSE) {
        // DPRINTF_ERROR("MB received unhandled message 0x%x from DB\r\n", spiDbMbCmd);
        return osErrorValue;
    }

    cncInfo_tp msg = NULL;
    do {
        msg = osPoolAlloc(cncMsgPool);
        if (msg == NULL) {
            noCncPoolMemoryCnt++;
            osDelay(1);
        }
    } while (msg == NULL);

    // memcpy (Destination, src, size)
    memcpy(&msg->payload, payload, sizeof(cncPayload_t));

    msg->cmdResponse = cmdResponse;
    msg->cmd = spiDbMbCmd;
    msg->cbFnPtr = NULL;
    msg->cbId = 0;
    msg->xInfo = xInfo;
    return (osMessagePut(p_dbThread->msgQId, (uint32_t)msg, 0));
}

__ITCMRAM__ osStatus
dbProcSendMsg(int dbId, spiDbMbCmd_e spiDbMbCmd, cncCbFnPtr cbFnPtr, uint32_t cbId, cncPayload_tp payload) {
    assert(payload != NULL);

    if (dbId >= MAX_DB_TASKS) {
        return osErrorParameter;
    }
    dbCommThreadInfo_tp p_dbThread = (dbCommThreadInfo_tp)&dbCommThreads[dbId];
    assert(p_dbThread != NULL);

    DPRINTF_DBCOMM_VERBOSE("%s dbThread %d cmd=0x%x\r\n", __FUNCTION__, p_dbThread->daughterBoardId, spiDbMbCmd);

    assert(spiDbMbCmd != SPICMD_STREAM_SENSOR && spiDbMbCmd != SPICMD_RESP_CNC);

    if (p_dbThread->msgQId == NULL) {
        return osErrorResource;
    }

    cncInfo_tp msg = NULL;
    do {
        if (noCncPoolMemoryCnt > 5) {
            return osErrorNoMemory;
        }

        msg = osPoolAlloc(cncMsgPool);
        if (msg == NULL) {
            DPRINTF_ERROR("%s dbThread %d No memory error\r\n", __FUNCTION__, p_dbThread->daughterBoardId);
            noCncPoolMemoryCnt++;
            osDelay(1);
        }
    } while (msg == NULL);

    memcpy(&msg->payload, payload, sizeof(cncPayload_t));
    msg->cmd = spiDbMbCmd;
    msg->cbFnPtr = cbFnPtr;
    msg->cbId = cbId;
    msg->xInfo = 0;
    return (osMessagePut(p_dbThread->msgQId, (uint32_t)msg, 0));
}

void appendDbprocStatsToBuffer(char *buf, uint32_t bufSz, uint32_t dbId) {

    assert(dbId < MAX_CS_ID);
    char *nxt = buf;
    nxt += snprintf((char *)nxt, bufSz - (nxt - buf), "System:\r\n");
    nxt += snprintf((char *)nxt, bufSz - (nxt - buf), "\tMemory Exhaustion = %lu\r\n", noCncPoolMemoryCnt);

    nxt += snprintf((char *)nxt, bufSz - (nxt - buf), "DB Task %d:\r\n", dbCommThreads[dbId].daughterBoardId);
    nxt += snprintf(
        (char *)nxt, bufSz - (nxt - buf), "\tenabled               = %u\r\n", dbCommThreads[dbId].dbCommState.enabled);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tnew sensor data       = %u\r\n",
                    dbCommThreads[dbId].dbCommState.newSensorData);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tlast sensor UID       = %u\r\n",
                    dbCommThreads[dbId].dbCommState.sensorUID);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\trxCmdsCnt             = %lu\r\n",
                    dbCommThreads[dbId].dbCommState.rxCmdsCnt);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\twaitingForCNCResponse = %u\r\n",
                    dbCommThreads[dbId].dbCommState.waitingForCNCResponse);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tdisableCnt            = %lu\r\n",
                    dbCommThreads[dbId].dbCommState.disableCnt);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tEnable Count          = %lu\r\n",
                    dbCommThreads[dbId].dbCommState.enabledCnt);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tCRC Error Count       = %lu\r\n",
                    dbCommThreads[dbId].dbCommState.crcErrors);
    // 1.0 to force to float before continuing the calculation
    double errorRate = (1.0 * dbCommThreads[dbId].dbCommState.crcErrors) /
                       ((HAL_GetTick() - dbCommThreads[dbId].dbCommState.procEnableAtTick) / HAL_GetTickFreq());
    nxt += snprintf((char *)nxt, bufSz - (nxt - buf), "\tCRC Error Rate        = %.04f\r\n", errorRate);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tRetry Cmd Count       = %lu\r\n",
                    dbCommThreads[dbId].dbCommState.resendCNCCnt);

    nxt += printStreamDataToBuffer(nxt, bufSz - (nxt - buf), dbId);

    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tCount == %u            = %lu\r\n",
                    MULTIPLER,
                    dbCommThreads[dbId].dbCommState.delayBins[BIN_TARGET]);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tCount == %u            = %lu\r\n",
                    MULTIPLER + 1,
                    dbCommThreads[dbId].dbCommState.delayBins[BIN_GREATER1]);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tCount == %u            = %lu\r\n",
                    MULTIPLER - 1,
                    dbCommThreads[dbId].dbCommState.delayBins[BIN_LESS1]);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tCount > %u             = %lu\r\n",
                    MULTIPLER + 1,
                    dbCommThreads[dbId].dbCommState.delayBins[BIN_GREATER]);
}

int16_t dbProcCliCmd(CLI *hCli, int argc, char *argv[]) {
    char buf[CLI_BUF_SZ_BYTES];
    uint16_t success = 0;
    uint32_t min = 0;
    uint32_t max = 1;
    if (argc > 1) {
        if ((argc == MATCH_IDX(ID_IDX)) && (strcmp(argv[CMD_IDX], "stats") == 0)) {
            if (strcmp(argv[ID_IDX], "all") == 0) {
                min = 0;
                max = MAX_DB_TASKS;
            } else {
                uint32_t idx = atoi(argv[ID_IDX]);
                if (idx >= MAX_DB_TASKS) {
                    CliPrintf(hCli, "value %d is out of range 0-%d or all\r\n", idx, MAX_DB_TASKS - 1);
                    return success;
                }
                min = idx;
                max = idx + 1;
            }
            success = 1;
            CliPrintf(hCli, "System:\r\n");
            CliPrintf(hCli, "\tMemory Exhaustion = %lu\r\n", noCncPoolMemoryCnt);
            for (int i = min; i < max; i++) {
                CliPrintf(hCli, "DB Task %d:\r\n", dbCommThreads[i].daughterBoardId);
                CliPrintf(hCli, "\tenabled               = %lu\r\n", dbCommThreads[i].dbCommState.enabled);
                CliPrintf(hCli, "\tnew sensor data       = %lu\r\n", dbCommThreads[i].dbCommState.newSensorData);
                CliPrintf(hCli, "\tlast sensor UID       = %u\r\n", dbCommThreads[i].dbCommState.sensorUID);
                CliPrintf(hCli, "\trxCmdsCnt             = %lu\r\n", dbCommThreads[i].dbCommState.rxCmdsCnt);
                CliPrintf(hCli, "\twaitingForCNCResponse = %u\r\n", dbCommThreads[i].dbCommState.waitingForCNCResponse);
                CliPrintf(hCli, "\tdisableCnt            = %u\r\n", dbCommThreads[i].dbCommState.disableCnt);
                CliPrintf(hCli, "\tEnable Count          = %lu\r\n", dbCommThreads[i].dbCommState.enabledCnt);
                CliPrintf(hCli, "\tCRC Error Count       = %lu\r\n", dbCommThreads[i].dbCommState.crcErrors);
                // 1.0 to force to float before continuing the calculation
                double errorRate =
                    (1.0 * dbCommThreads[i].dbCommState.crcErrors) /
                    ((HAL_GetTick() - dbCommThreads[i].dbCommState.procEnableAtTick) / HAL_GetTickFreq());
                snprintf(buf, sizeof(buf), "\tCRC Error Rate        = %f /s\r\n", errorRate);
                CliPrintf(hCli, "\tRetry Cmd Count       = %lu\r\n", dbCommThreads[i].dbCommState.resendCNCCnt);
                CliPrintf(hCli, "%s", buf);

                printStreamData(hCli, i);

                CliPrintf(hCli,
                          "\tCount == %lu            = %lu\r\n",
                          MULTIPLER,
                          dbCommThreads[i].dbCommState.delayBins[BIN_TARGET]);
                CliPrintf(hCli,
                          "\tCount == %lu            = %lu\r\n",
                          MULTIPLER + 1,
                          dbCommThreads[i].dbCommState.delayBins[BIN_GREATER1]);
                CliPrintf(hCli,
                          "\tCount == %lu            = %lu\r\n",
                          MULTIPLER - 1,
                          dbCommThreads[i].dbCommState.delayBins[BIN_LESS1]);
                CliPrintf(hCli,
                          "\tCount > %lu             = %lu\r\n",
                          MULTIPLER + 1,
                          dbCommThreads[i].dbCommState.delayBins[BIN_GREATER]);
#if MULTIPLER > 1
                CliPrintf(hCli,
                          "\tCount < %lu             = %lu\r\n",
                          MULTIPLER - 1,
                          dbCommThreads[i].dbCommState.delayBins[BIN_LESS]);

#endif
            }
            success = 1;
        } else if (argc == MATCH_IDX(ID_IDX) && strcmp(argv[CMD_IDX], "clear") == 0) {
            if (strcmp(argv[ID_IDX], "all") == 0) {
                min = 0;
                max = MAX_CS_ID;
                noCncPoolMemoryCnt = 0;
            } else {
                uint32_t idx = atoi(argv[ID_IDX]);
                if (idx >= MAX_CS_ID) {
                    CliPrintf(hCli, "value %d is out of range 0-3 or all\r\n", idx);
                    return success;
                }
                min = idx;
                max = idx + 1;
            }
            success = 1;
            for (int i = min; i < max; i++) {
                dbCommThreads[i].dbCommState.newSensorData = 0;
                dbCommThreads[i].dbCommState.sensorUID = 255;
                dbCommThreads[i].dbCommState.rxCmdsCnt = 0;
                for (int j = 0; j < NUMBER_OF_SENSOR_READINGS; j++) {
                    WRITE_XBITVALUE(dbCommThreads[i].dbCommState.sensorPayload.adc[j].VALUE_X_BIT, 0);
                }
            }
        } else if (argc == MATCH_IDX(DATA_IDX) && strcmp(argv[CMD_IDX], "send") == 0) {
            int dbId = atoi(argv[ID_IDX]);
            int addr = atoi(argv[DATA_IDX]);
            // send test packet to Device at destination
            static uint8_t xInfo = 0x34; // pick a random number
            xInfo++;
            cncMsgPayload_t payload = {.cncMsgPayloadHeader.peripheral = PER_ADC, .cncMsgPayloadHeader.action = 0, .cncMsgPayloadHeader.addr = addr};
            DPRINTF_INFO("peripheral=%d, xinfo=%d, addr=%d xInfo=%d\r\n",
                    payload.cncMsgPayloadHeader.peripheral,
                    payload.cncMsgPayloadHeader.action,
                    payload.cncMsgPayloadHeader.addr,
                    xInfo);

            dbProcSendMsg(dbId, SPICMD_CNC, dbCommCliCallback, (uint32_t)hCli, (cncPayload_tp)&payload);

            osStatus status = stallCliUntilComplete(WAIT_MS(20000));

            if (status == osOK) {
                success = 1;
            } else {
                DPRINTF_ERROR("adc cli command timeout\r\n");
                success = 0;
            }
        } else if (argc == MATCH_IDX(DATA_IDX) && strcmp(argv[CMD_IDX], "loopback_mb") == 0) {
            int dbId = atoi(argv[ID_IDX]);
            int value = atoi(argv[DATA_IDX]);
            cncMsgPayload_t payload = {.cncMsgPayloadHeader.peripheral = PER_NOP, .cncMsgPayloadHeader.action = CNC_ACTION_WRITE, .value = value};
            dbProcSendMsg(
                dbId, SPICMD_SET_SKIP_WAIT_FOR_RESPONSE, dbCommCliCallback, (uint32_t)hCli, (cncPayload_tp)&payload);
            success = 1;
        } else if (argc == MATCH_IDX(DATA_IDX) && strcmp(argv[CMD_IDX], "loopback_offset") == 0) {
            int dbId = atoi(argv[ID_IDX]);
            int offset = atoi(argv[DATA_IDX]);
            cncMsgPayload_t payload = {.cncMsgPayloadHeader.peripheral = PER_NOP, .cncMsgPayloadHeader.action = CNC_ACTION_WRITE, .value = offset};
            dbProcSendMsg(dbId, SPICMD_SET_LOOPBACK_OFFSET, dbCommCliCallback, (uint32_t)hCli, (cncPayload_tp)&payload);
            success = 1;
        } else if (argc == MATCH_IDX(DATA_IDX) && strcmp(argv[CMD_IDX], "enable") == 0) {
            int dbId = atoi(argv[ID_IDX]);
            bool enable = !(atoi(argv[DATA_IDX]) == 0);
            if (dbId >= DB_TASK_START_ID && dbId < MAX_CS_ID && dbId < MAX_DB_TASKS_END_ID) {
                dbCommThreads[dbId].dbCommState.enabled = enable;
                dbCommThreads[dbId].dbCommState.disableCnt = 0;
                dbCommThreads[dbId].dbCommState.waitingForCNCResponse = false;
                enable ? dbTriggerEnable(dbId) : dbTriggerDisable(dbId);
                success = 1;
            }
        } else if (argc == MATCH_IDX(ID_IDX) && strcmp(argv[CMD_IDX], "gpioen") == 0) {
            CliPrintf(hCli, "HEX 23-0 08%x\r\n", registerGPIOMask[0]);
            success = 1;
        } else if (argc == MATCH_IDX(DATA_IDX) && strcmp(argv[CMD_IDX], "gpioen") == 0) {
            uint32_t dbId = atoi(argv[ID_IDX]);
            uint32_t enable = atoi(argv[DATA_IDX]);
            if (dbId < MAX_DB_TASKS) {
                uint32_t groupId = GET_GROUP_EVT_IDX(dbId);
                uint32_t idBit = GET_GROUP_EVT_ID(dbId);
                if (enable) {
                    registerGPIOMask[groupId] |= idBit;
                } else {
                    registerGPIOMask[groupId] &= ~idBit;
                }
                success = 1;
            }
        } else if (argc == MATCH_IDX(NO_ID_DATA_IDX) && stricmp(argv[CMD_IDX], "fillEthPkt") == 0) {
            bool quadType = (stricmp(argv[NO_ID_DATA_IDX], "true") == 0);
            testSensorFill(quadType);
        }
    }
    return success;
}

osStatus dbCommCliCallback(spiDbMbCmd_e spiDbMbCmd,
                           uint32_t cbId,
                           uint8_t xinfo,
                           spiDbMbPacketCmdResponse_t cmdResponse,
                           cncPayload_tp p_cncMsg) {
    CLI *hcli = (CLI *)cbId;
    (void)xinfo;

    if (p_cncMsg->cmd.cncMsgPayloadHeader.cncActionData.result == osOK) {
        CliPrintf(hcli, "Spi command returned successful\r\n");
        CliPrintf(hcli, "addr=%d, value=%d\r\n", p_cncMsg->cmd.cncMsgPayloadHeader.addr, p_cncMsg->cmd.value);
    } else {
        CliPrintf(hcli, "Write command failed %d\r\n", p_cncMsg->cmd.cncMsgPayloadHeader.cncActionData.result);
    }
    osStatus status = cliCommandComplete();
    if (status != osOK) {
        DPRINTF_ERROR("cliCommand Complete failed with %d\r\n", status);
    }
    return status;
}

bool dbCommTaskLoopbackParamsGet(int dbId, bool *loopbackSetting, int32_t *offsetSetting) {
    assert(dbId < MAX_CS_ID);
    dbCommThreadInfo_tp p_dbThread = (dbCommThreadInfo_tp)&dbCommThreads[dbId];
    if (p_dbThread == NULL) {
        return false;
    }
    *loopbackSetting = p_dbThread->dbCommState.loopbackSensor;
    *offsetSetting = p_dbThread->dbCommState.loopbackOffset;

    return true;
}

bool dbCommTaskLoopbackParamsSet(int dbId, bool loopbackSetting, int32_t offsetSetting) {
    assert(dbId < MAX_CS_ID);
    dbCommThreadInfo_tp p_dbThread = (dbCommThreadInfo_tp)&dbCommThreads[dbId];
    if (p_dbThread == NULL) {
        return false;
    }
    p_dbThread->dbCommState.loopbackSensor = loopbackSetting;
    p_dbThread->dbCommState.loopbackOffset = offsetSetting;
    if (p_dbThread->dbCommState.enabled == false) {
        p_dbThread->dbCommState.enabled = true;
        if (registerDbStateCbFnPtr != NULL) {
            registerDbStateCbFnPtr(p_dbThread->daughterBoardId, p_dbThread->dbCommState.enabled);
        }
        dbTriggerEnable(p_dbThread->daughterBoardId);
    }
    return true;
}

void updateDBCrcErrorOccurred(int dbId) {
    assert(dbId < MAX_CS_ID);
    dbCommThreadInfo_tp p_dbThread = (dbCommThreadInfo_tp)&dbCommThreads[dbId];
    if (p_dbThread == NULL) {
        return;
    }
    p_dbThread->dbCommState.crcErrors++;
}
