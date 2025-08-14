/*
 * pwmPinConfig.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 10, 2023
 *      Author: jzhang
 */

#include "pwmPinConfig.h"
#include "cmsis_os.h"
#include "main.h"
#include "stmTarget.h"

#ifdef STM32F767xx
#error "STM32F767 NO LONGER SUPPORTED"
#define ADC_PIN_TIMER htim9 /* PWM output PE5 - Start of conversion ADC*/
#define SPARE_1_TIMER htim10
#elif defined(STM32H743xx) && defined(SAQ_GEN1)
#error "GEN1 NO LONGER SUPPORTED"
#define ADC_PIN_TIMER htim15 /* PWM output PE5 - Start of conversion ADC*/
#define SPARE_1_TIMER htim16
#elif defined(STM32H743xx) && defined(SAQ_GEN2)
#define ADC_PIN_TIMER htim2 /* PWM output PE5 - Start of conversion ADC*/
#define ADC_PWM_CHAN TIM_CHANNEL_1

#define SPARE_0_TIMER htim3
#define SPARE_0_CHAN TIM_CHANNEL_1

#define SPARE_1_TIMER htim4
#define SPARE_1_CHAN TIM_CHANNEL_1

#define DDS_TRIGGER_TIMER htim5
#define DDS_TRIGGER_CHAN TIM_CHANNEL_4

#define LED_GREEN_TIMER htim8
#define LED_GREEN_CHAN TIM_CHANNEL_2

#define LED_RED_TIMER htim13
#define LED_RED_CHAN TIM_CHANNEL_1

#define ETH_SEND_MSG_TIMER htim15
#define ETH_SEND_MSG_CHAN TIM_CHANNEL_1

#define PROFILE_TIMER htim16
#define PROFILE_TIM_CHAN TIM_CHANNEL_1

#define WAKE_SENSOR_TIMER htim17
#define WAKE_SENSOR_CHAN TIM_CHANNEL_1

#endif

extern TIM_HandleTypeDef ADC_PIN_TIMER;
extern TIM_HandleTypeDef SPARE_0_TIMER;
extern TIM_HandleTypeDef SPARE_1_TIMER;
extern TIM_HandleTypeDef LED_GREEN_TIMER;
extern TIM_HandleTypeDef LED_RED_TIMER;
extern TIM_HandleTypeDef ETH_SEND_MSG_TIMER;
extern TIM_HandleTypeDef PROFILE_TIMER;
extern TIM_HandleTypeDef WAKE_SENSOR_TIMER;
extern TIM_HandleTypeDef DDS_TRIGGER_TIMER; // Unused, it is a GPIO

extern osThreadId mbGatherTaskHandle;

/* STM32F767 */
// TIM2-7,12-14 APB1
// TIM1,8-11 APB2
// htim1:Ch1 PE9 SPARE_0
// htim5: No Output - Trigger next daughter board message
// htim9:ch1 PE5 Start Conversion of ADC
// htim10: CH1 PF6 SPARE_1
// htim12: freeRtos Profiling
// htim13: PWM - No Output - Send UDP Pkt
// htim14: freeRtos Systick
// Unused: TIM2,3,4,6,7,11

/* STM32H743 GEN1*/
// TIM2-5,12-14 APB1
// TIM1,8, 15-17 APB2
// htim1:Ch1 PE9 PWM SPARE_0
// htim5: No Output - Trigger next daughter board message - BASE ITR
// htim15:ch1 - ADC Sync
// Tim2,5 have 32bit arr values.
#define TIM_ARR_16 0xFFFF
#define TIM_ARR_32 0xFFFFFFFF

