/*
 * MB_handleLED.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 18, 2023
 *      Author: jzhang
 */

#ifndef APP_INC_PERIPHERALS_MB_HANDLELED_H_
#define APP_INC_PERIPHERALS_MB_HANDLELED_H_

#include "mongooseHandler.h"
#include <stdbool.h>

/**
 * @fn webLedStateGet
 *
 * @brief Handler for web API getting LED blinking state
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webLedStateGet(const char *jsonStr, int strLen);

/**
 * @fn webLedStateSet
 *
 * @brief Handler for web API setting LED blinking state
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webLedStateSet(const char *jsonStr, int strLen);

#endif /* APP_INC_PERIPHERALS_MB_HANDLELED_H_ */
