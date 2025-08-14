/*
 * MB_gatherTask.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 30, 2023
 *      Author: rlegault
 */

#ifndef APP_INC_MB_GATHERTASK_H_
#define APP_INC_MB_GATHERTASK_H_

#include "ctrlSpiCommTask.h"
#include "json.h"
#include "saqTarget.h"
#include <lwip/api.h>
#include <lwip/inet.h>
#include <lwip/opt.h>
#include <lwip/sys.h>
#include <stdbool.h>

#define macro_IPTYPE(T)                                                                                                \
    T(IPTYPE_UDP)                                                                                                      \
    T(IPTYPE_TCP)                                                                                                      \
    T(IPTYPE_MAX)

#ifdef GENERATE_IPTYPE_STRING_NAMES
GENERATE_ENUM_STRING_NAMES(macro_IPTYPE, IPTYPE_e)
#else
extern const char *IPTYPE_e_Strings[];
#endif
GENERATE_ENUM_LIST(macro_IPTYPE, IPTYPE_e)
#undef macro_IPTYPE
/**
 * @fn
 *
 * @brief Initialize the main board gather task
 *
 * This task is responsible for gathering the data from the various daughter
 * boards and sending the data to the udp server for analysis
 *
 * @param[in] priority: Set the task priority
 *
 * @param[in] stackSize: set the stask size for the task.
 **/
void mbGatherTaskInit(int priority, int stackSize);

/**
 * @fn
 *
 * @brief Initialize the thread that waits for connections to the TCP Server
 *
 * This task is responsible for waiting for connections and updating the
 * global connection value. It only allows one connection at a time.
 *
 * @param[in] priority: Set the task priority
 *
 * @param[in] stackSize: set the stask size for the task.
 **/
void tcpDataServerWaitTaskInit(int priority, int stackSize);
/**
 * @fn
 *
 * @brief Set the udp connection for UDP transmission destination
 *
 * @param[in] p_conn: connection information for NETCONN_SEND
 *
 **/
void updateUdpConnection(struct netconn *p_conn);

/**
 * @fn
 *
 * @brief Update this boards data for the next transmission
 *
 * @param[in] p_threadInfo: Daughter board process info
 *
 * @param[in] sensorReadings: Array of reading values
 *
 * @param[in] sensorReadingSz:
 *
 **/
void updateSensorData(dbCommThreadInfo_tp p_threadInfo, uint8_t *sensorReadings, size_t sensorReadingSz);

/**
 * @fn
 *
 * @brief Set the state of the board, for stats gathering information
 *
 * @param[in] boardId: identify the source of the readings
 *
 * @param[in] enable: Identify the new/current state of the board.
 *
 **/
void setDaughterboardState(int boardId, bool enable);

/**
 * @fn
 *
 * @brief Set the UDP packet interval
 *
 * @param[in] interval_ms: interval value in milli seconds
 *
 *
 **/
void setUdpSendInterval(uint32_t interval_ms);

/**
 * @fn
 *
 * @brief Print the current stream data for the identified sensor board
 *
 * @param[in] hCli: pointer to CLI print destination
 * @param[in] dbId: sensor board index 0-23
 *
 **/
void printStreamData(CLI *hCli, int dbId);

/**
 * @fn
 *
 * @brief Fill the entire data ethernet structure with new data
 *        that increments for each sensor and board.
 *
 * @param[in] quadImuType: IMU data can be in either quaternion(TRUE) or euler (FALSE) format
 *
 **/
void testSensorFill(bool quadImuType);

/**
 * @fn
 *
 * @brief Test the Stored configuration against the installed boards.
 *
 * @return True if all slots match installed boards else false.
 *
 **/
bool verifySensorConfiguration(void);

/**
 * @fn
 *
 * @brief Display any configuration detailed error information.
 *
 * @param hCli, UART handle to write data to.
 *
 **/
void cliDetailConfigurationError(CLI *hCli);

/**
 * @fn
 *
 * @brief add configuration error details to json array
 *
 * @param jsonArray,  add configuration error details to json array
 *
 **/
void jsonDetailConfigurationError(json_object *jsonArray);

/**
 * @fn
 *
 * @brief add configuration settings to json array for the destination
 * @note these are the run time settings and do not reflect any changes made
 *       to the registers that are only read on boot.
 *
 * @param destination, 0-23, -1 (all) location to read the data from, if -1 is sent then
 * all configuration slots are read
 * @param jsonArray, add configuration error details to json array
 *
 **/
void jsonAddConfigSettings(int destination, json_object *jsonResponse);

/**
 * @fn
 *
 * @brief update the underlying configuration register with the new board type
 * @note this change only takes affect after rebooting
 *
 * @param destination, 0-23, -1 (all) location to read the data from, if -1 is sent then
 * all configuration slots are read
 * @param boardType, slot to update.
 *
 **/
void alterConfigSettings(int destination, BOARDTYPE_e boardType);

/**
 * @fn
 *
 * @brief add configuration status information to jsonArray
 *
 * @param [in] destination, 0-23, -1 (all) location to read the data from, if -1 is sent then
 * all configuration slots are read
 * @param [out] jsonArray, location to add the configuration status data to.
 *
 **/
void jsonAddConfigStatus(int destination, json_object *jsonArray);

/**
 * @fn
 *
 * @brief return true if the dbId board is a COIL Driver board.
 *
 * @param [in] dbId, sensor board slot 0-23,
 *
 **/
bool coilDriverBoard(uint32_t dbId);

/**
 * @fn
 *
 * @brief copy the streaming data to the buffer
 *
 * @param [out] buf, location to write the data to
 *
 * @param [in] bufSz, size of buffer in bytes.
 *
 * @param [in] dbId, sensor board slot 0-23,
 *
 * @return number of bytes copied to the buffer
 **/
uint32_t printStreamDataToBuffer(char *buf, uint32_t bufSz, uint32_t dbId);

/**
 * @fn
 *
 * @brief Find all occurrences of target in SENSOR_BOARD_xx register and replace with
 *        value for BOARDTYPE_EMPTY (8)
 *
 * @note the empty value changed from 3 to 7 and thus the need for this function.
 *
 * @param [in] target, value to test against and replace.
 *
 **/
void correctTargetToEmpty(uint32_t target);

/**
 * @fn
 *
 * @brief Find all occurrences of target in SENSOR_BOARD_xx register and replace with
 *        value of newValue
 *
 *
 * @param [in] target, value to test against and replace.
 * @param [in] newValue, value to use as replacement.
 *
 **/
void correctTargetToNewValue(uint32_t target, uint32_t newValue);
#endif /* APP_INC_MB_GATHERTASK_H_ */
