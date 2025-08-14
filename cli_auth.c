/**
 *  @file
 *  Portable Command Line Interface Library
 *
 *  Source file for the portable command line interface authentication capability.
 *
 *  Copyright Nuvation Research Corporation 2018-2019. All Rights Reserved.
 *  www.nuvation.com
 */

#include "cli/cli_auth.h"
#include "cli/cli.h"
#include "cli/cli_print.h"
#include "macros.h"
#include "ringbuffer.h"
#include "secure_strncmp.h"
#include <stdint.h>
#include <string.h>

char passwd[CLI_AUTH_PASSWD_MAX_LEN];
const char *const defaultPasswd = CLI_AUTH_DEFAULT_PASSWD;
const CliAuthConfig *cliAuthConfig;

uint8_t authInit(const CliAuthConfig *config) {
    cliAuthConfig = config;

    strncpy(passwd, defaultPasswd, CLI_AUTH_PASSWD_MAX_LEN);

    return 0;
}

uint8_t authenticatePrompt(CLI *hCli, uint8_t commandId) {
#ifdef CLI_NO_AUTH
    return 1;
#endif
    uint16_t inputLen = 0;
    uint8_t authenticated = 0;
    uint8_t passwdBuf[CLI_AUTH_PASSWD_MAX_LEN];
    uint16_t passwdBufLen = 0;
    uint32_t startTick = 0;
    uint16_t visibleLen = 0;

    if (!CLI_CHECK_BOOL(hCli, ECHO_IS_SUPPRESSED)) {
        CliWriteString(hCli, "Password: ");
    }

    if (cliAuthConfig->osTickFunc != NULL) {
        startTick = (*cliAuthConfig->osTickFunc)();
    }

    while (1) {
        inputLen =
            SmallRbRead(&hCli->stdinBuffer.ringBuffer, &passwdBuf[passwdBufLen], ARRAY_SIZE(passwdBuf) - passwdBufLen);
        passwdBufLen += inputLen;

        if (passwdBufLen > 0 && inputLen > 0) {
            uint16_t i = passwdBufLen - inputLen;
            while (i < passwdBufLen) {
                if (passwdBuf[i] == '\b' || passwdBuf[i] == DEL_CHAR) {
                    if (visibleLen > 0) {
                        static const char backspace[] = "\b \b";
                        CliWriteString(hCli, backspace);
                        visibleLen--;
                    }

                    if (i > 0) {
                        if (i + 1 < passwdBufLen) {
                            memmove(&passwdBuf[i - 1], &passwdBuf[i + 1], passwdBufLen - i - 1);
                        }
                        passwdBufLen -= 2;
                        i--;
                    } else {
                        if (i + 1 < passwdBufLen) {
                            memmove(&passwdBuf[i], &passwdBuf[i + 1], passwdBufLen - i - 1);
                        }
                        passwdBufLen--;
                    }
                } else if (!CLI_CHECK_BOOL(hCli, ECHO_IS_SUPPRESSED)) {
                    if (passwdBuf[i] != '\r' && passwdBuf[i] != '\n') {
                        CliWriteString(hCli, "*");
                    }
                    i++;
                    visibleLen++;
                } else {
                    i++;
                    visibleLen++;
                }
            }
            if (passwdBufLen > 0) {
                if (passwdBuf[passwdBufLen - 1] == '\r' || passwdBuf[passwdBufLen - 1] == '\n' ||
                    passwdBufLen == CLI_AUTH_PASSWD_MAX_LEN) {
                    if (passwdBufLen > 1 &&
                        (passwdBuf[passwdBufLen - 2] == '\r' || passwdBuf[passwdBufLen - 2] == '\n')) {
                        passwdBuf[passwdBufLen - 2] = 0;
                    } else {
                        passwdBuf[passwdBufLen - 1] = 0;
                    }
                    authenticated = !securestrncmp((char *)passwdBuf, passwd, ARRAY_SIZE(passwd));
                    goto auth_result;
                }
            }
        }

        if (cliAuthConfig->watchdogKickFunc != NULL) {
            (*cliAuthConfig->watchdogKickFunc)();
        }

        if (cliAuthConfig->osTickFunc != NULL) {
            if ((*cliAuthConfig->osTickFunc)() > startTick + cliAuthConfig->timeoutMs) {
                CliWriteString(hCli, "\r\nTimeout exceeded\r\n");
                return 0;
            }
        }

        if (cliAuthConfig->osYieldFunc != NULL) {
            (*cliAuthConfig->osYieldFunc)();
        }
    }

auth_result:
    CliWriteString(hCli, "\r\n");

    if (cliAuthConfig->logFunc != NULL) {
        (*cliAuthConfig->logFunc)(authenticated, commandId);
    }

    if (!authenticated) {
        CliWriteString(hCli, "Incorrect password\r\n");
        if (cliAuthConfig->delayFunc != NULL) {
            (*cliAuthConfig->delayFunc)(cliAuthConfig->failDelayMs);
        }
    }

    return authenticated;
}
