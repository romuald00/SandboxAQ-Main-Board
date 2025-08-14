/*
 * perseioTrace.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Jan 22, 2024
 *      Author: rlegault
 */

#ifndef LIB_INC_PERSEIOTRACE_H_
#define LIB_INC_PERSEIOTRACE_H_

#ifdef TRACEALYZER

#include "saqTarget.h"
#include "trcRecorder.h"

extern TraceStringHandle_t dbCommTrace[MAX_CS_ID];
extern TraceStringHandle_t spiCommTrace[MAX_SPI];
extern TraceStringHandle_t traceMsg;
extern TraceStringHandle_t gatherMsg;
extern TraceStringHandle_t urlLogTxMsg;
extern TraceStringHandle_t imuMsg;
void traceInit(void);

#endif

#endif /* LIB_INC_PERSEIOTRACE_H_ */
