/*
 * gpioMB.h
 *
 *  Created on: Oct 15, 2024
 *      Author: rlegault
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 */

#ifndef LIB_INC_GPIOMB_H_
#define LIB_INC_GPIOMB_H_

#include "cli/cli.h"
#include "cmsis_os.h"
#include <stdint.h>

// Enum for setting io type to input or output
typedef enum { GPIO_TYPE_INPUT = 0, GPIO_TYPE_OUTPUT } gpioType_e;

// Used to set/check that an output/input is high/low
typedef enum { GPIO_STATE_LOW = 0, GPIO_STATE_HIGH } gpioState_e;

#define macro_handlerPIN(T)                                                                                            \
    T(DBG_0)                                                                                                           \
    T(DBG_1)                                                                                                           \
    T(DBG_2)                                                                                                           \
    T(DBG_3)                                                                                                           \
    T(DBG_4)                                                                                                           \
    T(DBG_5)                                                                                                           \
    T(DBG_6)                                                                                                           \
    T(DBG_7_DAC)                                                                                                       \
    T(PWR_SW_EN)                                                                                                       \
    T(PWR_SW_DIA_EN)                                                                                                   \
    T(PWR_SW_SEL2)                                                                                                     \
    T(PWR_SW_SEL1)                                                                                                     \
    T(PWR_SW_LATCH)                                                                                                    \
    T(PWR_SW_STATUS)                                                                                                   \
    T(REF_CLK_EN)                                                                                                      \
    T(SPARE1)

GENERATE_ENUM_LIST(macro_handlerPIN, gpioMBPin_e)
#ifdef CREATE_GPIOMB_STRING_LOOKUP_TABLE
GENERATE_ENUM_STRING_NAMES(macro_handlerPIN, gpioMBPin_e)
#else
extern const char *gpioMBPin_e_Strings[];
#endif

// Enum for all possible ports
typedef enum {
    GPIO_PORT_A_ENUM = 0,
    GPIO_PORT_B_ENUM,
    GPIO_PORT_C_ENUM,
    GPIO_PORT_D_ENUM,
    GPIO_PORT_E_ENUM,
    GPIO_PORT_F_ENUM,
    GPIO_PORT_G_ENUM,
    GPIO_PORT_H_ENUM,
    GPIO_PORT_MAX
} gpioPort_e;

/**
 * @fn gpioGetInput
 *
 * @brief Write GPIO output
 *
 * @param[in] DBG_PINS_e gpioPin: GPIO pin enum
 * @param[in] gpioState_t pinState: GPIO output state to set
 *
 * @return osStatus
 **/
osStatus gpioSetOutput(gpioMBPin_e gpioPin, gpioState_e pinState);

/**
 * @fn gpioGetInput
 *
 * @brief Read GPIO input
 *
 * @param[in] DBG_PINS_e gpioPin: GPIO pin enum
 * @param[in] gpioState_t *pinValue: GPIO input state
 *
 * @return osStatus
 **/
osStatus gpioGetInput(gpioMBPin_e gpioPin, gpioState_e *pinValue);

/**
 * @fn gpioToggle
 *
 * @brief Toggle GPIO output
 *
 * @param[in] gpioPin_t gpioPin: GPIO pin enum
 *
 * @return osStatus
 **/
osStatus gpioToggle(gpioMBPin_e gpioPin);

#endif /* LIB_INC_GPIOMB_H_ */
