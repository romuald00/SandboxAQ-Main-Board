/*
 * pwm.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 8, 2023
 *      Author: jzhang
 */

#ifndef LIB_INC_PWM_H_
#define LIB_INC_PWM_H_

#include "cmsis_os.h"
#include "pwmPinConfig.h"

#define TIMER_MAX_PERIOD 0xFFFF // 16bit timer

#define MIN_PERIOD 20

/**
 * @fn pwmInit
 *
 * @brief Initialize PWM output
 *
 * @return osStatus
 **/
osStatus pwmInit();

/**
 * @fn pwmSetFrequency
 *
 * @brief Set PWM frequency
 *
 * @param[in] pwmPort_e pwmPort: PWM port
 * @param[in] uint32_t frequency: Frequency to set PWM to
 *
 * @return 0 osStatus
 **/
osStatus pwmSetFrequency(pwmPort_e pwmPort, double frequency);

/**
 * @fn pwmSetDutyCycle
 *
 * @brief Set PWM duty cycle
 *
 * @param[in] pwmPort_e pwmPort: PWM port
 * @param[in] uint32_t duty: Duty cycle to set PWM to
 *
 * @return 0 osStatus
 **/
osStatus pwmSetDutyCycle(pwmPort_e pwmPort, uint32_t duty);

/**
 * @fn pwmStart
 *
 * @brief Start PWM output
 *
 * @param[in] pwmPort_e pwmPort: PWM port
 *
 * @return osStatus
 **/
osStatus pwmStart(pwmPort_e pwmPort);

/**
 * @fn pwmStop
 *
 * @brief Stop PWM output
 *
 * @param[in] pwmPort_e pwmPort: PWM port
 *
 * @return osStatus
 **/
osStatus pwmStop(pwmPort_e pwmPort);

#endif /* LIB_INC_PWM_H_ */
