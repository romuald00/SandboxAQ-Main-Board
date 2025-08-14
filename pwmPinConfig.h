/*
 * pwmPinConfig.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 10, 2023
 *      Author: jzhang
 */

#ifndef INC_PWMPINCONFIG_H_
#define INC_PWMPINCONFIG_H_

#include "registerParams.h"
#include "stmTarget.h"

// Enum for all possible PWM pins
typedef enum {
    ADC_PIN = 0,
    TIM_UDP_TX_SIGNAL,
    PWM_SPARE_0,
    TIM_DB_TRIGGER_MSG,
    PWM_SPARE_1,
    TIM_PROFILING,
    LED_RED,
    LED_GREEN,
    DDS_PIN,      // This is a GPIO as the DDS is off when the line is low.
    PWM_PIN_COUNT // must be last
} pwmPort_e;

// Struct for PWM config mapping
typedef struct {
    pwmPort_e port;
    struct {
        struct {
            TIM_HandleTypeDef *htim;
            uint32_t channel;
            uint32_t clockFrequency;
            uint32_t maxArr;
        };
        struct {
            GPIO_TypeDef *gpioPort;
            uint16_t gpioPin;
        };
    };
} pwmConfig_t;

typedef struct {
    registerInfo_t freqInfo;
    registerInfo_t dutyInfo;
    const pwmConfig_t *pwmPinConfig;
} pwmPinConfigState_t;

extern const pwmConfig_t pwmMap[PWM_PIN_COUNT];
extern pwmPinConfigState_t pwmPinStates[PWM_PIN_COUNT];
#endif /* INC_PWMPINCONFIG_H_ */
