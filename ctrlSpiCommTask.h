/*
 * ctrlSpiCommTask.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 16, 2023
 *      Author: rlegault
 */

#ifndef APP_INC_CTRLSPICOMMTASK_H_
#define APP_INC_CTRLSPICOMMTASK_H_

#include "cmdAndCtrl.h"
#include "saqTarget.h"
#include "watchDog.h"
#include <stdlib.h>

typedef enum { BIN_TARGET, BIN_LESS, BIN_LESS1, BIN_GREATER1, BIN_GREATER, BIN_MAX } BIN_e;

typedef struct {
    bool enabled;
    bool newSensorData;
    uint8_t sensorUID; // xInfo changes with each new sensor data store last one
    uint32_t rxCmdsCnt;
    rx_dbNopPayload_t sensorPayload;
    bool waitingForCNCResponse;
    uint8_t xInfoMatch; // xInfo CNC Response message we need to match to.
    uint8_t responseDelayCnt;
    cncCbFnPtr cbFnPtr;
    uint32_t cbId;
    uint32_t disableCnt;    // number of consecutive erred packets
    bool loopbackSensor;    // DEBUG will skip sending sensor and mock sensor data
    int32_t loopbackOffset; // offset sensor data calculation
    uint32_t delayBins[BIN_MAX];
    uint32_t rxDataCnt;        // increment on every rx sensor packet from board
    uint32_t crcErrors;        // number of CRC errors
    uint32_t enabledCnt;       // number of times process has been enabled
    uint32_t procEnableAtTick; // Tick at which the proc was last enabled.
    uint32_t resendCNCCnt;     // number of times a CNC command was lost
    uint8_t *largeBuffer;      // pointer to the storage location of the next spi update
    uint32_t largeBufferSz;    // size of largeBuffer
} dbCommState_t, *dbCommState_tp;

typedef struct {
    uint8_t daughterBoardId;
    osMessageQId msgQId;
    osThreadId threadId;
    WatchdogTask_e watchDogId;
    dbCommState_t dbCommState;
    uint32_t dbGroupEvtIdx; // 0,1
    uint32_t dbGroupEvtId;  // if 0 above then 0-31, else 0-15 bit
    cncInfo_t copyLastCmd;  // Store the last CNC command in case we need to resend it
    uint32_t resendCnt;     // number of times the command has been resent, we just try once.
    uint16_t cmdUid;        // used to ensure the response is to the latest command sent.
} dbCommThreadInfo_t, *dbCommThreadInfo_tp;

/**
 *
 * Create the spi controller tasks
 *
 * @param[in]     priority thread priority
 * @param[in]     stackSz  thread stack size
 */

void ctrlSpiCommTaskInit(int priority, int stackSize);

/**
 * Called once a second to update the spi tx rate
 */
void updateSpiStats(void);

/**
 * update the daughter board enable count for the associated spi bus
 * based on enable state
 * @param[in]     destination identify the DB whose state has changed
 * @param[in]     enable identify if the board has been enabled or disabled
 *                for spi communication
 *
 */

void updateSPIEnableCount(uint8_t destination, bool enable);

/**
 *
 * Write the CMD response payload to the buffer for spi task to send to the main board.
 * Also signals the spi task that new data is ready
 *
 * @param[in]     destination on MB this identifies the DB to be communicated with
 * @param[in]     cmd       identifies the payload
 * @param[in]     xInfo     extra info that is payload dependent
 * @param[in]     cmdUid    Daughterboard task assigned cmd unique identifier for response match
 * @param[in]     payload   cnc msg payload
 * @param[in]     cbFnPtr   when the command completes this function si called with the result
 * @param[in]     cbId      passed back when cbFnPtr is called
 * @param[in]     timeout   amount of time to wait to add to queue.
 *
 * @ret osStatus
 */

osStatus spiSendMsg(uint8_t destination,
                    spiDbMbCmd_e cmd,
                    uint8_t xInfo,
                    uint16_t cmdUid,
                    cncMsgPayload_tp payload,
                    cncCbFnPtr cbFnPtr,
                    uint32_t cbId,
                    uint32_t timeout_ms);

/*
 * Return the string name of the Chip select for this board
 *
 * @param[in]     Board Id
 *
 * @ret String containing board chip select pin: PG3
 *
 * */
char *csName(uint32_t csIdx);

/**
 * Return true if the associated spi thread is ready
 *
 * @param[in] destination used to identify which spi to test
 *
 * @ret bool true if spi thread is ready.
 *
 **/
bool SPIIsReady(uint32_t destination);

/**
 * @fn
 *
 * @brief copy the spi data to the buffer
 *
 * @param [out] buf, location to write the data to
 *
 * @param [in] bufSz, size of buffer in bytes.
 *
 * @param [in] spiId, spi id to gather information about
 *
 **/
void appendSpiStatsToBuffer(char *buf, uint32_t bufSz, uint32_t spiId);
#endif /* APP_INC_CTRLSPICOMMTASK_H_ */
