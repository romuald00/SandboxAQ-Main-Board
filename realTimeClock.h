/**
 * @file
 * functions for using real time clock.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef __REAL_TIME_CLOCK_H__
#define __REAL_TIME_CLOCK_H__

#include "cli/cli_print.h"
#include "mongooseHandler.h"
#include "stmTarget.h"

/**
 *
 * Initialize the Real time driver
 *
 * @return 0 success else error
 */
int initRTC(void);

/**
 *
 * Set global counter to current RTC value
 *
 *
 */
void setTimeAtBoot(void);

/**
 *
 * @return the amount of seconds since the 2000 epoch.
 *
 */
double timeSinceEpoch(void);

/**
 *
 * @return the uptime in seconds
 *
 */
double timeSinceBoot(void);

/**
 * Get the time from the Real time chip
 *
 * @param[out] time structure
 * @param[out] date structure
 * @return HAL_OK if success else error
 *
 */
HAL_StatusTypeDef getTime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date);

/**
 * Handle the CLI command destined for the RTC
 *
 * @param[in] hCli print destination
 * @param[in] argc number of args in argv
 * @param[in] argv string array with command and parameters for RTC
 * @return 1 if success else 0
 *
 */
int16_t rtcCliCmd(CLI *hCli, int argc, char *argv[]);

/**
 * Return to the webserver a json structure to send to the user as a http body.
 *
 * @param[in] body is unused
 * @param[in] body len is unused
 * @return http response, null if out of memory error occurs
 *
 */
webResponse_tp webTimeGet(const char *body, const int bodyLen);

/**
 * Return to the webserver a json structure to send to the user as a http body.
 *
 * @param[in] body contains the time and date parameters as a json structure
 * @param[in] string length of the body
 * @return http response, null if out of memory error occurs
 *
 */
webResponse_tp webTimeSet(const char *body, const int bodyLen);

#endif /* __REAL_TIME_CLOCK_H__ */
