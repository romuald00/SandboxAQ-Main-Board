/**
 *  @file
 *  Portable Command Line Interface Library
 *
 *  Header file for the portable command line interface authentication capability.
 *
 *  Copyright Nuvation Research Corporation 2018. All Rights Reserved.
 *  www.nuvation.com
 */

#ifndef NUVC_CLI_AUTH_H_
#define NUVC_CLI_AUTH_H_

#include "cli.h"
#include <stdint.h>

#define CLI_AUTH_PASSWD_MAX_LEN 64
#define DEL_CHAR 127
#define CLI_AUTH_DEFAULT_PASSWD "jgroptics"

typedef uint8_t (*cliAuthLogFunc)(uint8_t success, uint8_t commandId);

typedef struct CliAuthConfig {
    cliAuthLogFunc logFunc;
    void (*watchdogKickFunc)();
    void (*osYieldFunc)();
    uint32_t (*osTickFunc)();
    void (*delayFunc)(uint32_t delayMs);
    uint16_t failDelayMs;
    uint16_t timeoutMs;
} CliAuthConfig;

/**
 * \brief
 * Initialize the authorization API.
 *
 * @param config Configuration for authentication
 *
 * @returns 0 if successful
 */
uint8_t authInit(const CliAuthConfig *config);

/**
 * \brief
 * Prompt the user for a password and authenticate them.
 *
 * @param hCli The CLI handle
 * @param commandId ID of the command to log when authentication is attempted
 *
 * @returns 0 if successful
 */
uint8_t authenticatePrompt(CLI *hCli, uint8_t commandId);

#endif /* NUVC_CLI_AUTH_H_ */
