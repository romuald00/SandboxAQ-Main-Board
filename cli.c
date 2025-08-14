/**
 * @file
 * Command Line Interface Library
 *
 * These functions provide the framework for a command line interface parser.
 *
 * Copyright Nuvation Research Corporation 2006-2018. All Rights Reserved.
 * www.nuvation.com
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "asserts.h"
#include "generic_printf.h"
#include "ringbuffer.h"

#include "cli/cli.h"
#include "cli/cli_uart.h"

#define CLI_CARRIAGE_RETURN_CHARACTER '\r'
#define CLI_NEWLINE_CHARACTER '\n'
#define CLI_BACKSPACE_CHARACTER '\b'
#define CLI_ESC_CHARACTER 27
#define CLI_LEFT_BRACKET_CHARACTER 91
#define CLI_UP_ARROW_CHARACTER 65
#define CLI_DOWN_ARROW_CHARACTER 66
#define CLI_DELETE_CHARACTER 127
#define CLI_EOT_CHARACTER 0x03

/**
 *  Looks up the requested command in the CLI's list of registered commands.
 *
 *  Searches the list of command definitions registered with the CLI for the
 *  requested command and returns a pointer to its definition if found or
 *  NULL if it was not found.
 *
 *  The name comparison is case insensitive.
 *
 *  @param[in] hCli
 *      A pointer to the Command Line Interface (CLI) structure.
 *
 *  @param[in] name
 *      The name of the command to be searched for.
 *
 *  @return
 *      A pointer to the CLI_COMMAND definition structure for the requested
 *      command or NULL if the command was not found in the list of registered
 *      commands with the given CLI.
 */
static const CLI_COMMAND *CliLookupCommand(const CLI *hCli, const char *name);
static const char backspace[] = "\b \b";

void CliInit(CLI *hCli,
             int channel,
             int numCommands,
             const CLI_COMMAND *commandList,
             const char *prompt,
             uint_least8_t *_inputBuffer,
             uint_least8_t *_historyBuffers,
             CommandReadyCallback callback,
             InitMessageGenerator initMsgs) {
    hCli->cliUart = channel;
    hCli->numCommands = numCommands;
    hCli->commandList = commandList;
    strlcpy(hCli->prompt, prompt, 8);
    hCli->inputBuffer = _inputBuffer;
    hCli->historyBuffers = _historyBuffers;
    hCli->inputBufferIndex = 0;
    hCli->lastViewedHistoryBuffer = 0;
    hCli->upLevels = 0;
    for (uint8_t i = 0; i < CLI_MAX_COMMAND_HISTORY; i++) {
        hCli->historyBufferIndexes[i] = 0;
    }
    hCli->currentHistoryBuffer = 0;
    hCli->maxHistoryBuffer = -1;
    CLI_CLEAR_BOOL(hCli, COMMAND_IS_EXECUTING);
    CLI_CLEAR_BOOL(hCli, ECHO_IS_SUPPRESSED);
    hCli->commandReadyCallback = callback;

    if (initMsgs != NULL) {
        initMsgs();
    }

    CliWriteString(hCli, "<CLI INITIALIZED>\r\n");
    CliWriteString(hCli, hCli->prompt);
}

