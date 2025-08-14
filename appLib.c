/*
 * appLib.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 21, 2023
 *      Author: jzhang
 */

#include "appLib.h"

#include "eeprom.h"
#include "registerParams.h"
#include "stmTarget.h"

__weak void initBoardLib(void) {
    // pass
}
void initCommonLib(void) {
    registerInit();

    eepromInit();

    registerInfo_t regInfo;
#ifdef STM32H743xx
    regInfo.mbId = ADC_MODE;
#else
    regInfo.sbId = SB_ADC_MODE;
#endif
    regInfo.type = DATA_UINT;
    registerRead(&regInfo);
    if (regInfo.u.dataUint >= ADC_MODE_MAX) {
        regInfo.u.dataUint = ADC_MODE_SINGLE;
        registerWrite(&regInfo);
    }
    // NOTE that mode must be the same for all daughter cards and Main board
    g_adcModeSwitch = regInfo.u.dataUint;

    initBoardLib();
}
