/*
 * mbLib.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 18, 2023
 *      Author: jzhang
 */

#include "fanCtrl.h"
#include "largeBuffer.h"
#include "pwm.h"

void initBoardLib(void) {
    largeBufferInit();
    pwmInit();
    fanCtrlInit();
}
