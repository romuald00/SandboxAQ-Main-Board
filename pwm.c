/*
 * pwm.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 8, 2023
 *      Author: jzhang
 */

#include "pwm.h"
#include "board_registerParams.h"
#include "debugPrint.h"
#include "registerParams.h"
#include "stmTarget.h"

RETURN_CODE adcSetPwmRate(registerInfo_tp info) {
#ifdef STM32F411xE
    // No Adc clock on daughter board
    osStatus result = osErrorOS;
#else
    double freq = info->u.dataUint / 100.0; // get to hz
    osStatus result = pwmSetFrequency(ADC_PIN, freq);
    if (result == osOK) {
        registerWriteForce(info);
    }
#endif
    return result;
}

RETURN_CODE adcSetPwmDuty(registerInfo_tp info) {
#ifdef STM32F411xE
    // No Adc clock on daughter board
    osStatus result = osErrorOS;
#else
    uint32_t duty = info->u.dataUint;
    if (g_adcModeSwitch == ADC_MODE_CONTINUOUS) {
        duty = 100;
    }
    osStatus result = pwmSetDutyCycle(ADC_PIN, duty);
    if (result == osOK) {
        registerWriteForce(info);
    }
#endif
    return result;
}
#ifdef STM32H743xx
RETURN_CODE ddsSetPwmDuty(registerInfo_tp info) {
    uint32_t duty = info->u.dataUint;
    osStatus result = pwmSetDutyCycle(PWM_SPARE_0, duty);
    if (result == osOK) {
        registerWriteForce(info);
    }
    return result;
}

RETURN_CODE ddsSetPwmRate(registerInfo_tp info) {
    double freq = info->u.dataUint / 100.0; // get to hz
    osStatus result = pwmSetFrequency(PWM_SPARE_0, freq);
    if (result == osOK) {
        registerWriteForce(info);
    }
    return result;
}
#endif
osStatus pwmInit() {
    // validate pinStates and pwmMap
    for (int i = 0; i < PWM_PIN_COUNT; i++) {
        if (i == DDS_PIN) {
            continue;
        }
        configASSERT(pwmPinStates[i].pwmPinConfig->port == i);
    }

    // reading the values from storage and then writing the freq and duty registers has the side affect of updating the
    // timers to use those values
    for (int i = 0; i < PWM_PIN_COUNT; i++) {
        if (i == DDS_PIN || i == TIM_PROFILING)
            continue;
        // read register values
        registerRead(&pwmPinStates[i].freqInfo);
        registerRead(&pwmPinStates[i].dutyInfo);

        // write to pwm output (okay because not eeprom)
        registerWrite(&pwmPinStates[i].freqInfo);
        registerWrite(&pwmPinStates[i].dutyInfo);
        pwmStart(i); // start the PWM
    }

    return osOK;
}

osStatus pwmSetDutyCycle(pwmPort_e pwmPort, uint32_t duty) {
    if (duty > 100) {
        return osErrorParameter;
    }

    TIM_HandleTypeDef *htim = pwmMap[pwmPort].htim;

    float duty_percent = duty * 0.01;

    uint32_t pulse = htim->Instance->ARR * duty_percent;

    switch (pwmMap[pwmPort].channel) {
    case TIM_CHANNEL_1:
        htim->Instance->CCR1 = pulse;
        break;
    case TIM_CHANNEL_2:
        htim->Instance->CCR2 = pulse;
        break;
    case TIM_CHANNEL_3:
        htim->Instance->CCR3 = pulse;
        break;
    case TIM_CHANNEL_4:
        htim->Instance->CCR4 = pulse;
        break;
    default:
        break;
    }

    return osOK;
}

osStatus pwmSetFrequency(pwmPort_e pwmPort, double frequency) {

    assert(pwmPort < PWM_PIN_COUNT);
    uint32_t psc = (pwmMap[pwmPort].clockFrequency / frequency) / (pwmMap[pwmPort].maxArr);
    if (psc > pwmMap[pwmPort].maxArr) {
        psc = pwmMap[pwmPort].maxArr;
    }
    uint32_t period = (pwmMap[pwmPort].clockFrequency / (frequency * (psc + 1))) - 1;

    if (period > pwmMap[pwmPort].maxArr) {
        DPRINTF_WARN("Period is above max acceptable period pwmPort %d, freq=%d\n\r", pwmPort, frequency);
        return osErrorParameter;
    }
    if (period < MIN_PERIOD) {
        DPRINTF_WARN("Period is below 20\n\r");
    }
    pwmMap[pwmPort].htim->Instance->PSC = psc;
    pwmMap[pwmPort].htim->Instance->ARR = period;

    return pwmSetDutyCycle(pwmPort, pwmPinStates[pwmPort].dutyInfo.u.dataUint);
}

osStatus pwmStart(pwmPort_e pwmPort) {
    if (HAL_TIM_PWM_Start(pwmMap[pwmPort].htim, pwmMap[pwmPort].channel) != 0) {
        return osErrorOS;
    }
    return osOK;
}

osStatus pwmStop(pwmPort_e pwmPort) {
    if (HAL_TIM_PWM_Stop(pwmMap[pwmPort].htim, pwmMap[pwmPort].channel) != 0) {
        return osErrorOS;
    }
    return osOK;
}
