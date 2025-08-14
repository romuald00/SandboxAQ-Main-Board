/**
 * @file
 * Definition of the date and time API for dealing with conversions between
 * UNIX and Gregorian calendars.
 *
 * Copyright Nuvation Research Corporation 2012-2013. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_DATETIME_H_
#define NUVC_DATETIME_H_

#include <stdbool.h>
#include <stdint.h>

#define SECS_PER_MIN (60UL)
#define SECS_PER_HOUR (60UL * SECS_PER_MIN)
#define SECS_PER_DAY (24UL * SECS_PER_HOUR)

/* Want to use smallest memory structure for DateTime members. */
typedef uint_least8_t DateTimeUnit;

/**
 * Stores the date a time information
 */
typedef struct _DateTime {
    DateTimeUnit sec;  /** The second component (0 to 59) */
    DateTimeUnit min;  /** The minute component (0 to 59) */
    DateTimeUnit hour; /** The hour component (0 to 23) */
    DateTimeUnit day;  /** The day component (1 to 31) */
    DateTimeUnit mon;  /** The month component (1 to 12) */
    uint16_t year;     /** The year component (1970 to 2070) */
} DateTime;

typedef enum {
    UNIX_YEAR_MAXIMUM = 2038,
    UNIX_MON_MAXIMUM = 1,
    UNIX_DAY_MAXIMUM = 19,
    UNIX_HOUR_MAXIMUM = 3,
    UNIX_MIN_MAXIMUM = 14,
    UNIX_SEC_MAXIMUM = 7,
} DateTimeUnixMaximum;

typedef enum {
    UNIX_YEAR_MINIMUM = 1970,
    UNIX_MON_MINIMUM = 1,
    UNIX_DAY_MINIMUM = 1,
    UNIX_HOUR_MINIMUM = 0,
    UNIX_MIN_MINIMUM = 0,
    UNIX_SEC_MINIMUM = 0
} DateTimeUnixMinimum;

/**
 * Checks whether a given date is within the range of valid UNIX time defined
 * by the BspUnixTimeMinimum and BspUnixTimeMaximum enumerations.
 *
 * The functions in this module make no guarantees about their results if the
 * time has not first been verified to conform to a valid date range.
 *
 * @param[in] time
 *      Time structure representing the time to verify.
 * @return
 *      True if the time is valid UNIX time. False otherwise.
 */
bool DateIsValidUnixTime(DateTime *time);

/**
 * Helper funciton to convert Real time and date to Unix time in Epoch
 *
 *  @param[in] realTime
 *      A pointer to the structure containing the real time components.
 *  @return
 *      The number of seconds since Epoch
 */
uint32_t ConvertRealTimeToUnix(const DateTime *realTime);

/**
 * Helper funciton to convert Unix time in Epoch to Real date and time
 *
 *  @param[in] unixTime
 *      The number of seconds since Epoch
 *  @param[out] realTime
 *      A buffer to store the structure containing the real time components
 */
void ConvertUnixTimeToReal(uint32_t unixTime, DateTime *realTime);

/**
 * Helper to check whether an hour is between two times of the day.
 *
 * This is mostly useful for functions that are only active during certain
 * hours of the day.
 *
 * All inputs must be in the range [0, 23].
 *
 * The function checks the following:
 * (hour >= start && hour < stop)
 * but it also accounts for start and stop on different days.
 *
 * @param[in] hour
 *      the hour to check whether it's in range
 * @param[in] start
 *      start of the range to check
 * @param[in] stop
 *      end of the range to check
 * @return
 *      true if the hour is within range, false otherwise
 */
bool HourIsInRange(DateTimeUnit hour, DateTimeUnit start, DateTimeUnit stop);

/**
 * Compute the Day of the Week for a given Gregorian date
 *
 * @param[in] y
 *      the year (>1752 for UK)
 * @param[in] m
 *      the month (1:January, ... 12:December)
 * @param[in] d
 *      the day of the month
 * @return
 *      the day of the week 0:Sunday...6:Saturday, or -1 if parameters out of
 *      range.
 */
int16_t ComputeDayOfWeek(uint16_t y, uint16_t m, uint16_t d);

/**
 * Compute the day of the month corresponding to Week of Month and Day of Week
 * @param[in] y
 *      the year
 * @param[in] m
 *      the month
 * @param[in] WoM
 *      the week of the month [1-5]
 * @param[in] DoW
 *      the day of the week [0:Sunday...6:Saturday]
 * @return
 *      the day of the month [1..31]or -1 if parameters out of range.
 */
int16_t ComputeDayOfMonth(uint16_t y, uint16_t m, uint16_t WoM, uint16_t DoW);

#endif /* NUVC_DATETIME_H_ */
