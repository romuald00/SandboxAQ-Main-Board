/*
 * rtosTasks.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 27, 2023
 *      Author: rlegault
 */

#ifndef COMMON_TASKS_INC_RTOSTASKS_H_
#define COMMON_TASKS_INC_RTOSTASKS_H_

void initRtosTasks(void);  // these tasks run on all boards
void initBoardTasks(void); // these are tasks that only run on a particular board

#endif /* TASKS_INC_RTOSTASKS_H_ */
