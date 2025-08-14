/*
 * MB_handleADC.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 19, 2023
 *      Author: jzhang
 */

#ifndef APP_INC_PERIPHERALS_MB_HANDLEADC_H_
#define APP_INC_PERIPHERALS_MB_HANDLEADC_H_

#include "mongooseHandler.h"
#include <stdbool.h>

/**
 * @fn webAdcStart
 *
 * @brief Handler for web API ADC start command.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webAdcStart(const char *jsonStr, int strLen);

/**
 * @fn webAdcStop
 *
 * @brief Handler for web API ADC stop command.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webAdcStop(const char *jsonStr, int strLen);

webResponse_tp webAdcGetDuty(const char *jsonStr, int strLen);
webResponse_tp webAdcGetFreq(const char *jsonStr, int strLen);
webResponse_tp webAdcSetDuty(const char *jsonStr, int strLen);
webResponse_tp webAdcSetFreq(const char *jsonStr, int strLen);

webResponse_tp webPwmDdsStart(const char *jsonStr, int strLen);
webResponse_tp webPwmDdsStop(const char *jsonStr, int strLen);
webResponse_tp webPwmDdsGetDuty(const char *jsonStr, int strLen);
webResponse_tp webPwmDdsGetFreq(const char *jsonStr, int strLen);
webResponse_tp webPwmDdsSetDuty(const char *jsonStr, int strLen);
webResponse_tp webPwmDdsSetFreq(const char *jsonStr, int strLen);

webResponse_tp webRLDriveConnectedStatusGet(const char *jsonStr, int strLen);

webResponse_tp webRLDriveFunctionSet(const char *jsonStr, int strLen);
webResponse_tp webRLDriveFunctionGet(const char *jsonStr, int strLen);

webResponse_tp webLeadOffComparatorThresholdSet(const char *jsonStr, int strLen);
webResponse_tp webLeadOffComparatorThresholdGet(const char *jsonStr, int strLen);

webResponse_tp webLeadOffDetectionModeSet(const char *jsonStr, int strLen);
webResponse_tp webLeadOffDetectionModeGet(const char *jsonStr, int strLen);

webResponse_tp webIleadOffCurrentSet(const char *jsonStr, int strLen);
webResponse_tp webIleadOffCurrentGet(const char *jsonStr, int strLen);

webResponse_tp webEcg12FleadOffFreqSet(const char *jsonStr, int strLen);
webResponse_tp webEcg12FleadOffFreqGet(const char *jsonStr, int strLen);

webResponse_tp webLeadOffNegativeStateGet(const char *jsonStr, int strLen);
webResponse_tp webLeadOffPositiveStateGet(const char *jsonStr, int strLen);

webResponse_tp webLeadConnectionStatesGet(const char *jsonStr, int strLen);

webResponse_tp webPaceDetectReset(const char *jsonStr, int strLen);
webResponse_tp webPaceDetectGet(const char *jsonStr, int strLen);
#endif /* APP_INC_PERIPHERALS_MB_HANDLEADC_H_ */
