/*
 * MB_cncHandleMsg.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 4, 2023
 *      Author: rlegault
 */

#ifndef APP_INC_MB_CNCHANDLEMSG_H_
#define APP_INC_MB_CNCHANDLEMSG_H_

#include "cli/cli.h"
#include "cli/cli_print.h"
#include "cmdAndCtrl.h"
#include "mongooseHandler.h"
#include "saqTarget.h"
#include <stdbool.h>

typedef struct {
    osThreadId taskHandle; // release event
    cncPayload_tp p_payload;
    uint8_t xInfo;
    httpErrorCodes cmdResponse;
    CLI *hCli;
} webCncCbId_t, *webCncCbId_tp;

/**
 * @fn webCncRequestCB
 *
 * @brief Handler for web API get DAC excitation command.
 *
 * @param[in] spiDbMbCmd_e spiDbMbCmd: SPI command
 * @param[in] uint32_t cbId: Callback ID
 * @param[in] uint8_t xInfo: sensor/cmd counter
 * @param[in] spiDbMbPacketCmdResponse_t cmdResponse: uid and command response value, only used when cpiDbMbCmd is a
 * SPICMD_CNC_SHORT
 * @param[in] cncPayload_tp p_payload: Command payload
 *
 * @return osStatus: result
 **/
osStatus webCncRequestCB(spiDbMbCmd_e spiDbMbCmd,
                         uint32_t cbId,
                         uint8_t xInfo,
                         spiDbMbPacketCmdResponse_t cmdResponse,
                         cncPayload_tp p_payload);

/**
 * @fn testDestinationValue
 *
 * @brief Test destination value from web API call.
 *
 * @param[in] webResponse_tp p_webResponse: Web response
 * @param[in] int destination: Command destination
 *
 * @return webResponse_tp: Updated JSON structure
 **/
bool testDestinationValue(webResponse_tp p_webResponse, int destination);

#endif /* APP_INC_MB_CNCHANDLEMSG_H_ */
