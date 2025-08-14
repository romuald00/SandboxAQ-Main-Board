/*
 * MB_handleLogs.h
 *
 *  Copyright Nuvation Research Corporation 2018-2025. All Rights Reserved.
 *
 *  Created on: Jan 22, 2025
 *      Author: rlegault
 */

#ifndef APP_INC_PERIPHERALS_MB_HANDLELOGS_H_
#define APP_INC_PERIPHERALS_MB_HANDLELOGS_H_

#include "mongooseHandler.h"

#include "cli/cli.h"
#include "cli/cli_print.h"

/**
 * @fn
 *
 * @brief      from a web request return the contents of the identified debug
 *             log
 *
 * @param[in]  jsonStr Web json parameter buffer
 *
 * @param[in]  strLen length of json parameter buffer
 *
 * @return     webResponse structure to send to requester
 *
 */
webResponse_tp webDebugLogGet(const char *jsonStr, int strLen);

/**
 * @fn
 *
 * @brief      from a web request clear the debug log at destination
 *
 * @param[in]  jsonStr Web json parameter buffer
 *
 * @param[in]  strLen length of json parameter buffer
 *
 * @return     webResponse structure to send to requester
 *
 */
webResponse_tp webDebugLogClear(const char *jsonStr, int strLen);

/**
 * @fn
 *
 * @brief       from a CLI request print the identified sensor board log
 *
 * @param[in]   hCli  pointer to buffer to write result
 *
 * @param[out]  destination, sensor board to request sensor log
 *
 * @return      1=successful, 0=failed
 *
 */
uint32_t cliSensorLogGet(CLI *hCli, uint32_t destination);

/**
 * @fn
 *
 * @brief       from a CLI request clear the identified sensor board log
 *
 * @note that this function will cause the sensor board to place the
 * data in string format into the large buffer location and will
 * then be printed out from here.
 * The use of this function will cause multiple data points to be missed by
 * all sensor boards on this spi bus.
 *
 * @param[in]   hCli  pointer to buffer to write result
 *
 * @param[out]  destination, sensor board to request sensor log
 *
 * @return      1=successful, 0=failed
 *
 */
uint32_t cliClearSensorLog(CLI *hCli, uint32_t destination);

/**
 * @fn
 *
 * @brief      from a web request get the stat information for
 *             the identified type at the destination both of which
 *             are in the jsonStr
 *
 * @note that this function will cause the sensor board to place the
 * data in string format into the large buffer location and will
 * then be printed out from here.
 * The use of this function will cause multiple data points to be missed by
 * all sensor boards on this spi bus.
 *
 * @param[in]  jsonStr Web json parameter buffer
 *
 * @param[in]  strLen length of json parameter buffer
 *
 * @return     webResponse structure to send to requester
 *
 */
webResponse_tp webDebugStats(const char *jsonStr, int strLen);

/**
 * @fn
 *
 * @brief      from a web request get the stat information for
 *             the identified type at the main board of which
 *             the type is in the jsonStr
 *
 *
 * @param[in]  jsonStr Web json parameter buffer
 *
 * @param[in]  strLen length of json parameter buffer
 *
 * @return     webResponse structure to send to requester
 *
 */
webResponse_tp webDebugStatsMb(const char *jsonStr, int strLen);

/**
 * @fn
 *
 * @brief   Dump to the cli the hex and ascii value for the Large Rx buffer
 *
 *
 * @param[in]   hCli  pointer to buffer to write result
 *
 * @param[in]  count  number of bytes to read from memory
 *
 * @return      1=successful, 0=failed
 *
 */
void dumpRxBuffer(CLI *hCli, uint32_t count);

#endif /* APP_INC_PERIPHERALS_MB_HANDLELOGS_H_ */
