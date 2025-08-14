/**
 * @file
 * Unit test group file for the sprintf function.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifdef USE_NUVC_SNPRINTF

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unity/unity_fixture.h>

#include "sprintf.h"

#define TEST_BUF_SIZE 2048

extern UNITY_FIXTURE_T SprintfGroup;

TEST_GROUP(SprintfGroup);

TEST_SETUP(SprintfGroup) {
    /* Make sure that we have enough room to test all cases. */
    TEST_ASSERT_TRUE(TEST_BUF_SIZE > MAX_SNPRINTF_SIZE);
}

TEST_TEAR_DOWN(SprintfGroup) {
}

TEST(SprintfGroup, VariousPatterns) {
    char buffer[TEST_BUF_SIZE];

    /* Test 1 */
    TEST_ASSERT_EQUAL(13, snprintf(buffer, MAX_SNPRINTF_SIZE, "Hello World!\n"));
    TEST_ASSERT_EQUAL(0, strcmp("Hello World!\n", buffer));

    /* Test 2 */
    TEST_ASSERT_EQUAL(25, snprintf(buffer, MAX_SNPRINTF_SIZE, "a: %d\nb: %d\nc: %d\nd: %d\n", 1, 2, -4, 30000));
    TEST_ASSERT_EQUAL(0, strcmp("a: 1\nb: 2\nc: -4\nd: 30000\n", buffer));

    /* Test 3 */
    TEST_ASSERT_EQUAL(21, snprintf(buffer, MAX_SNPRINTF_SIZE, "a: %l\nb: %l\nc: %l\n", 1UL, 2UL, 3000000UL));
    TEST_ASSERT_EQUAL(0, strcmp("a: 1\nb: 2\nc: 3000000\n", buffer));

    /* Test 4 */
    TEST_ASSERT_EQUAL(17, snprintf(buffer, MAX_SNPRINTF_SIZE, "0x%04y 0x%8Y", 0x0430UL, 0xf00ba5UL));
    TEST_ASSERT_EQUAL(0, strcmp("0x0430 0x  F00BA5", buffer));

    /* Test 5 */
    TEST_ASSERT_EQUAL(10, snprintf(buffer, MAX_SNPRINTF_SIZE, "%10s", "foo"));
    TEST_ASSERT_EQUAL(0, strcmp("       foo", buffer));

    /* Test 6 */
    TEST_ASSERT_EQUAL(10, snprintf(buffer, MAX_SNPRINTF_SIZE, "%-10s", "foo"));
    TEST_ASSERT_EQUAL(0, strcmp("foo       ", buffer));

    /* Test 7 */
    TEST_ASSERT_EQUAL(18, snprintf(buffer, MAX_SNPRINTF_SIZE, "%10s", "foo longer than 10"));
    TEST_ASSERT_EQUAL(0, strcmp("foo longer than 10", buffer));
}

TEST(SprintfGroup, SizeLimitations) {
    char buffer[16] = {'\0'};

    TEST_ASSERT_EQUAL(8, snprintf(buffer, 8, "foo longer than 8"));
    TEST_ASSERT_EQUAL(0, strcmp("foo long", buffer));
    TEST_ASSERT_NOT_EQUAL(0, strcmp("foo longer than 8", buffer));
}

TEST_GROUP_RUNNER(SprintfGroup) {
    RUN_TEST_CASE(SprintfGroup, VariousPatterns);
    RUN_TEST_CASE(SprintfGroup, SizeLimitations);
}

#endif /* USE_NUVC_SNPRINTF */
