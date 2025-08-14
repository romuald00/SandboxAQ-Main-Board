/*
 * MB_handleDac.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 7, 2023
 *      Author: jzhang
 */

#ifndef APP_INC_PERIPHERALS_MB_HANDLEDAC_H_
#define APP_INC_PERIPHERALS_MB_HANDLEDAC_H_

#include "mongooseHandler.h"
#include <stdbool.h>

#define MAX_SLOT_ID 2

/**
 * @fn testSensorSlotValue
 *
 * @brief Test if the Sensor Id is 0 or 1. 2 sensors per board
 *
 * @param[in] webResponse_tp p_webResponse: Web response structure
 * @param[in] int slot: slot from web API call
 *
 * @return bool: True if value is valid otherwise false
 **/
bool testSensorSlotValue(webResponse_tp p_webResponse, int slot);

/**
 * @fn testVoltageValueExc
 *
 * @brief Test if inputted voltage is within excitation DAC limits.
 *
 * @param[in] webResponse_tp p_webResponse: Web response structure
 * @param[in] int voltage_mv: Voltage from web API call
 *
 * @return bool: True if value is valid otherwise false
 **/
bool testVoltageValueExc(webResponse_tp p_webResponse, int voltage_mv);

/**
 * @fn testVoltageValueComp
 *
 * @brief Test if inputted voltage is within compensation DAC limits.
 *
 * @param[in] webResponse_tp p_webResponse: Web response structure
 * @param[in] int voltage_mv: Voltage from web API call
 *
 * @return bool: True if value is valid otherwise false
 **/
bool testVoltageValueComp(webResponse_tp p_webResponse, int voltage_mv);

/**
 * @fn webDacCompGet
 *
 * @brief Handler for web API get DAC compensation command.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDacCompGet(const char *jsonStr, int strLen);

/**
 * @fn webDacCompSet
 *
 * @brief Handler for web API set DAC compensation command.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDacCompSet(const char *jsonStr, int strLen);

/**
 * @fn webDacExcGet
 *
 * @brief Handler for web API get DAC excitation command.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDacExcGet(const char *jsonStr, int strLen);

/**
 * @fn webDacExcSet
 *
 * @brief Handler for web API set DAC excitation command.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDacExcSet(const char *jsonStr, int strLen);

#endif /* APP_INC_PERIPHERALS_MB_HANDLEDAC_H_ */
