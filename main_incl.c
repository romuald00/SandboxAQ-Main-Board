/*
 * main_incl.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 20, 2023
 *      Author: rlegault
 */

#include "applib.h"
#include "cmsis_os.h"
#include "debugPrint.h"
#include "pwmPinConfig.h"
#include "registerParams.h"
#include "rtosTasks.h"
#include "saqTarget.h"
#include "taskWatchdog.h"
#ifdef STM32F411xE
#include "workerSpiCommTask.h"
#else
#include "ctrlSpiCommTask.h"
#endif
#include <stdint.h>
#include <sys/time.h>

#define FIRST_NET_INTERFACE 1

#define SPI_DIVIDER SPI_BAUDRATEPRESCALER_16
#define SPI_CLK_POLARITY SPI_POLARITY_LOW

#define TICKS_IN_ONE_SEC 1000

volatile uint32_t profilingCounter;
unsigned long getRunTimeCounterValue(void);
void Error_Handler(void);

uint32_t getRunTimeCounterValue(void) {
    return profilingCounter;
}
void configureTimerForRunTimeStats(void);
void configureTimerForRunTimeStats(void) {

#ifdef STM32F411xE
    // too many interrupts on F411 processor, increment profiling with Sys tick
#else
    HAL_TIM_Base_Start_IT(pwmMap[TIM_PROFILING].htim);
#endif
}

void __assert_func(const char *file, int line, const char *func, const char *failedexpr) {
    DPRINTF_NOLOCK_ARG("ASSERT: %s at %d\r\n (%s)", file, line, failedexpr);
    rebootReasonSetPrintf(REBOOT_ASSERT, "%s at %d\r\n (%s)", file, line, failedexpr);
    Error_Handler(); // does not return from this
    while (1)
        ;
}

extern osSemaphoreId txNewDataId;
extern SPI_HandleTypeDef hspi2;
// RESET the worker SPI bus if the dma becomes unsynced
#ifdef STM32F411xE
void RESET_SPI(SPI_HandleTypeDef *hspi) {
    HAL_SPI_DMAStop(hspi);
    __HAL_RCC_SPI2_FORCE_RESET();
    __HAL_RCC_SPI2_RELEASE_RESET();
    if (hspi == &hspi1) {
        MX_SPI1_Init();
    }
}
#endif
void debugPrintInit(void) {

#if DEBUG_PRINTING == 1
    debugPrintingInit();
    ramLogInit();
#endif
    osDelay(100);
}
void userCode5_StartDefaultTask() {
    initCommonLib();
    osDelay(100);
    uint32_t uptime = 0;
    initRtosTasks();
#ifdef STM32H743xx
    registerInfo_t uptimeInfo = {.mbId = UPTIME_SEC, .type = DATA_UINT};
#else
    registerInfo_t uptimeInfo = {.sbId = SB_UPTIME_SEC, .type = DATA_UINT};
#endif
    /* Infinite loop */
    for (;;) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_0);
        osDelay(1000);
        uptime++;
        uptimeInfo.u.dataUint = uptime;
        registerWriteForce(&uptimeInfo);
        updateSpiStats();
    }
}
#ifndef STM32F411xE
int _gettimeofday(struct timeval *tv, void *tzvp) {
    RTC_TimeTypeDef gTime;
    HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN);
    tv->tv_sec = gTime.Seconds + gTime.Minutes * 60 + gTime.Hours * 3600;
    tv->tv_usec = gTime.SubSeconds;
    return 0;
}
#endif
