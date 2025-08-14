/*
 * resetBoard.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 20, 2023
 *      Author: jzhang
 */

#ifndef LIB_INC_RESETBOARD_H_
#define LIB_INC_RESETBOARD_H_

#include "cmsis_os.h"

/**
 * @fn resetTaskInit
 *
 * @brief initialize the board reset Task.
 *
 * @param[in]     priority thread priority
 * @param[in]     stackSz  thread stack size
 **/

void resetTaskInit(int priority, int stackSize);
/**
 * @fn startResetTask
 *
 * @brief Task for checking reset register
 **/
void startResetTask();

#endif /* LIB_INC_RESETBOARD_H_ */
