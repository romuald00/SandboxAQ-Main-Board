/*
 * MB_handlePwrCtrl.h
 *
 *  Copyright Nuvation Research Corporation 2018-2025. All Rights Reserved.
 *
 *  Created on: Feb 14, 2025
 *      Author: rlegault
 */

#ifndef LIB_INC_PWRCTRL_H_
#define LIB_INC_PWRCTRL_H_

#include "mongooseHandler.h"

#include <stdbool.h>

/**
 * @fn isPowerModeEn
 *
 * @brief Return the power mode state, if false then in low power mode
 *
 * @return true if power mode is high else return false.
 **/
bool isPowerModeEn(void);

/**
 * @fn powerModeSet
 *
 * @brief Set the power mode for the sensor boards
 *
 * @param[in] on: true turn clk and power on to the sensor boards
 *
 * @return osStatus
 **/

void powerModeSet(bool on);

webResponse_tp webPowerSet(const char *jsonStr, int strLen);
webResponse_tp webPowerGet(const char *jsonStr, int strLen);

#endif /* LIB_INC_PWRCTRL_H_ */
