/**
 * @file
 * Unit test group file for the byte order API.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <unity/unity_fixture.h>

#include "byte_order.h"

extern UNITY_FIXTURE_T ByteOrderGroup;

typedef union {
    char chars[5];
    uint32_t value;
} end32;

typedef union {
    char chars[3];
    uint16_t value;
} end16;

TEST_GROUP(ByteOrderGroup);

TEST_SETUP(ByteOrderGroup) { /* This is run before each test case. */
}

TEST_TEAR_DOWN(ByteOrderGroup) { /* This is run after each test case. */
}

TEST(ByteOrderGroup, BigEndianConversions) {
    end32 bigVar32;
    end16 bigVar16;

    strcpy(bigVar32.chars, "0123");
    strcpy(bigVar16.chars, "45");

    TEST_ASSERT_EQUAL(bigVar32.value, htob32(0x30313233));
    TEST_ASSERT_EQUAL(bigVar16.value, htob16(0x3435));
}

TEST(ByteOrderGroup, LittleEndianConversions) {
    end32 littleVar32;
    end16 littleVar16;

    strcpy(littleVar32.chars, "3210");
    strcpy(littleVar16.chars, "54");

    TEST_ASSERT_EQUAL(littleVar32.value, htol32(0x30313233));
    TEST_ASSERT_EQUAL(littleVar16.value, htol16(0x3435));
}

TEST(ByteOrderGroup, BigEndianWordConversions) {
    end32 bigVar32;
    end16 bigVar16;

    bigVar32.chars[0] = (char)0x3031;
    bigVar32.chars[1] = (char)0x3233; /* packed "0123" */
    bigVar32.chars[2] = (char)0x0;
    bigVar16.chars[0] = (char)0x3435; /* packed "45" */
    bigVar16.chars[1] = (char)0x0;

    TEST_ASSERT_EQUAL(bigVar32.value, htob32(0x30313233));
    TEST_ASSERT_EQUAL(bigVar16.value, htob16(0x3435));
}

TEST(ByteOrderGroup, LittleEndianWordConversions) {
    end32 littleVar32;
    end16 littleVar16;

    littleVar32.chars[0] = (char)0x3332;
    littleVar32.chars[1] = (char)0x3130; /* packed "3210" */
    littleVar32.chars[2] = (char)0x0;
    littleVar16.chars[0] = (char)0x3534; /* packed "54" */
    littleVar16.chars[1] = (char)0x0;

    TEST_ASSERT_EQUAL(littleVar32.value, htol32(0x30313233));
    TEST_ASSERT_EQUAL(littleVar16.value, htol16(0x3435));
}

TEST_GROUP_RUNNER(ByteOrderGroup) {
    if (sizeof(char) == sizeof(short)) {
        /* 16 bit char */
        RUN_TEST_CASE(ByteOrderGroup, BigEndianWordConversions);
        RUN_TEST_CASE(ByteOrderGroup, LittleEndianWordConversions);
    } else {
        RUN_TEST_CASE(ByteOrderGroup, BigEndianConversions);
        RUN_TEST_CASE(ByteOrderGroup, LittleEndianConversions);
    }
}
