/*
 * MB_handleDDS.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 8, 2023
 *      Author: jzhang
 */

#ifndef APP_INC_PERIPHERALS_MB_HANDLEIMU_H_
#define APP_INC_PERIPHERALS_MB_HANDLEIMU_H_

#include "mongooseHandler.h"
#include <stdbool.h>

/**
 * @fn webImuCmd
 *
 * @brief Handler for web API get IMU string command.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webImuCmd(const char *jsonStr, int strLen);

#endif /* APP_INC_PERIPHERALS_MB_HANDLEDDS_H_ */
