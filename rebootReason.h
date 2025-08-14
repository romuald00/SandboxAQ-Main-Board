/*
 * rebootReason.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Sep 2, 2024
 *      Author: rlegault
 */

#ifndef LIB_INC_REBOOTREASON_H_
#define LIB_INC_REBOOTREASON_H_

#include "saqTarget.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define macro_handlerREB(T)                                                                                            \
    T(REBOOT_WDT)                                                                                                      \
    T(REBOOT_ASSERT)                                                                                                   \
    T(REBOOT_HARDFAULT)                                                                                                \
    T(REBOOT_ERRORHANDLER)                                                                                             \
    T(REBOOT_NMI)                                                                                                      \
    T(REBOOT_MEMMANAGER)                                                                                               \
    T(REBOOT_BUSFAULT)                                                                                                 \
    T(REBOOT_USAGEFAULT)                                                                                               \
    T(REBOOT_UNKNOWN)                                                                                                  \
    T(REBOOT_MAX)

GENERATE_ENUM_LIST(macro_handlerREB, REBOOT_REASON_e)
#ifdef CREATE_REBOOT_REASON_STRING_LOOKUP_TABLE
GENERATE_ENUM_STRING_NAMES(macro_handlerREB, REBOOT_REASON_e)
#else
extern const char *REBOOT_REASON_e_Strings[];
#endif

#define ERROR_HANDLER                                                                                                  \
    rebootReasonSetPrintf(REBOOT_ERRORHANDLER, "%s %s:%d", __FUNCTION__, __FILE__, __LINE__);                          \
    Error_Handler();

/**
 * @fn rebootReasonClear
 *
 * @brief Clear the reboot reason flag to allow updates to occur
 *
 **/
void rebootReasonClear(void);

/**
 * @fn rebootReasonSet
 *
 * @brief Set the reboot reason in noload memory for retrieval on poweron reboot
 *
 * @param reason: enum reason value
 * @param msg: String containing additional information about the reboot reason
 **/
void rebootReasonSet(REBOOT_REASON_e reason, const char *msg);

/**
 * @fn rebootReasonSetPrintf
 *
 * @brief Set the reboot reason in noload memory for retrieval on poweron reboot
 *
 * @param reason: enum reason value
 * @param msg, ... : printf formatted string containing additional information about the reboot reason
 **/
void rebootReasonSetPrintf(REBOOT_REASON_e reason, const char *msg, ...);

/**
 * @fn rebootReasonGet
 *
 * @brief Get the reboot reason values of enum and string. Returns True if the reboot reason flag is set.
 *        else returns false indicating power on reboot
 *
 * @param reason: pointer to enum reason value
 * @param msg : pointer to string to return the stored string message
 * @return True if reason is valid, else false.
 **/
bool rebootReasonGet(REBOOT_REASON_e *reason, const char **msg);

#endif /* LIB_INC_REBOOTREASON_H_ */
