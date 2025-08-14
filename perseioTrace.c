/*
 * perseioTrace.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Jan 22, 2024
 *      Author: rlegault
 */

#ifdef TRACEALYZER

#include "trcRecorder.h"
#include "trcString.h"
#include <perseioTrace.h>
#include <stdio.h>

TraceStringHandle_t urlLogTxMsg;
TraceStringHandle_t dbCommTrace[MAX_CS_ID];
TraceStringHandle_t spiCommTrace[MAX_SPI];
TraceStringHandle_t traceMsg;
TraceStringHandle_t gatherMsg;
TraceStringHandle_t imuMsg;

void traceInit(void) {
    char buf[24];
    for (int i = 0; i < MAX_CS_ID; i++) {
        snprintf(buf, 24, "dbComm %d", i);
        dbCommTrace[i] = xTraceRegisterString(buf);
    }
    for (int i = 0; i < MAX_SPI; i++) {
        snprintf(buf, 24, "spiComm %d", i);
        spiCommTrace[i] = xTraceRegisterString(buf);
    }
    traceMsg = xTraceRegisterString("msgs");
    gatherMsg = xTraceRegisterString("gather");
    urlLogTxMsg = xTraceRegisterString("urlLog");
    imuMsg = xTraceRegisterString("imuMsg");
    xTracePrintCompactF0(traceMsg, "TEST");
}

#endif
