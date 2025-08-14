/**
 * @file
 * Date and Time unit test group file.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#include <stdbool.h>

#include "datetime.h"
#include <unity/unity_fixture.h>

extern UNITY_FIXTURE_T DateTimeGroup;

TEST_GROUP(DateTimeGroup);

TEST_SETUP(DateTimeGroup) { /* This is run before each test case. */
}

TEST_TEAR_DOWN(DateTimeGroup) { /* This is run after each test case. */
}

TEST(DateTimeGroup, DateTimeTest1) {
    DateTime myTime;

    myTime.year = 1970;
    myTime.mon = 1;
    myTime.day = 1;
    myTime.hour = 0;
    myTime.min = 0;
    myTime.sec = 0;

    TEST_ASSERT(ConvertRealTimeToUnix(&myTime) == 0);
}

TEST(DateTimeGroup, DateTimeTest2) {
    DateTime myTime;
    uint32_t myUnix;

    myTime.year = 2079;
    myTime.mon = 12;
    myTime.day = 31;
    myTime.hour = 23;
    myTime.min = 59;
    myTime.sec = 59;

    myUnix = ConvertRealTimeToUnix(&myTime);
    TEST_ASSERT(myUnix == 3471292799UL);
}

TEST(DateTimeGroup, DateTimeTest3) {
    DateTime myTime;
    uint32_t myUnix;

    myTime.year = 2000;
    myTime.mon = 1;
    myTime.day = 1;
    myTime.hour = 0;
    myTime.min = 0;
    myTime.sec = 0;

    myUnix = ConvertRealTimeToUnix(&myTime);
    TEST_ASSERT(myUnix == 946684800UL);
}

/* Unix Time to Calendar Time. */
TEST(DateTimeGroup, DateTimeTest4) {
    DateTime realTime[1];

    uint32_t unixTime = 1124733317;

    ConvertUnixTimeToReal(unixTime, realTime);
    /*
     * printf("\n Converted %d to %d/%d/%d %d:%d:%d\n", unixTime,
     *      realTime->year, realTime->mon, realTime->day,
     *      realTime->hour, realTime->min, realTime->sec);
     */
    TEST_ASSERT(realTime->year == 2005);
    TEST_ASSERT(realTime->mon == 8);
    TEST_ASSERT(realTime->day == 22);
    TEST_ASSERT(realTime->hour == 17);
    TEST_ASSERT(realTime->min == 55);
    TEST_ASSERT(realTime->sec == 17);
}

TEST(DateTimeGroup, DateTimeTest5) {
    DateTime realTime[1];
    uint32_t unixTime = 3471292799UL;

    ConvertUnixTimeToReal(unixTime, realTime);
    /*
     * printf("\n Converted %d to %d/%d/%d %d:%d:%d\n", unixTime,
     *      realTime->year, realTime->mon, realTime->day,
     *      realTime->hour, realTime->min, realTime->sec);
     */
    TEST_ASSERT(realTime->year == 2079);
    TEST_ASSERT(realTime->mon == 12);
    TEST_ASSERT(realTime->day == 31);
    TEST_ASSERT(realTime->hour == 23);
    TEST_ASSERT(realTime->min == 59);
    TEST_ASSERT(realTime->sec == 59);
}

TEST(DateTimeGroup, DateTimeTest6) {
    DateTime realTime[1];
    uint32_t unixTime = 0;

    ConvertUnixTimeToReal(unixTime, realTime);
    /*
     * printf("\n Converted %d to %d/%d/%d %d:%d:%d\n", unixTime,
     *      realTime->year, realTime->mon, realTime->day,
     *      realTime->hour, realTime->min, realTime->sec);
     */
    TEST_ASSERT(realTime->year == 1970);
    TEST_ASSERT(realTime->mon == 1);
    TEST_ASSERT(realTime->day == 1);
    TEST_ASSERT(realTime->hour == 0);
    TEST_ASSERT(realTime->min == 0);
    TEST_ASSERT(realTime->sec == 0);
}

TEST(DateTimeGroup, DateTimeTest7) {
    DateTime realTime[1];

    uint32_t unixTime = 1340901234;

    ConvertUnixTimeToReal(unixTime, realTime);
    /*
     * printf("\n Converted %d to %d/%d/%d %d:%d:%d\n", unixTime,
     *      realTime->year, realTime->mon, realTime->day,
     *      realTime->hour, realTime->min, realTime->sec);
     */
    TEST_ASSERT(realTime->year == 2012);
    TEST_ASSERT(realTime->mon == 6);
    TEST_ASSERT(realTime->day == 28);
    TEST_ASSERT(realTime->hour == 16);
    TEST_ASSERT(realTime->min == 33);
    TEST_ASSERT(realTime->sec == 54);
}

