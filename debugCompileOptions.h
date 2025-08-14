/*
 * DebugCompileOptions.h
 *
 *  Copyright Nuvation Research Corporation 2018-2025. All Rights Reserved.
 *
 *  Created on: Jun 3, 2025
 *      Author: rlegault
 */

#ifndef INC_DEBUGCOMPILEOPTIONS_H_
#define INC_DEBUGCOMPILEOPTIONS_H_

#ifndef TRACEALYZER
#define TRACEALYZER 0
#endif

#if TRACEALYZER

#define INIT_DELAYS 5

#if DEBUG_ONE_BOARD_COMPILER_FLAG

#define DEBUG_SENSOR_BOARD 5
#define DEBUG_SENSOR_BOARD_CNT 1



#define MULTIPLER 1

#define DB_TASK_START_ID DEBUG_SENSOR_BOARD
#define MAX_DB_TASKS_END_ID (DEBUG_SENSOR_BOARD+DEBUG_SENSOR_BOARD_CNT)

#else

#define MULTIPLER 1

#define DB_TASK_START_ID 0
#define MAX_DB_TASKS_END_ID (MAX_CS_ID)

#endif
#define TRACEALYZER_EN 1
#else

#define TRACEALYZER_EN 0

#define INIT_DELAYS 0
#define xTracePrintF(...)
#define MULTIPLER 1
#define DB_TASK_START_ID 0
#define MAX_DB_TASKS_END_ID MAX_CS_ID // non inclusive TODO set to MAX_CS_ID

#endif

#ifdef DEBUG
#define OPT_FAST 0
#else
#define OPT_FAST 1
#endif


#endif /* INC_DEBUGCOMPILEOPTIONS_H_ */
