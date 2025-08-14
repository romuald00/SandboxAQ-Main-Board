/*
 * raiseIssue.h
 *  Control Red LED in thread on F411, on H743 use PWM ctrl
 *
 *   Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *   Created on: Sep 9, 2024
 *       Author: rlegault
 */

#ifndef LIB_INC_RAISEISSUE_H_
#define LIB_INC_RAISEISSUE_H_

#ifdef STM32H743xx
#include "json.h"
#endif
#include "registerParams.h"
#include "saqTarget.h"
#include "stmTarget.h"

/**
 * @fn hardwareFailure
 *
 * @brief Set the error state of the identified peripheral
 * @note it can only be cleared by a reboot.
 *
 * @param[in] peripheral, peripheral to set error state
 *
 * @return 0 osStatus
 **/
void hardwareFailure(PERIPHERAL_e peripheral);

#ifdef STM32H743xx

/**
 * @fn handleRedLedPriority
 *
 * @brief iterate over list of errors and set the RED led to the proper blinking rate
 *
 * @param[in] testRegState: Read the blink control register to see if we can change its state.
 **/
void handleRedLedPriority(bool testRegState);

/**
 * @fn configurationErrorRaise
 *
 * @brief Set the error state of the sensor board configuration mismatch to true
 *
 * @param[in] msg, configuration error message.
 *
 **/
void configurationErrorRaise(const char *msg);

/**
 * @fn configurationErrorClear
 *
 * @brief Set the error state of the sensor board configuration mismatch to false
 *
 *
 **/
void configurationErrorClear(void);

/**
 * @fn networkErrorRaise
 *
 * @brief Set the error state of the network connection status to true
 *
 * @param[in] msg, network error message.
 *
 **/
void networkErrorRaise(const char *msg);

/**
 * @fn networkErrorClear
 *
 * @brief Set the error state of the network connection status to false
 *
 *
 **/
void networkErrorClear(void);

/**
 * @fn errorListJson
 *
 * @brief Create json array of error types
 *
 * @param[in] json, json object to create the array within
 *
 **/
void errorListJson(json_object *json);

/**
 * @fn webAddSystemStatus
 *
 * @brief Create json response array of the System status
 *
 * @param[in] jsonObjectResponse, json object to create the array within
 *
 **/
void webAddSystemStatus(json_object *jsonObjectResponse);

#else
/**
 * @fn redLedInit
 *
 * @brief Initialize the RED LED control thread.
 *
 * @param[in] priority, task priority
 * @param[in] stackSize, thread stack size
 *
 * @return 0 osStatus
 **/
void redLedInit(int priority, int stackSize);

#endif

/**
 * @fn mapPeriperalError
 *
 * @brief get a bit map representation of the peripheral errors.
 *
 * @param[out] regInfo, fill with value of peripheral map errors.
 *
 * @return 0 RETURN_OK
 **/
RETURN_CODE mapPeriperalError(const registerInfo_tp regInfo);

/**
 * @fn testForPeripheralError
 *
 * @brief iterate over peripheral error table, return true if any peripherals
 *        are in error, else false
 *
 * @return return true if any peripherals are in error, else false
 **/
bool testForPeripheralError(void);

/**
 * @fn peripheralErrorAppend
 *
 * @brief Append the peripheral error to the string buffer up to max size
 *
 * @param[in] buf, configuration error message.
 *
 * @param[in] maxsz, amount of buffer space to add the string
 *
 **/
void peripheralErrorAppend(char *buf, uint32_t maxSz);

#endif /* LIB_INC_RAISEISSUE_H_ */
