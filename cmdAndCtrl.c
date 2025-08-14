/**
 * @file
 * Functions for command and control task.
 *
 * Copyright Nuvation Research Corporation 2023-2024. All Rights Reserved.
 * www.nuvation.com
 */

#include "cmdAndCtrl.h"

#include "cli/cli_print.h"
#include "cmsis_os.h"
#include "debugPrint.h"
#include "taskWatchdog.h"
#ifdef STM32F411xE
#include "gpioDB.h"
#endif
#include <stdlib.h>
#include <string.h>

#define CNC_TASK_MSG_TIMEOUT_MS 500
#define CNC_MSG_PUT_TIMEOUT_MS 5

osPoolDef(cncMsgPool, CNC_MSG_POOL_DEPTH, cncInfo_t);
osPoolId cncMsgPool = NULL;

osMessageQDef(cncMsgQ, CNC_MSG_POOL_DEPTH, cncInfo_tp);
static osMessageQId cncMsgQ = NULL;

const strLUT_t commandLUT[CNC_WEB_CMD_MAX] = {
    {.name = "READ", .cmdId = CNC_ACTION_READ},
    {.name = "WRITE", .cmdId = CNC_ACTION_WRITE},
};

static void startCncTask(void const *argument);

void cncTaskInit(int priority, int stackSize) {

    cncMsgPool = osPoolCreate(osPool(cncMsgPool));
    assert(cncMsgPool != NULL);

    cncMsgQ = osMessageCreate(osMessageQ(cncMsgQ), NULL);
    assert(cncMsgQ != NULL);

    // create the cli task
    osThreadDef(cncTask, startCncTask, priority, 0, stackSize);
    osThreadId cncTaskHandle = osThreadCreate(osThread(cncTask), NULL);
    assert(cncTaskHandle != NULL);
}

void startCncTask(void const *argument) {
    osEvent evt;
    DPRINTF_CNC("CNC task started\r\n");
    watchdogAssignToCurrentTask(WDT_TASK_CNC);
    watchdogSetTaskEnabled(WDT_TASK_CNC, 1);

    while (1) {
        // wait for incoming data
        evt = osMessageGet(cncMsgQ, CNC_TASK_MSG_TIMEOUT_MS); // wait for message
        cncInfo_tp data;
        if (evt.status == osEventMessage) {
            data = evt.value.p;
            DPRINTF_CMD_STREAM_VERBOSE("Got msg from cncMsgQ \r\n");
            cncHandleMsg(data);
            osPoolFree(cncMsgPool, data);
        }

        watchdogKickFromTask(WDT_TASK_CNC);
    }
}

osStatus cncSendMsg(uint8_t destination,
                    spiDbMbCmd_e spiDbMbCmd,
                    cncPayload_tp p_cncMsg,
                    uint8_t xInfo,
                    cncCbFnPtr cbFnPtr,
                    uint32_t callbackId) {
    cncInfo_tp msg;
    msg = osPoolAlloc(cncMsgPool);
    if (msg == NULL) {
        return osErrorNoMemory;
    }
    if (p_cncMsg->cmd.cncMsgPayloadHeader.peripheral == PER_TEST_MSG) {
        DPRINTF_INFO("stop\r\n");
    }
    memcpy(&msg->payload, p_cncMsg, sizeof(cncPayload_t));
    msg->destination = destination;
    msg->cbFnPtr = cbFnPtr;
    msg->xInfo = xInfo;
    msg->cbId = callbackId;
    msg->cmd = spiDbMbCmd;
    DPRINTF_CMD_STREAM_VERBOSE("Put msg in cncMsgQ cbId=%d\r\n", msg->cbId);
    return (osMessagePut(cncMsgQ, (uint32_t)msg, CNC_MSG_PUT_TIMEOUT_MS));
}

// MB and DB have their own versions.
__attribute__((weak)) void cncHandleMsg(cncInfo_tp p_cncInfo) {
    assert(false);
}