void CliCharacterReceivedCallback(CLI *hCli, uint_least8_t ch) {
    if (CLI_CHECK_BOOL(hCli, COMMAND_IS_EXECUTING)) {
        SmallRbEnqueue(&hCli->stdinBuffer.ringBuffer, ch);
        return;
    }

    switch (ch) {
    case CLI_NEWLINE_CHARACTER:
        break;
    case CLI_CARRIAGE_RETURN_CHARACTER:
        hCli->inputBuffer[hCli->inputBufferIndex] = '\0';

        if (CLI_CHECK_BOOL(hCli, ECHO_IS_SUPPRESSED) && hCli->inputBufferIndex > 0) {
            static const char confirmation[] = "CMD\r\n";
            CliWriteOnUart(hCli->cliUart, confirmation, strlen(confirmation));
        } else if (!CLI_CHECK_BOOL(hCli, ECHO_IS_SUPPRESSED)) {
            CliWriteString(hCli, "\r\n");
        }

        CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_1);
        CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_2);

        if (hCli->inputBufferIndex > 0) {
            if (hCli->historyBuffers != NULL) {
                hCli->historyBufferIndexes[hCli->currentHistoryBuffer] = hCli->inputBufferIndex;
                strncpy((char *)&hCli->historyBuffers[hCli->currentHistoryBuffer * MIN_CLI_BUFFER_SIZE],
                        (char *)hCli->inputBuffer,
                        MIN_CLI_BUFFER_SIZE);
            }

            if (hCli->commandReadyCallback) {
                SmallRbInit(&hCli->stdinBuffer.ringBuffer, sizeof(hCli->stdinBuffer._buffer));
                CLI_SET_BOOL(hCli, COMMAND_IS_EXECUTING);
                hCli->commandReadyCallback(hCli);
            }

            if (hCli->historyBuffers != NULL) {
                if (hCli->currentHistoryBuffer > hCli->maxHistoryBuffer) {
                    hCli->maxHistoryBuffer = hCli->currentHistoryBuffer;
                }
                hCli->currentHistoryBuffer =
                    ((hCli->currentHistoryBuffer + 1) % CLI_MAX_COMMAND_HISTORY + CLI_MAX_COMMAND_HISTORY) %
                    CLI_MAX_COMMAND_HISTORY;
                hCli->lastViewedHistoryBuffer = hCli->currentHistoryBuffer;
                hCli->upLevels = 0;
            }
            hCli->inputBufferIndex = 0;
        } else {
            hCli->lastViewedHistoryBuffer = hCli->currentHistoryBuffer;
            hCli->upLevels = 0;
            CliWriteString(hCli, hCli->prompt);
        }
        break;
    case CLI_DELETE_CHARACTER:
    case CLI_BACKSPACE_CHARACTER:
        CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_1);
        CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_2);
        /* Handle a backspace properly. */
        if (hCli->inputBufferIndex > 0) {
            hCli->inputBufferIndex--;
            CliWriteOnUart(hCli->cliUart, backspace, strlen(backspace));
        }
        break;
    case CLI_EOT_CHARACTER:
        hCli->inputBufferIndex = 0;
        CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_1);
        CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_2);
        hCli->lastViewedHistoryBuffer = hCli->currentHistoryBuffer;
        hCli->upLevels = 0;
        CliWriteString(hCli, "\r\n");
        CliWriteString(hCli, hCli->prompt);
        break;
    case CLI_ESC_CHARACTER:
        CLI_SET_BOOL(hCli, INTERPRETING_ESC_STAGE_1);
        break;
    case CLI_LEFT_BRACKET_CHARACTER:
        if (CLI_CHECK_BOOL(hCli, INTERPRETING_ESC_STAGE_1)) {
            CLI_SET_BOOL(hCli, INTERPRETING_ESC_STAGE_2);
            break;
        } else {
            CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_1);
        }
        /* intentional fall-through */
    default:
        if (hCli->inputBufferIndex < MIN_CLI_BUFFER_SIZE - 1) {
            if (CLI_CHECK_BOOL(hCli, INTERPRETING_ESC_STAGE_2)) {
                if (ch == CLI_UP_ARROW_CHARACTER || ch == CLI_DOWN_ARROW_CHARACTER) {
                    CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_1);
                    CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_2);

                    if (hCli->historyBuffers != NULL) {
                        if (ch == CLI_DOWN_ARROW_CHARACTER && hCli->upLevels == 0) {
                            hCli->inputBufferIndex = 0;
                            return;
                        }

                        if (ch == CLI_UP_ARROW_CHARACTER && hCli->upLevels > hCli->maxHistoryBuffer) {
                            return;
                        }

                        for (uint16_t i = hCli->inputBufferIndex; i > 0; i--) {
                            CliWriteOnUart(hCli->cliUart, backspace, strlen(backspace));
                        }

                        if (ch == CLI_UP_ARROW_CHARACTER) {
                            hCli->lastViewedHistoryBuffer =
                                ((hCli->lastViewedHistoryBuffer - 1) % CLI_MAX_COMMAND_HISTORY +
                                 CLI_MAX_COMMAND_HISTORY) %
                                CLI_MAX_COMMAND_HISTORY;
                            hCli->upLevels++;
                        } else if (ch == CLI_DOWN_ARROW_CHARACTER) {
                            hCli->lastViewedHistoryBuffer =
                                ((hCli->lastViewedHistoryBuffer + 1) % CLI_MAX_COMMAND_HISTORY +
                                 CLI_MAX_COMMAND_HISTORY) %
                                CLI_MAX_COMMAND_HISTORY;
                            hCli->upLevels--;

                            if (hCli->upLevels == 0) {
                                hCli->inputBufferIndex = 0;
                                return;
                            }
                        }

                        if (hCli->lastViewedHistoryBuffer > hCli->maxHistoryBuffer) {
                            hCli->lastViewedHistoryBuffer = hCli->maxHistoryBuffer;
                        }

                        if (hCli->historyBufferIndexes[hCli->lastViewedHistoryBuffer] > 0) {
                            strncpy(
                                (char *)hCli->inputBuffer,
                                (char *)&hCli->historyBuffers[(hCli->lastViewedHistoryBuffer) * MIN_CLI_BUFFER_SIZE],
                                MIN_CLI_BUFFER_SIZE);
                            CliWriteString(hCli, (char *)hCli->inputBuffer);
                            hCli->inputBufferIndex = hCli->historyBufferIndexes[hCli->lastViewedHistoryBuffer];
                        } else {
                            hCli->inputBufferIndex = 0;
                        }
                    }
                }
            } else {
                CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_1);
                CLI_CLEAR_BOOL(hCli, INTERPRETING_ESC_STAGE_2);

                /* While there's room in the buffer, fill it with characters.
                 * Do nothing unless the echo is suppressed. */
                if (!CLI_CHECK_BOOL(hCli, ECHO_IS_SUPPRESSED))
                    CliWriteByteOnUart(hCli->cliUart, (char)ch);

                hCli->inputBuffer[hCli->inputBufferIndex++] = (char)ch;
            }
        }
        break;
    }
}

