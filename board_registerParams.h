/*
 * board_registerParams.h
 *  Main board registers
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 5, 2023
 *      Author: rlegault
 */

#ifndef MB_LIB_INC_BOARD_REGISTERPARAMS_H_
#define MB_LIB_INC_BOARD_REGISTERPARAMS_H_

typedef struct registerInfo *registerInfo_tp;

#include "registerParams.h"
#include "saqTarget.h"
RETURN_CODE streamIntervalWrite(const registerInfo_tp regInfo);
RETURN_CODE dbSpiInterval(const registerInfo_tp regInfo);

RETURN_CODE noWriteFn(const registerInfo_tp regInfo);

#endif /* LIB_INC_BOARD_REGISTERPARAMS_H_ */
