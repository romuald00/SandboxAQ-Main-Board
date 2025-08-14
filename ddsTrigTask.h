/*
 * ddsTrigTask.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Feb 23, 2024
 *      Author: rlegault
 */

#ifndef SAQ01_FW_MB_APP_INC_DDSTRIGTASK_H_
#define SAQ01_FW_MB_APP_INC_DDSTRIGTASK_H_

#include "cli/cli.h"
#include <stdint.h>

/**
 * @brief Stop and Start the DDS trigger to sync all the DDS outputs
 */
void ddsTriggerSync(void);

/**
 * @brief Start the DDS trigger, causing all DDS to output the waveform
 */
void ddsTriggerEnable(void);

/**
 * @brief Stop the DDS trigger, causing all DDS to stop output of the waveform
 */
void ddsTriggerDisable(void);

/**
 * @brief Init the dds trigger task
 * @param[in]     priority thread priority
 * @param[in]     stackSz  thread stack size
 */
void ddsTrigThreadInit(int priority, int stackSize);

/**
 * @brief DDS trigger Cli command handler
 * @param hCli UART output
 * @param argc number of CLI args in argv
 * @param argv CLI args
 * @return CLI_RESULT_SUCCESS if successful else error code
 */
int16_t ddsTrigCliCmd(CLI *hCli, int argc, char *argv[]);

/**
 * @brief DDS Clock cli command handler
 * @param hCli UART output
 * @param argc number of CLI args in argv
 * @param argv CLI args
 * @return CLI_RESULT_SUCCESS if successful else error code
 */
int16_t ddsClockCliCmd(CLI *hCli, int argc, char *argv[]);

/**
 * @brief Disable the MCG dds wave form generation
 **/
void ddsWaveDisable(void);

/**
 * @brief Disable the coil driver dds wave form generation
 **/
void coilDDSWaveDisable(void);

#endif /* SAQ01_FW_MB_APP_INC_DDSTRIGTASK_H_ */
