/*
 * MB_handleFan.h
 *
 *  Copyright Nuvation Research Corporation 2018-2025. All Rights Reserved.
 *  Created on: Jan 16, 2025
 *      Author: rlegault
 */

#ifndef APP_INC_PERIPHERALS_MB_HANDLEFAN_H_
#define APP_INC_PERIPHERALS_MB_HANDLEFAN_H_

/**
 * @fn webFanSpeedGet
 *
 * @brief Handler for web API getting fan speed setting
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webFanSpeedGet(const char *jsonStr, int strLen);

/**
 * @fn webFanSpeedSet
 *
 * @brief Handler for web API setting fan speed
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webFanSpeedSet(const char *jsonStr, int strLen);

/**
 * @fn webFanTachometerGet
 *
 * @brief Handler for web API getting fan Tachometer reading
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webFanTachometerGet(const char *jsonStr, int strLen);

/**
 * @fn webFanTemperatureGet
 *
 * @brief Handler for web API getting fan Temperature reading
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webFanTemperatureGet(const char *jsonStr, int strLen);

/**
 * @fn webFanRegisterGet
 *
 * @brief Handler for web API getting fan register value
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webFanRegisterGet(const char *jsonStr, int strLen);

/**
 * @fn webFanRegisterSet
 *
 * @brief Handler for web API setting fan register value
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webFanRegisterSet(const char *jsonStr, int strLen);

#endif /* APP_INC_PERIPHERALS_MB_HANDLEFAN_H_ */
