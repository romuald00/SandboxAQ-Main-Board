/*
 * appLib.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 21, 2023
 *      Author: jzhang
 */

#ifndef APP_COMMON_INC_APP_H_
#define APP_COMMON_INC_APP_H_

/**
 * @fn initCommonLib
 *
 * @brief Initialize libraries used on all boards
 *
 **/
void initCommonLib(void);

/**
 * @fn initBoardLib
 *
 * @brief Initialize libraries used on a particular board
 *
 **/
void initBoardLib(void);

#endif /* APP_INC_APP_H_ */
