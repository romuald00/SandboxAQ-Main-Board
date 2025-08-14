/**
 * @file
 * functions for using real time clock.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#include "realTimeClock.h"
#include "cli/cli.h"
#include "cli/cli_print.h"
#include "cmsis_os.h"
#include "datetime.h"
#include "debugPrint.h"
#include "json.h"
#include "registerParams.h"
#include "time.h"
#include "webCliMisc.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_YEAR 2000

extern RTC_HandleTypeDef hrtc;
static osMutexId rtcMutexId = NULL;

extern char *strptime(const char *__restrict __s, const char *__restrict __fmt, struct tm *__tp);

int initRTC(void) {
    // Only allow init once
    assert(rtcMutexId == NULL);

    // Create mutex
    osMutexDef(rtcMutex);
    rtcMutexId = osMutexCreate(osMutex(rtcMutex));

    // Ensure that the mutex was created
    if (rtcMutexId == NULL) {
        return -1;
    }
    return 0;
}

static double timeAtBoot = 0;
HAL_StatusTypeDef setRTC(const RTC_DateTypeDef *currentDay, const RTC_TimeTypeDef *currentTime) {
    if (currentTime == NULL || currentDay == NULL) {
        return HAL_ERROR;
    }
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    date.Year = currentDay->Year;
    date.Month = currentDay->Month;
    date.Date = currentDay->Date;
    time.Hours = currentTime->Hours;
    time.Minutes = currentTime->Minutes;
    time.Seconds = currentTime->Seconds;
    if (!IS_RTC_YEAR(date.Year) || !IS_RTC_MONTH(date.Month) || !IS_RTC_DATE(date.Date)) {
        return HAL_ERROR;
    }
    if (!IS_RTC_HOUR24(time.Hours) || !IS_RTC_MINUTES(time.Minutes) || !IS_RTC_SECONDS(time.Seconds)) {
        return HAL_ERROR;
    }

    time.StoreOperation = RTC_STOREOPERATION_SET;
    time.TimeFormat = RTC_HOURFORMAT_24;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    date.WeekDay = RTC_WEEKDAY_MONDAY;

    // Wait on the mutex
    if (osMutexWait(rtcMutexId, 1000) != osOK) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef ret1 = HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_StatusTypeDef ret2 = HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
    osMutexRelease(rtcMutexId);

    if (ret1 != HAL_OK) {
        return ret1;
    }
    return ret2;
}

void setTimeAtBoot(void) {
    timeAtBoot = timeSinceEpoch();
}

double timeSinceEpoch(void) {
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    if (getTime(&time, &date) != HAL_OK) {
        return 0;
    }

    struct tm t;
    t.tm_sec = time.Seconds;
    t.tm_min = time.Minutes;
    t.tm_hour = time.Hours;
    t.tm_mday = date.Date;
    t.tm_mon = date.Month;
    t.tm_year = BASE_YEAR + date.Year;

    return (double)mktime(&t) + (double)time.SubSeconds / (double)(time.SecondFraction + 1);
}

double timeSinceBoot(void) {
    return timeSinceEpoch() - timeAtBoot;
}

HAL_StatusTypeDef getTime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date) {
    if (osMutexWait(rtcMutexId, 1000) != osOK) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef ret1 = HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN);
    HAL_StatusTypeDef ret2 = HAL_RTC_GetDate(&hrtc, date, RTC_FORMAT_BIN);
    osMutexRelease(rtcMutexId);

    if (ret1 != HAL_OK) {
        return ret1;
    }
    return ret2;
}

int16_t rtcCliCmd(CLI *hCli, int argc, char *argv[]) {
    if (argc > 1) {
        if (argc == 2 && strcmp(argv[1], "get") == 0) {
            RTC_TimeTypeDef time;
            RTC_DateTypeDef date;
            if (getTime(&time, &date) != HAL_OK) {
                CliPrintf(hCli, "Time not initialized\r\n");
                return 0;
            }
            CliPrintf(hCli,
                      "%04u %02u %02u %02u:%02u:%02u\r\n",
                      date.Year + BASE_YEAR,
                      date.Month,
                      date.Date,
                      time.Hours,
                      time.Minutes,
                      time.Seconds);
        } else if (argc == 3 && strcmp(argv[1], "set") == 0) {
            struct tm tm;
            memset(&tm, 0, sizeof(tm));
            strptime(argv[2], "%Y %m %d %T", &tm);
            // tm.year is year-1900, but STM32 register is only allowed 0-99 so subtract another 100
            // tm.mon is 0-11 while RTC is 1-12 so add 1.
            RTC_DateTypeDef currentDay = {
                .Year = tm.tm_year - 100, .Month = tm.tm_mon + 1, .Date = tm.tm_mday, .WeekDay = tm.tm_wday};
            RTC_TimeTypeDef currentTime = {.Hours = tm.tm_hour,
                                           .Minutes = tm.tm_min,
                                           .Seconds = tm.tm_sec,
                                           .DayLightSaving = RTC_DAYLIGHTSAVING_NONE,
                                           .TimeFormat = RTC_HOURFORMAT_24,
                                           .StoreOperation = RTC_STOREOPERATION_SET};

            HAL_StatusTypeDef status = setRTC(&currentDay, &currentTime);
            if (status != HAL_OK) {
                CliPrintf(hCli, "Failed to write RTC time and date\r\n");
                return 0;
            }
            CliPrintf(hCli, "Success\r\n");
            return 1;
        }
    }
    return 1;
}

webResponse_tp webTimeGet(const char *body, const int bodyLen) {
    WEB_CMD_NO_PARAM_SETUP(body, bodyLen);

    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    if (getTime(&time, &date) != HAL_OK) {
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;
        json_object *jsonResult = json_object_new_string("error");
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        return p_webResponse;
    }

    p_webResponse->httpCode = HTTP_OK;

    json_object *jsonTmp = json_object_new_int(date.Year + BASE_YEAR);
    json_object_object_add_ex(p_webResponse->jsonResponse, "year", jsonTmp, JSON_C_OBJECT_KEY_IS_CONSTANT);

    jsonTmp = json_object_new_int(date.Month);
    json_object_object_add_ex(p_webResponse->jsonResponse, "month", jsonTmp, JSON_C_OBJECT_KEY_IS_CONSTANT);

    jsonTmp = json_object_new_int(date.Date);
    json_object_object_add_ex(p_webResponse->jsonResponse, "day", jsonTmp, JSON_C_OBJECT_KEY_IS_CONSTANT);

    jsonTmp = json_object_new_int(time.Hours);
    json_object_object_add_ex(p_webResponse->jsonResponse, "hour24", jsonTmp, JSON_C_OBJECT_KEY_IS_CONSTANT);

    jsonTmp = json_object_new_int(time.Minutes);
    json_object_object_add_ex(p_webResponse->jsonResponse, "minute", jsonTmp, JSON_C_OBJECT_KEY_IS_CONSTANT);

    jsonTmp = json_object_new_int(time.Seconds);
    json_object_object_add_ex(p_webResponse->jsonResponse, "second", jsonTmp, JSON_C_OBJECT_KEY_IS_CONSTANT);

    return p_webResponse;
}

bool testYearValue(webResponse_tp p_webResponse, int year) {
    TEST_VALUE(year, BASE_YEAR, BASE_YEAR + 99);
}

bool testMonthValue(webResponse_tp p_webResponse, int month) {
    TEST_VALUE(month, 1, 13);
}

bool testDayValue(webResponse_tp p_webResponse, int day) {
    TEST_VALUE(day, 1, 32);
}

bool testHourValue(webResponse_tp p_webResponse, int hour) {
    TEST_VALUE(hour, 0, 24);
}

bool testMinuteValue(webResponse_tp p_webResponse, int minute) {
    TEST_VALUE(minute, 0, 60);
}

bool testSecondValue(webResponse_tp p_webResponse, int second) {
    TEST_VALUE(second, 0, 60);
}

webResponse_tp webTimeSet(const char *jsonStr, const int strLen) {
    WEB_CMD_PARAM_SETUP(jsonStr, strLen);

    GET_REQ_KEY_VALUE(int, year, obj, json_object_get_int);

    if (!testYearValue(p_webResponse, year)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }

    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    date.Year = year - BASE_YEAR;

    GET_REQ_KEY_VALUE(int, month, obj, json_object_get_int)
    if (!testMonthValue(p_webResponse, month)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    date.Month = month;

    GET_REQ_KEY_VALUE(int, day, obj, json_object_get_int)
    if (!testDayValue(p_webResponse, day)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    date.Date = day;

    GET_REQ_KEY_VALUE(int, hour24, obj, json_object_get_int)
    if (!testHourValue(p_webResponse, hour24)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    time.Hours = hour24;

    GET_REQ_KEY_VALUE(int, minute, obj, json_object_get_int)
    if (!testMinuteValue(p_webResponse, minute)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    time.Minutes = minute;

    GET_REQ_KEY_VALUE(int, second, obj, json_object_get_int)
    if (!testSecondValue(p_webResponse, second)) {
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    time.Seconds = second;

    HAL_StatusTypeDef status = setRTC(&date, &time);
    if (status != HAL_OK) {
        json_tokener_free(jsonTok);
        json_object_put(obj);
        json_object *jsonResult = json_object_new_string("set real time clock failure");
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object *jsonParam = json_object_new_int(status);
        json_object_object_add_ex(p_webResponse->jsonResponse, "error", jsonParam, JSON_C_OBJECT_KEY_IS_CONSTANT);
        WEB_CMD_PARAM_CLEANUP
        return p_webResponse;
    }
    struct json_object *jsonResult = json_object_new_string("Success");
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);

    WEB_CMD_PARAM_CLEANUP
    return p_webResponse;
}

RETURN_CODE getUptimeAsRegister(const registerInfo_tp regInfo) {
    regInfo->u.dataUint = round(timeSinceBoot());
    return 0;
}
