/*
 * eeprom.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 31, 2023
 *      Author: jzhang
 */

#ifndef APP_INC_EEPROM_CMD_H_
#define APP_INC_EEPROM_CMD_H_

#include "cli/cli.h"

/**
 * @fn testEepromCommand
 *
 * @brief CLI command to test reading/writing to the EEPROM
 *
 * @param[in] CLI *hCli: CLI instance
 * @param[in] int argc: Number or command line arguments
 * @param[in] char *argv[]: List of command line arguments
 *
 * @return CLI_RESULT_SUCCESS on success
 **/
int16_t testEepromCommand(CLI *hCli, int argc, char *argv[]);

/**
 * @fn eepromCommand
 *
 * @brief CLI command to read/write to the EEPROM
 *
 * @param[in] CLI *hCli: CLI instance
 * @param[in] int argc: Number or command line arguments
 * @param[in] char *argv[]: List of command line arguments
 *
 * @return CLI_RESULT_SUCCESS on success
 **/
int16_t eepromCommand(CLI *hCli, int argc, char *argv[]);

#endif /* APP_INC_EEPROM_CMD_H_ */
