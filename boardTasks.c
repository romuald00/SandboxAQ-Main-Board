/*
 * boardTasks.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 31, 2023
 *      Author: rlegault
 */

#include "MB_gatherTask.h"
#include "ctrlSpiCommTask.h"
#include "dbCommTask.h"
#include "dbTriggerTask.h"
#include "ddsTrigTask.h"
#include "pwm.h"
#include "pwmPinConfig.h"
#include "realTimeClock.h"

#define LED_BLINK_FREQ 1.0
#define LED_BLINK_DUTY 50

void initMongoose(int priority, int stackSize);

void initBoardTasks(void) {

    ctrlSpiCommTaskInit(osPriorityHigh, SPI_STACK_WORDS); // 4 threads TX/RX pkts on spi bus
    osDelay(INIT_DELAYS);

    dbCommTaskInit(osPriorityNormal, DB_COMM_STACK_WORDS); // 48 threads, get sensor information
    osDelay(INIT_DELAYS);

    dbTriggerThreadInit(osPriorityHigh, DBTRIGGER_STACK_WORDS); // cause daughter tasks to get next spi packets
    osDelay(INIT_DELAYS);

    ddsTrigThreadInit(osPriorityBelowNormal, DDSTRIGGER_STACK_WORDS);
    osDelay(INIT_DELAYS);

    tcpDataServerWaitTaskInit(osPriorityNormal, TCPDATA_STACK_WORDS);
    osDelay(INIT_DELAYS);

    initMongoose(osPriorityLow, MONGOOSE_STACK_WORDS); // Command and control by user
    osDelay(INIT_DELAYS);

    mbGatherTaskInit(osPriorityRealtime, MBGATHER_STACK_WORDS); // send sensor information to server
    osDelay(INIT_DELAYS);

    pwmStart(LED_GREEN);
    pwmSetFrequency(LED_GREEN, LED_BLINK_FREQ);
    pwmSetDutyCycle(LED_GREEN, LED_BLINK_DUTY);
}
