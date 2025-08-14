/*
 * cli_commands.h
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 27, 2023
 *      Author: jzhang
 */

#ifndef SAQ01_FW_COMMON_APP_INC_CLI_COMMANDS_H_
#define SAQ01_FW_COMMON_APP_INC_CLI_COMMANDS_H_

#include "cli/cli.h"
#include "cli_commands_board.h"
#include "cmdAndCtrl.h"
#include "registerParams.h"
#include <eeprom_cmd.h>

#define NUM_COMMANDS (10 + NUM_BOARD_CMDS)

/**
 * @fn nameCommand
 *
 * @brief CLI command to get the current board
 *
 * @param[in] CLI *hCli: CLI instance
 * @param[in] int argc: Number or command line arguments
 * @param[in] char *argv[]: List of command line arguments
 *
 * @return CLI_RESULT_SUCCESS on success
 **/
int16_t nameCommand(CLI *hCli, int argc, char *argv[]);

/**
 * @fn testCommand
 *
 * @brief CLI command for testing purposes
 *
 * @param[in] CLI *hCli: CLI instance
 * @param[in] int argc: Number or command line arguments
 * @param[in] char *argv[]: List of command line arguments
 *
 * @return CLI_RESULT_SUCCESS on success
 **/
int16_t testCommand(CLI *hCli, int argc, char *argv[]);

/**
 * @fn helpCommand
 *
 * @brief CLI command that lists all available commands
 *
 * @param[in] CLI *hCli: CLI instance
 * @param[in] int argc: Number or command line arguments
 * @param[in] char *argv[]: List of command line arguments
 *
 * @return CLI_RESULT_SUCCESS on success
 **/
int16_t helpCommand(CLI *hCli, int argc, char *argv[]);

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
 * @brief CLI command read/write to the EEPROM
 *
 * @param[in] CLI *hCli: CLI instance
 * @param[in] int argc: Number or command line arguments
 * @param[in] char *argv[]: List of command line arguments
 *
 * @return CLI_RESULT_SUCCESS on success
 **/
int16_t eepromCommand(CLI *hCli, int argc, char *argv[]);

#if 0
#define XSTR(x) STR(x)
#define STR(x) #x

#pragma message "NUM_COMMANDS=" XSTR(NUM_COMMANDS)
#pragma message "NUM_BOARD_CMDS=" XSTR(NUM_BOARD_CMDS)
#endif

int16_t cliHandler_raiseIssue(CLI *hCli, int argc, char *argv[]);

// clang-off
static const CLI_COMMAND cliCommandList[NUM_COMMANDS] = {
    {"name", "Get the current board (MB or DB)", NULL, nameCommand},
    {"test",
     "Diagnostics and self test",
     "\twdog - display watchdog high water marks\r\n"
     "\ttaskstats - Get task statistics\r\n"
     "\tstacks - Query task stacks high watermark in remaining bytes\r\n"
     "\tticks - Get current number of milliseconds since reset\r\n"
     "\tdumplog - Dump SDRAM log\r\n"
     "\tfillramlog - Fills RAM log with data\r\n"
#ifndef STM32F411xE
     "\tsensorlog <slot> - Print sensor board log\r\n"
     "\tclearsensorlog <slot> - clear the sensor board debug log\r\n"
     "\tlwipstats - Get LwIP statistics\r\n"
     "\tiperf start - Start iperf v2 server\r\n"
     "\tdumprxbuf <count> - Print count bytes from the large rx buffer\r\n"
#else
     "\tdumptxbuf <count> - Print count bytes from the large tx buffer\r\n"
#endif
     "\tclearramlog - Erases RAM log\r\n"
     "\tramloglength - Returns length of the current RAM log\r\n"
     "\tassert - Triggers an assert\r\n"
     "\tstopwdt - Stops the watchdog on the current task\r\n"
     "\theap - Display Free Heap value\r\n"
     "\tcorrectEmptyTo7 <num> - change the num to 7 for all occurrences\r\n\t\tin register SENSOR_BOARD_0...23\r\n",
     testCommand},
    {"test_eeprom", "Test eeprom", NULL, testEepromCommand},
    {"eeprom",
     "Read/write eeprom registers",
     "\tlist List all the eeprom registers\n\r"
     "\tread <register> Read register from eeprom\n\r"
     "\twrite <register> <data> Write data to eeprom register"
     "\taddrRead <addr> <bytes> Read number of bytes starting at <addr> from eeprom",
     eepromCommand},
    {"writereg",
     "Write register value",
     "Usage: writereg <register> <value>"
     "\tRegister value types will be determined automatically",
     cliHandler_writereg},
    {"readreg", "Read register value", "Usage: readreg <register>", cliHandler_readreg},
    {"listreg", "List available register names", "Usage: listreg", cliHandler_listreg},
    {"system_status", "Query any system errors", "Usage: syst_status", cliHandler_raiseIssue},
    BOARD_CMDS{"help", "List all available commands.", NULL, helpCommand}};
// clang-on
#endif /* SAQ01_FW_COMMON_APP_INC_CLI_COMMANDS_H_ */