const pwmConfig_t pwmMap[PWM_PIN_COUNT] = {
    [ADC_PIN] = {.port = ADC_PIN,
                 .htim = &ADC_PIN_TIMER,
                 .channel = ADC_PWM_CHAN,
                 .clockFrequency = APB1_TIM_CLOCK,
                 .maxArr = TIM_ARR_32,
                 .gpioPort = GPIOA,
                 .gpioPin = GPIO_PIN_5},

    [TIM_UDP_TX_SIGNAL] = {.port = TIM_UDP_TX_SIGNAL,
                           .htim = &ETH_SEND_MSG_TIMER,
                           .channel = ETH_SEND_MSG_CHAN,
                           .clockFrequency = APB2_TIM_CLOCK,
                           .maxArr = TIM_ARR_16,
                           .gpioPort = GPIOE,
                           .gpioPin = GPIO_PIN_4},

    [PWM_SPARE_0] = {.port = PWM_SPARE_0,
                     .htim = &SPARE_0_TIMER,
                     .channel = SPARE_0_CHAN,
                     .clockFrequency = APB1_TIM_CLOCK,
                     .maxArr = TIM_ARR_16,
                     .gpioPort = GPIOC,
                     .gpioPin = GPIO_PIN_6},

    [TIM_DB_TRIGGER_MSG] = {.port = TIM_DB_TRIGGER_MSG,
                            .htim = &WAKE_SENSOR_TIMER,
                            .channel = WAKE_SENSOR_CHAN,
                            .clockFrequency = APB2_TIM_CLOCK,
                            .maxArr = TIM_ARR_16,
                            .gpioPort = GPIOF,
                            .gpioPin = GPIO_PIN_7},

    [PWM_SPARE_1] = {.port = PWM_SPARE_1,
                     .htim = &SPARE_1_TIMER,
                     .channel = SPARE_1_CHAN,
                     .clockFrequency = APB1_TIM_CLOCK,
                     .maxArr = TIM_ARR_16,
                     .gpioPort = GPIOD,
                     .gpioPin = GPIO_PIN_12},

    [TIM_PROFILING] = {.port = TIM_PROFILING,
                       .htim = &PROFILE_TIMER,
                       .channel = PROFILE_TIM_CHAN,
                       .clockFrequency = APB1_TIM_CLOCK,
                       .maxArr = TIM_ARR_16,
                       .gpioPort = GPIOF,
                       .gpioPin = GPIO_PIN_6},

    [LED_RED] = {.port = LED_RED,
                 .htim = &LED_RED_TIMER,
                 .channel = LED_RED_CHAN,
                 .clockFrequency = APB1_TIM_CLOCK,
                 .maxArr = TIM_ARR_16,
                 .gpioPort = GPIOF,
                 .gpioPin = GPIO_PIN_8},

    [LED_GREEN] = {.port = LED_GREEN,
                   .htim = &LED_GREEN_TIMER,
                   .channel = LED_GREEN_CHAN,
                   .clockFrequency = APB2_TIM_CLOCK,
                   .maxArr = TIM_ARR_16,
                   .gpioPort = GPIOC,
                   .gpioPin = GPIO_PIN_7},

    // The DDS trigger syncs the start of the wave generation and disables it.
    // When the pin is high Wave generation is off.
    // The act of thepin going low is then sync and enable event
    [DDS_PIN] = {.htim = &DDS_TRIGGER_TIMER,
                 .channel = TIM_CHANNEL_4,
                 .port = DDS_PIN,
                 .gpioPort = GPIOA,
                 .gpioPin = GPIO_PIN_3},
};

pwmPinConfigState_t pwmPinStates[PWM_PIN_COUNT] = {
    {.freqInfo = {.mbId = ADC_READ_RATE, .type = DATA_UINT},
     .dutyInfo = {.mbId = ADC_READ_DUTY, .type = DATA_UINT},
     .pwmPinConfig = &pwmMap[ADC_PIN]},
    {.pwmPinConfig = &pwmMap[TIM_UDP_TX_SIGNAL]},
    {.freqInfo = {.mbId = DDS_CLK_RATE, .type = DATA_UINT},
     .dutyInfo = {.mbId = DDS_CLK_DUTY, .type = DATA_UINT},
     .pwmPinConfig = &pwmMap[PWM_SPARE_0]},
    {.pwmPinConfig = &pwmMap[TIM_DB_TRIGGER_MSG]},
    {.pwmPinConfig = &pwmMap[PWM_SPARE_1]},
    {.pwmPinConfig = &pwmMap[TIM_PROFILING]},
    {.pwmPinConfig = &pwmMap[LED_RED]},
    {.pwmPinConfig = &pwmMap[LED_GREEN]},
    {.pwmPinConfig = &pwmMap[DDS_PIN]},
};
