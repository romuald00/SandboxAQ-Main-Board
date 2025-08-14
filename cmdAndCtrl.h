/*
 * cmdAndCtrl.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 29, 2023
 *      Author: rlegault
 */

#ifndef APP_INC_CMDANDCTRL_H_
#define APP_INC_CMDANDCTRL_H_

#include "cli/cli.h"
#include "cmsis_os.h"
#include "saqTarget.h"
#include <stdbool.h>

#define CNC_MSG_PAYLOAD_STR_SZ (sizeof(dbCmdPayload_t) - sizeof(cncMsgPayloadHeader_t))

#if 0// macro to print sizeof values at compile time
char (*__kaboom)[CNC_MSG_PAYLOAD_STR_SZ] = 1;  // 52 bytes
void kaboom_print(void) {
    printf("%d",__kaboom);
}
#endif

typedef struct __attribute__((packed)) cncSpiMsgPayloadHeader {
    uint8_t peripheral;  /** cast to PERIPHERAL_e , high bit is read=0/write=1 */
    CNC_ACTION_e action; /** CNC ACTION */
    uint16_t shortCncId; /** returned in structure spiDbMbPacketCmdResponse_t.cmdUid */
    uint32_t addr;       /** address of the peripheral to access */
    union {
        uint32_t result;        /** on return from daughterboard success = 0, else errorcode */
        uint32_t largeBufferSz; /** on CNC_ACTION_READ_LARGE_BUFFER
                                    size of next transmission from sensor board to main board size */
    } cncActionData;
} cncMsgPayloadHeader_t, *cncMsgPayloadHeader_tp;
typedef struct __attribute__((packed)) cncSpiMsgPayload {
    cncMsgPayloadHeader_t cncMsgPayloadHeader;
    union {
        uint8_t *largeBufferAddr;
        uint32_t value; /** value read or to be written */
        // in structure above 4 Uint8_t  and 2 uint32_t
        char str[CNC_MSG_PAYLOAD_STR_SZ];
        float fvalue;
        bool boolean;
    };
} cncMsgPayload_t, *cncMsgPayload_tp;

typedef union __attribute__((packed)) {
    rx_dbNopPayload_t sensor;
    cncMsgPayload_t cmd;
    uint8_t u8Array[sizeof(dbCmdPayload_t)];
} cncPayload_t, *cncPayload_tp;

/**
 * @struct cncCbFnPtr
 * When a cnc command response has been recieved then this callback is called
 *
 * @param cbId, value passed in when assigned the cbFn
 * @param xInfo, xtra info from the SPI interface
 * @param cmdResponse, if a short cnc message was sent, this will contain its response.
 * @param payload, either sensor data and sensorUID or CNC payload
 *
 * @Note SensorUID is changed each time a new value is read by the daughter board.
 */
typedef osStatus (*cncCbFnPtr)(spiDbMbCmd_e spiDbMbCmd,
                               uint32_t cbId,
                               uint8_t xInfo,
                               spiDbMbPacketCmdResponse_t cmdResponse,
                               cncPayload_tp payload);

/**
 * @struct cncInfo_t
 * This structure is used to communicate with Daughter Board procs on the main board and the
 * main board and daughter board CNC tasks.
 * @Note: This structure is available from an osPool so use the osPoolAlloc and osPoolFree methods
 * to get instances to pass to other tasks.
 */
typedef struct __attribute__((packed)) cncInfo {
    uint8_t destination; // unused on the daughter board cnc.
    uint8_t cmd;         // spiDbMbCmd_e, cannot specify size for enum so cast it when needed
    uint8_t xInfo;
    uint8_t dummy;
    spiDbMbPacketCmdResponse_t cmdResponse;

    cncPayload_t payload;
    cncCbFnPtr cbFnPtr;
    uint32_t cbId;
} cncInfo_t, *cncInfo_tp;

// Check that the payload fits in the SPI payload field
_Static_assert(sizeof(cncPayload_t) == sizeof(spiDBMBPacket_payload_t), "Data structure is too large for SPI payload");

#if 0 // macro to print sizeof values at compile time
char (*__kaboom)[sizeof(spiDBMBPacket_payload_t)] = 1; // 40 bytes
char (*__kaboom)[sizeof(cncPayload_t)] = 1; // 40 bytes
void kaboom_print(void) {
    printf("%d",__kaboom);
}
#endif



/**
 * @fn cncTaskInit
 *
 * @brief initialize the Command and Control Task.
 *
 * @param[in]     priority thread priority
 * @param[in]     stackSz  thread stack size
 **/
void cncTaskInit(int priority, int stackSize);

/**
 * @fn cncSendMsg
 *
 * @brief Send a command and control message to the appropriate task
 *
 * @param[in] destination: identify board destination. 0XFE = MB, 0-47 DB, 0xFF local
 * @param[in] p_cncMsg: Cli uart identifier.
 * @param[in] xInfo: Number of arguments received wit the command.
 * @param[in] p_cbFn: when the cnc message is complete this method will be called.
 * @param[in] cbId: passed to the cbFn
 *
 * @return osStatus result
 **/
osStatus cncSendMsg(uint8_t destination,
                    spiDbMbCmd_e spiDbMbCmd,
                    cncPayload_tp p_cncMsg,
                    uint8_t xInfo,
                    cncCbFnPtr p_cbFn,
                    uint32_t cbId);

/**
 * @fn cncHandleMsg
 *
 * @brief Handle RX cnd messages from either SPI CLI or Web.
 *    The mainboard and daughter board have different handlers.
 *
 * @param[in] data: data recieved from the tasks message Q.
 *
 **/
void cncHandleMsg(cncInfo_tp data);

#endif /* APP_INC_CMDANDCTRL_H_ */