void CliGetReadyForNewCommand(CLI *hCli) {
    CLI_CLEAR_BOOL(hCli, COMMAND_IS_EXECUTING);
    CliWriteString(hCli, hCli->prompt);
}

CLI_RESULT CliGetNewCommand(const CLI *hCli, const CLI_COMMAND **cmd, int *argc, char *argv[]) {
    CLI_RESULT result;

    ASSERT_TRUE(hCli != NULL);
    ASSERT_TRUE(cmd != NULL);
    ASSERT_TRUE(argc != NULL);
    ASSERT_TRUE(argv != NULL);

    result = CliParse((char *)hCli->inputBuffer, argc, argv);
    *cmd = NULL;

    if (result == CLI_RESULT_SUCCESS && *argc > 0) {
        *cmd = CliLookupCommand(hCli, argv[0]);

        if (*cmd == NULL) {
            return CLI_RESULT_UNRECOGNIZED_COMMAND;
        }
    }

    return result;
}

int CliReadChar(CLI *hCli) {
    uint8_t byte;
    int result = CLI_RESULT_NO_DATA;

    if (SmallRbRead(&hCli->stdinBuffer.ringBuffer, &byte, 1)) {
        result = (int)(byte);
    }

    return result;
}

void CliWriteChar(CLI *hCli, int ch) {
    CliWriteByteOnUart(hCli->cliUart, (uint8_t)ch);
}

void CliWriteString(CLI *hCli, const char *string) {
    CliWriteOnUart(hCli->cliUart, (const uint8_t *)string, strlen(string));
}

CLI_RESULT CliParse(char *commandBuffer, int *argc, char *argv[]) {
    uint8_t inQuotes;
    char *srcPtr;
    char *destPtr;

    *argc = 0;
    srcPtr = commandBuffer;
    destPtr = commandBuffer;
    inQuotes = 0;

STATE_ADVANCE:
    if (*srcPtr == '\0')
        goto STATE_END;
    else if (*srcPtr <= ' ') {
        srcPtr++;
        goto STATE_ADVANCE;
    } else if (*srcPtr == '\"') {
        srcPtr++;
        inQuotes = 1;
    }

    if (*argc == CLI_MAX_COMMAND_ARGUMENTS)
        return CLI_RESULT_TOO_MANY_ARGUMENTS;
    argv[(*argc)++] = destPtr;
    goto STATE_PARSE_TOKEN;

STATE_PARSE_TOKEN:
    if (*srcPtr == '\0')
        goto STATE_END;
    else if ((!inQuotes && *srcPtr <= ' ') || (inQuotes && *srcPtr == '\"')) {
        *destPtr++ = '\0';
        srcPtr++;
        inQuotes = 0;
        goto STATE_ADVANCE;
    } else if (inQuotes && *srcPtr == '\\') {
        srcPtr++;
        if (*srcPtr == '\0')
            goto STATE_END;
        else if (*srcPtr == 'a')
            *srcPtr = '\a';
        else if (*srcPtr == 'n')
            *srcPtr = '\n';
        else if (*srcPtr == 'r')
            *srcPtr = '\r';
        else if (*srcPtr == 't')
            *srcPtr = '\t';
    }

    *destPtr++ = *srcPtr++;
    goto STATE_PARSE_TOKEN;

STATE_END:
    *destPtr = '\0';
    return CLI_RESULT_SUCCESS;
}

void CliSuppressEcho(CLI *hCli) {
    CLI_SET_BOOL(hCli, ECHO_IS_SUPPRESSED);
}

void CliEnableEcho(CLI *hCli) {
    CLI_CLEAR_BOOL(hCli, ECHO_IS_SUPPRESSED);
}

static const CLI_COMMAND *CliLookupCommand(const CLI *hCli, const char *name) {
    int i;

    if (name != NULL) {
        for (i = 0; i < hCli->numCommands; i++) {
            if (!strcmp(name, hCli->commandList[i].name)) {
                return &hCli->commandList[i];
            }
        }
    }

    return NULL;
}
