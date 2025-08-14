/*
 * dbCommTask.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 17, 2023
 *      Author: rlegault
 */

#ifndef APP_INC_DBCOMMTASK_H_
#define APP_INC_DBCOMMTASK_H_

#include "cmdAndCtrl.h"
#include "cmsis_os.h"
#include "saqTarget.h"

#define GROUP_EVT_CTRL_BITS 24
#define GET_GROUP_EVT_ID(daughterBoardId) (1 << (daughterBoardId % GROUP_EVT_CTRL_BITS))
#define GET_GROUP_EVT_IDX(daughterBoardId) (daughterBoardId / GROUP_EVT_CTRL_BITS)

/**
 *
 * Create all Daughter board communication tasks and associated message queues.
 *
 * @param[in]     priority thread priority
 * @param[in]     stackSz  thread stack size
 **/
void dbCommTaskInit(int priority, int stackSize);

/**
 * Write the CMD message and payload to the buffer for db task identified by dbId.
 *
 * @param[in]     dbId      on MB this identifies the DB to be communicated with
 * @param[in]     cmd       identifies the payload
 * @param[in]     cbFnPtr   when the command completes this function si called with the result
 * @param[in]     cbId      passed back when cbFnPtr is called
 * @param[in]     payload   cnc msg payload
 *
 * @ret osStatus osOK or osErrorNoMemory
 */
osStatus dbProcSendMsg(int dbId, spiDbMbCmd_e spiDbMbCmd, cncCbFnPtr cbFnPtr, uint32_t cbId, cncPayload_tp payload);

/**
 * Parse the string elements in argv for commands and parameters and retrieves the requested info.
 *
 * @param[in]     hCli      uart to write the response on
 * @param[in]     argc      number of arguments in argv
 * @param[in]     argv      array of parameters in string format
 *
 * @ret 1 if success else 0
 */
int16_t dbProcCliCmd(CLI *hCli, int argc, char *argv[]);

/**
 * Check if the all task initialization is complete
 *
 * @ret bool TRUE if the task initializations are complete
 */
bool dbCommTasksInitComplete(void);

/**
 * Send a message to all Comm Tasks to send a new message to the daughter
 * board over SPI
 */
void dbCommTasksignalAll(void);

/**
 * Tn the thread state information set the flag enable to true.
 * For all dbComm threads
 */
void dbCommTaskEnableAll(void);

/**
 * Enable or disable the daughter board task associated with dbId
 *
 * @param[in] dbId  Daughter board to set state to
 * @param[in] enable  new daughter board state
 */
void dbCommTaskEnable(uint32_t dbId, bool enable);

/**
 * Call the callback when the state of the daughter board connection to the SPI
 * changes
 *
 * @param[in] daughterBoardId
 * @param[in] enabled: Latest state
 */
void registerDbStateCallback(void (*cbFnPtr)(int daughterBoardId, bool enabled));

/**
 * Get the loopback parameters values loopback setting and offset setting
 *
 *
 * @param[in] dbId: identify daughterboard to query
 * @param[out] loopbackSetting: current loopback setting
 * @param[out]  offsetSetting: value of the offset that the output of the loopback is using
 */
bool dbCommTaskLoopbackParamsGet(int dbId, bool *loopbackSetting, int32_t *offsetSetting);

/**
 * Set the loopback parameters values loopback setting and offset setting
 *
 *
 * @param[in] dbId: identify daughter board to query
 * @param[in] loopbackSetting: current loopback setting
 * @param[in] offsetSetting: value of the offset that the output of the loopback is using
 */
bool dbCommTaskLoopbackParamsSet(int dbId, bool loopbackSetting, int32_t offsetSetting);

/**
 * Return if the sensor board is present
 *
 *
 * @param[in] dbId: identify daughter board to query
 * @return true if board is present
 */
bool sensorBoardPresent(int dbId);

/**
 * update sensor board crc stats
 *
 *
 * @param[in] dbId: identify daughter board to query
 */
void updateDBCrcErrorOccurred(int dbId);

/**
 * @fn
 *
 * @brief copy the dbproc stats to the buffer
 *
 * @param [out] buf, location to write the data to
 *
 * @param [in] bufSz, size of buffer in bytes.
 *
 * @param [in] dbId, sensor board slot 0-23,
 *
 **/
void appendDbprocStatsToBuffer(char *buf, uint32_t size, uint32_t dbId);

#endif /* APP_INC_DBCOMMTASK_H_ */