TEST(DateTimeGroup, HourRangeCheck) {
    TEST_ASSERT_TRUE(HourIsInRange(1, 0, 2));
    TEST_ASSERT_TRUE(HourIsInRange(12, 0, 23));
    TEST_ASSERT_TRUE(HourIsInRange(12, 11, 23));
    TEST_ASSERT_TRUE(HourIsInRange(12, 12, 23));
    TEST_ASSERT_TRUE(HourIsInRange(12, 23, 15));

    TEST_ASSERT_FALSE(HourIsInRange(12, 23, 1));
    TEST_ASSERT_FALSE(HourIsInRange(1, 23, 1));
    TEST_ASSERT_FALSE(HourIsInRange(23, 23, 23));
}

TEST(DateTimeGroup, DayOfWeekCheck) {
    TEST_ASSERT_EQUAL(0, ComputeDayOfWeek(2012, 11, 4));
    TEST_ASSERT_EQUAL(5, ComputeDayOfWeek(2011, 11, 4));
    TEST_ASSERT_EQUAL(2, ComputeDayOfWeek(2012, 2, 28));
    TEST_ASSERT_EQUAL(3, ComputeDayOfWeek(2012, 2, 29));
    TEST_ASSERT_EQUAL(4, ComputeDayOfWeek(2012, 3, 1));
    TEST_ASSERT_EQUAL(1, ComputeDayOfWeek(2012, 12, 31));
    TEST_ASSERT_EQUAL(2, ComputeDayOfWeek(2013, 1, 1));
    TEST_ASSERT_EQUAL(-1, ComputeDayOfWeek(2012, 13, 1));
    TEST_ASSERT_EQUAL(-1, ComputeDayOfWeek(2012, 1, 32));
    TEST_ASSERT_EQUAL(-1, ComputeDayOfWeek(1751, 1, 1));
    TEST_ASSERT_EQUAL(6, ComputeDayOfWeek(1752, 1, 1));
}

TEST(DateTimeGroup, ComputeDayOfMonthCheck) {
    TEST_ASSERT_EQUAL(3, ComputeDayOfMonth(2012, 11, 1, 6));
    TEST_ASSERT_EQUAL(4, ComputeDayOfMonth(2012, 11, 1, 0));
    TEST_ASSERT_EQUAL(-1, ComputeDayOfMonth(2012, 11, 0, 1));
    TEST_ASSERT_EQUAL(-1, ComputeDayOfMonth(2012, 11, 5, 0));
    TEST_ASSERT_EQUAL(-1, ComputeDayOfMonth(2012, 12, 6, 0));
    TEST_ASSERT_EQUAL(-1, ComputeDayOfMonth(2012, 12, 5, 2));
    TEST_ASSERT_EQUAL(30, ComputeDayOfMonth(2012, 12, 5, 0));
    TEST_ASSERT_EQUAL(-1, ComputeDayOfMonth(2012, 12, 4, 7));
    TEST_ASSERT_EQUAL(23, ComputeDayOfMonth(2012, 11, 4, 5));
    TEST_ASSERT_EQUAL(1, ComputeDayOfMonth(2012, 12, 1, 6));
}

