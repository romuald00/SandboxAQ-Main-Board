/*
 * MB_handleReg.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Jan 16, 2024
 *      Author: jzhang
 */

#ifndef APP_INC_PERIPHERALS_MB_HANDLEREG_H_
#define APP_INC_PERIPHERALS_MB_HANDLEREG_H_

#include "mongooseHandler.h"
#include <stdbool.h>

/**
 * @fn webUniqueIdGet
 *
 * @brief Handler for web API getting daughterboard unique ID
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webUniqueIdGet(const char *jsonStr, int strLen);

/**
 * @fn webUniqueIdSet
 *
 * @brief Handler for web API setting daughterboard unique ID
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webUniqueIdSet(const char *jsonStr, int strLen);

/**
 * @fn webLoopbackGetDb
 *
 * @brief Handler for web API getting daughterboard loopback setting
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webLoopbackGetDb(const char *jsonStr, int strLen);

/**
 * @fn webLoopbackSetDb
 *
 * @brief Handler for web API setting daughterboard loopback setting
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webLoopbackSetDb(const char *jsonStr, int strLen);

/**
 * @fn webLoopbackIntervalGetDb
 *
 * @brief Handler for web API getting daughterboard loopback interval setting
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webLoopbackIntervalGetDb(const char *jsonStr, int strLen);

/**
 * @fn webLoopbackIntervalSetDb
 *
 * @brief Handler for web API setting daughterboard loopback interval setting
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webLoopbackIntervalSetDb(const char *jsonStr, int strLen);

/**
 * @fn webSerialNumberGetDb
 *
 * @brief Handler for web API getting daughterboard serial number
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webSerialNumberGetDb(const char *jsonStr, int strLen);

/**
 * @fn webSerialNumberSetDb
 *
 * @brief Handler for web API setting sensor board serial number
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webSerialNumberSetDb(const char *jsonStr, int strLen);

/**
 * @fn webHwRevGetDb
 *
 * @brief Handler for web API getting sensor board Hardware Revision
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webHwRevGetDb(const char *jsonStr, int strLen);

/**
 * @fn webHwRevSetDb
 *
 * @brief Handler for web API setting sensor board Hardware Revision
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webHwRevSetDb(const char *jsonStr, int strLen);

/**
 * @fn webHwTypeGetDb
 *
 * @brief Handler for web API getting sensor board Hardware Type
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webHwTypeGetDb(const char *jsonStr, int strLen);

/**
 * @fn webHwTypeSetDb
 *
 * @brief Handler for web API setting sensor board Hardware Type
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webHwTypeSetDb(const char *jsonStr, int strLen);

/**
 * @fn webReboot
 *
 * @brief Handler for web API rebooting daughterboard
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webReboot(const char *jsonStr, const int strLen);

/**
 * Send a request to the individual or all sensor boards to handle
 * the Command And Control command
 * Can only handle String values and it must be null terminated.
 *
 * @param[out] json_object jsonResponse json object response that is filled
 * @param[in]     dbId         on MB this identifies the DB to be communicated with, can be ALL
 * @param[in]     commandEnum  identifies the CNC command
 * @param[in]     addr      Register Addr on destination
 * @param[in]     str     If a write command then this contains the UINT value
 * @param[in]     uid       unique identifier
 * @param[in]     valueName used in JSON response to identify the value read or written.
 *
 * @ret osStatus osOK or osErrorNoMemory
 */
osStatus handleCncRequestStrJson(json_object *jsonResponse,
                                 int dbId,
                                 CNC_ACTION_e commandEnum,
                                 uint32_t addr,
                                 const char *str,
                                 uint32_t uid,
                                 char *valueName);

#endif /* APP_INC_PERIPHERALS_MB_HANDLEREG_H_ */
