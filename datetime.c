/**
 * @file
 * Implementation of the date and time API for dealing with conversions
 *between
 * UNIX and Gregorian calendars.
 *
 * Copyright Nuvation Research Corporation 2012-2013. All Rights Reserved.
 * www.nuvation.com
 */

#include "datetime.h"
#include "asserts.h"

#define IS_LEAP_YEAR(year) ((year % 400 == 0) || ((year % 100 != 0) && (year % 4 == 0)))

#define YEAR_TO_DAYS(y) ((uint32_t)(y)*365 + (y) / 4 - (y) / 100 + (y) / 400)

bool DateIsValidUnixTime(DateTime *time) {
    ASSERT_TRUE(time);

    if (time->year > UNIX_YEAR_MAXIMUM || time->year < UNIX_YEAR_MINIMUM) {
        return false;
    }

    if (time->mon > 12 || time->mon == 0 || time->day > 31 || time->day == 0 || time->hour > 24 || time->min > 59 ||
        time->sec > 59) {
        return false;
    }

    /* Verify day is valid for February. */
    if (time->mon == 2) {
        if (IS_LEAP_YEAR(time->year)) {
            if (time->day > 29) {
                return false;
            }
        } else if (time->day > 28) {
            return false;
        }
    }

    /* Verify day is valid for April, June, September, November. */
    if ((time->mon == 4 || time->mon == 6 || time->mon == 9 || time->mon == 11) && time->day > 30) {
        return false;
    }

    if (time->year == UNIX_YEAR_MAXIMUM) {
        if (time->mon > UNIX_MON_MAXIMUM)
            return false;
        if (time->day > UNIX_DAY_MAXIMUM)
            return false;

        if (time->day == UNIX_DAY_MAXIMUM) {
            if (time->hour > UNIX_HOUR_MAXIMUM)
                return false;

            if (time->hour == UNIX_HOUR_MAXIMUM) {
                if (time->min > UNIX_MIN_MAXIMUM)
                    return false;

                if (time->min == UNIX_MIN_MAXIMUM) {
                    if (time->sec > UNIX_SEC_MAXIMUM)
                        return false;
                }
            }
        }
    }

    if (time->year == UNIX_YEAR_MINIMUM) {
        /* Do not check hour/min/sec as any time past the minumum DD/MM/YYYY is
         * valid. Testing the unsigned DateTime type to ensure it is greater
         * than zero is pointless (and also generates a relevant warning). */
        if (time->mon < UNIX_MON_MINIMUM)
            return false;

        if (time->mon == UNIX_MON_MINIMUM) {
            if (time->day < UNIX_DAY_MINIMUM)
                return false;
        }
    }

    return true;
}

uint32_t ConvertRealTimeToUnix(const DateTime *realTime) {
    /* The day of the year that each month starts on (not during leap year). */
    static const uint16_t startOfMonth[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    uint32_t days;
    uint16_t myYear = realTime->year - 1900;
    DateTimeUnit myMon = realTime->mon - 1;

    /* Calculate days since epoch (midnight on Jan 1, 1970). */
    days = (myYear / 4) * (4 * 365 + 1) + (myYear & 0x3) * 365;
    days -= (70 / 4) * (4 * 365 + 1) + (70 & 0x3) * 365;
    days += startOfMonth[myMon];
    if ((myYear & 0x3) == 0) {
        /* Leap year: subtract 1 since 1900 was not a leap year. */
        days -= 1;
        if (myMon > 1) {
            /* Add Feb 29 */
            days += 1;
        }
    }
    days += realTime->day - 1;

    /* Calculate seconds since epoch (verified against mktime for 1970-2070). */
    return 60 * (60 * (realTime->hour + 24 * days) + realTime->min) + realTime->sec;
}

void ConvertUnixTimeToReal(uint32_t unixTime, DateTime *realTime) {
    /** Determine hour/minutes/seconds. */
    realTime->sec = (DateTimeUnit)(unixTime % 60);
    unixTime /= 60;

    realTime->min = (DateTimeUnit)(unixTime % 60);
    unixTime /= 60;

    realTime->hour = (DateTimeUnit)(unixTime % 24);
    unixTime /= 24;

    /* unixTime is now days since 01/01/1970 UTC. Re-baseline to the Common Era
     */
    unixTime += 719499;

    /* Roll forward looking for the year. Start at 1969 because the year we
     * calculate here runs from March - so January and February 1970 will
     * come out as 1969 here. */
    uint32_t year = 1969;
    while (unixTime > YEAR_TO_DAYS(year + 1) + 30)
        year++;

    realTime->year = (uint16_t)year;

    /* So subtract off the days accounted for by full years. */
    unixTime -= YEAR_TO_DAYS(realTime->year);

    /* unixTime is now number of days we are into the year
     * (remembering that March 1 is the first day of the "year" still).
     * Roll forward looking for the month.
     * 1 = March through to 12 = February. */
    uint32_t month = 1;
    while (month < 12 && unixTime > 367 * (month + 1) / 12)
        month++;

    realTime->mon = (DateTimeUnit)month;

    /* Subtract off the days accounted for by full months. */
    unixTime -= 367 * realTime->mon / 12;

    /* unixTime is now number of days we are into the month.
     * Adjust the month/year so that 1 = January, and years start where we
     * usually expect them to. */
    realTime->mon += 2;
    if (realTime->mon > 12) {
        realTime->mon -= 12;
        realTime->year++;
    }

    realTime->day = (DateTimeUnit)unixTime;
}

bool HourIsInRange(DateTimeUnit hour, DateTimeUnit start, DateTimeUnit stop) {
    if (start > stop) {
        stop += 24;
        if (start > hour)
            hour += 24;
    }

    return hour >= start && hour < stop;
}

int16_t ComputeDayOfWeek(uint16_t y, uint16_t m, uint16_t d) {
    static const uint16_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

    /* source of this algorithm
     * https://groups.google.com/forum/?hl=en&fromgroups=#!msg/comp.lang.c/KTnoRmSao6A/uViF8AkwNc4J
     * or http://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week#cite_ref-7
     * Tomohiko Sakamoto
     * 1 <= m <= 12,  y > 1752 (in the U.K.). */
    if (y < 1752)
        return -1;
    if (m > 12)
        return -1;
    if (d > 31)
        return -1;
    y -= m < 3;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

int16_t ComputeDayOfMonth(uint16_t y, uint16_t m, uint16_t WoM, uint16_t DoW) {
    uint16_t day;
    uint16_t DoW1st = ComputeDayOfWeek(y, m, 1);

    if (DoW > 6)
        return -1;
    if (WoM < 1 || WoM > 5)
        return -1;

    if (DoW1st > DoW) {
        day = 7 - DoW1st + 1 + DoW;
    } else {
        day = DoW - DoW1st + 1;
    }

    day += 7 * (WoM - 1);
    if (day > 31) {
        return -1;
    }
    return day;
}
