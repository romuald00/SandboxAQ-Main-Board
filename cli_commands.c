/*
 * cli_commands.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 27, 2023
 *      Author: jzhang
 */

#include "cli_commands.h"
#include "cli/cli.h"
#include "cli/cli_print.h"
#include <stdint.h>
#include <string.h>

__attribute__((weak)) int16_t testCommandTest(CLI *hCli, int argc, char *argv[]) {
    CliWriteString(hCli, "In (weak) test command\n\r");

    if (argc > 1) {
        if (argc == 2 && strcmp(argv[1], "subtest") == 0) {
            CliWriteString(hCli, "in (weak) subtest command\n\r");
            CliPrintf(hCli, "args received: %d %s %s\n\r", argc, argv[0], argv[1]);
        }
    }

    return CLI_RESULT_SUCCESS;
}

int16_t helpCommand(CLI *hCli, int argc, char *argv[]) {
    int16_t returnValue = 0;

    if (argc > 1) {
        uint8_t cmdFound = 0;
        for (int i = 0; i < hCli->numCommands; i++) {
            if (strcmp(hCli->commandList[i].name, argv[1]) == 0) {
                CliPrintf(hCli, "%s - %s\n\r", hCli->commandList[i].name, hCli->commandList[i].description);
                CliPrintf(hCli, "%s\n\r", hCli->commandList[i].helpText);
                cmdFound = 1;
                break;
            }
        }
        if (cmdFound == 0) {
            CliPrintf(hCli, "Command %s not found\n\r", argv[1]);
        }
    } else {
        CliWriteString(hCli, "Available commands:\r\n");

        for (int i = 0; i < hCli->numCommands; i++) {
            if (hCli->commandList[i].name != NULL) {
                CliPrintf(hCli, "%s - %s\n\r", hCli->commandList[i].name, hCli->commandList[i].description);
            }
        }
    }

    return returnValue;
}
