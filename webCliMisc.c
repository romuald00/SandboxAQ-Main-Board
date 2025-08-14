/*
 * webCliMisc.c
 *
 * Handle miscellaneous commands from the CLI or WebServer
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 5, 2023
 *      Author: rlegault
 */

#include "webCliMisc.h"
#include "mongooseHandler.h"
#include "saqTarget.h"
#include "sockets.h"

#include <regex.h>
#include <stdbool.h>
#include <stdio.h>

bool validIpStr(const char *ipAddress) {
#if 0
    const char * ipReg = "/^([0-9]+(\.|$)){4}/";
    regex_t regex;
    int reti = regcomp(&regex, ipReg, 0);
    assert(reti!=0);
    reti = regexec(&regex, ipAddress, 0, NULL, 0);
    regfree(&regex);
    if(reti==0) {
        return true;
    }
    return false;
#endif
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

bool testDaughterBoardId(webResponse_tp p_webResponse, int dbId) {
    if (dbId == MAIN_BOARD_ID) {
        return true;
    }
    TEST_VALUE(dbId, 0, MAX_CS_ID);
}