TEST(DateTimeGroup, IsValidUnixTimeCheck) {
    DateTime time;

    time.year = UNIX_YEAR_MAXIMUM;
    time.mon = UNIX_MON_MAXIMUM;
    time.day = UNIX_DAY_MAXIMUM;
    time.hour = UNIX_HOUR_MAXIMUM;
    time.min = UNIX_MIN_MAXIMUM;
    time.sec = UNIX_SEC_MAXIMUM;
    TEST_ASSERT_TRUE(DateIsValidUnixTime(&time));

    time.year = UNIX_YEAR_MAXIMUM;
    time.mon = UNIX_MON_MAXIMUM;
    time.day = UNIX_DAY_MAXIMUM;
    time.hour = UNIX_HOUR_MAXIMUM - 1;
    time.min = UNIX_MIN_MAXIMUM;
    time.sec = UNIX_SEC_MAXIMUM;
    TEST_ASSERT_TRUE(DateIsValidUnixTime(&time));

    time.year = UNIX_YEAR_MAXIMUM;
    time.mon = UNIX_MON_MAXIMUM;
    time.day = 1;
    time.hour = 4;
    time.min = 0;
    time.sec = 0;
    TEST_ASSERT_TRUE(DateIsValidUnixTime(&time));

    time.year = UNIX_YEAR_MINIMUM;
    time.mon = UNIX_MON_MINIMUM;
    time.day = UNIX_DAY_MINIMUM;
    time.hour = UNIX_HOUR_MINIMUM;
    time.min = UNIX_MIN_MINIMUM;
    time.sec = UNIX_SEC_MINIMUM;
    TEST_ASSERT_TRUE(DateIsValidUnixTime(&time));

    time.year = UNIX_YEAR_MINIMUM;
    time.mon = UNIX_MON_MINIMUM;
    time.day = UNIX_DAY_MINIMUM + 1;
    time.hour = UNIX_HOUR_MINIMUM;
    time.min = UNIX_MIN_MINIMUM;
    time.sec = UNIX_SEC_MINIMUM;
    TEST_ASSERT_TRUE(DateIsValidUnixTime(&time));

    time.year = UNIX_YEAR_MAXIMUM;
    time.mon = 11;
    time.day = 31;
    time.hour = UNIX_HOUR_MAXIMUM;
    time.min = UNIX_MIN_MAXIMUM;
    time.sec = UNIX_SEC_MAXIMUM + 1;
    TEST_ASSERT_FALSE(DateIsValidUnixTime(&time));

    time.year = UNIX_YEAR_MAXIMUM;
    time.mon = UNIX_MON_MAXIMUM;
    time.day = UNIX_DAY_MAXIMUM;
    time.hour = UNIX_HOUR_MAXIMUM;
    time.min = UNIX_MIN_MAXIMUM;
    time.sec = UNIX_SEC_MAXIMUM + 1;
    TEST_ASSERT_FALSE(DateIsValidUnixTime(&time));

    time.year = UNIX_YEAR_MINIMUM;
    time.mon = UNIX_MON_MINIMUM;
    time.day = UNIX_DAY_MINIMUM - 1;
    time.hour = UNIX_HOUR_MINIMUM;
    time.min = UNIX_MIN_MINIMUM;
    time.sec = UNIX_SEC_MINIMUM;
    TEST_ASSERT_FALSE(DateIsValidUnixTime(&time));

    time.year = 4095;
    time.mon = UNIX_MON_MINIMUM;
    time.day = UNIX_DAY_MINIMUM;
    time.hour = UNIX_HOUR_MINIMUM;
    time.min = UNIX_MIN_MINIMUM;
    time.sec = UNIX_SEC_MINIMUM;
    TEST_ASSERT_FALSE(DateIsValidUnixTime(&time));

    time.year = 12;
    time.mon = UNIX_MON_MAXIMUM;
    time.day = UNIX_DAY_MAXIMUM;
    time.hour = UNIX_HOUR_MAXIMUM;
    time.min = UNIX_MIN_MAXIMUM;
    time.sec = UNIX_SEC_MAXIMUM;
    TEST_ASSERT_FALSE(DateIsValidUnixTime(&time));

    time.year = UNIX_YEAR_MINIMUM;
    time.mon = 15;
    time.day = UNIX_DAY_MINIMUM;
    time.hour = UNIX_HOUR_MINIMUM;
    time.min = UNIX_MIN_MINIMUM;
    time.sec = UNIX_SEC_MINIMUM;
    TEST_ASSERT_FALSE(DateIsValidUnixTime(&time));

    time.year = UNIX_YEAR_MAXIMUM;
    time.mon = UNIX_MON_MAXIMUM;
    time.day = UNIX_DAY_MAXIMUM;
    time.hour = 32;
    time.min = UNIX_MIN_MAXIMUM;
    time.sec = UNIX_SEC_MAXIMUM;
    TEST_ASSERT_FALSE(DateIsValidUnixTime(&time));

    /* Test leap years. */
    time.year = 2012;
    time.mon = 2;
    time.day = 30;
    time.hour = UNIX_HOUR_MAXIMUM;
    time.min = UNIX_MIN_MAXIMUM;
    time.sec = UNIX_SEC_MAXIMUM;
    TEST_ASSERT_FALSE(DateIsValidUnixTime(&time));

    time.year = 2012;
    time.mon = 2;
    time.day = 29;
    time.hour = UNIX_HOUR_MAXIMUM;
    time.min = UNIX_MIN_MAXIMUM;
    time.sec = UNIX_SEC_MAXIMUM;
    TEST_ASSERT_TRUE(DateIsValidUnixTime(&time));

    time.year = 2011;
    time.mon = 2;
    time.day = 29;
    time.hour = UNIX_HOUR_MAXIMUM;
    time.min = UNIX_MIN_MAXIMUM;
    time.sec = UNIX_SEC_MAXIMUM;
    TEST_ASSERT_FALSE(DateIsValidUnixTime(&time));

    time.year = 2011;
    time.mon = 2;
    time.day = 28;
    time.hour = UNIX_HOUR_MAXIMUM;
    time.min = UNIX_MIN_MAXIMUM;
    time.sec = UNIX_SEC_MAXIMUM;
    TEST_ASSERT_TRUE(DateIsValidUnixTime(&time));
}

TEST_GROUP_RUNNER(DateTimeGroup) {
    RUN_TEST_CASE(DateTimeGroup, DateTimeTest1);
    RUN_TEST_CASE(DateTimeGroup, DateTimeTest2);
    RUN_TEST_CASE(DateTimeGroup, DateTimeTest3);
    RUN_TEST_CASE(DateTimeGroup, DateTimeTest4);
    RUN_TEST_CASE(DateTimeGroup, DateTimeTest5);
    RUN_TEST_CASE(DateTimeGroup, DateTimeTest6);
    RUN_TEST_CASE(DateTimeGroup, DateTimeTest7);
    RUN_TEST_CASE(DateTimeGroup, HourRangeCheck);
    RUN_TEST_CASE(DateTimeGroup, DayOfWeekCheck);
    RUN_TEST_CASE(DateTimeGroup, ComputeDayOfMonthCheck);
    RUN_TEST_CASE(DateTimeGroup, IsValidUnixTimeCheck);
}
