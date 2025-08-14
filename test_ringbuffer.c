/**
 * @file
 * Unit test group file for the ringbuffer checks.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <unity/unity_fixture.h>

#include "ringbuffer.h"

#define TEST_BUF_SIZE 2048
#define SMALL_RB_LOG2SIZE 2
#define SMALL_RB_SIZE (1 << SMALL_RB_LOG2SIZE)

extern UNITY_FIXTURE_T RingBufferGroup;
SMALL_RINGBUFFER_TYPE(SMALL_RB_LOG2SIZE) smallRb;
static uint8_t tempBuffer[TEST_BUF_SIZE];

TEST_GROUP(RingBufferGroup);

TEST_SETUP(RingBufferGroup) {
    TEST_ASSERT_NOT_EQUAL(NULL, tempBuffer);
    SmallRbInit(&smallRb.ringBuffer, sizeof(smallRb));
    TEST_ASSERT_EQUAL(SMALL_RB_SIZE, smallRb.ringBuffer.size);
}

TEST_TEAR_DOWN(RingBufferGroup) {
    memset(tempBuffer, 0, TEST_BUF_SIZE);
}

TEST(RingBufferGroup, SmallRbFillTest) {
    int i;
    uint8_t dummy[SMALL_RB_SIZE] = {
        0,
    };

    /* Clear the small ringbuffer. */
    SmallRbRead(&smallRb.ringBuffer, dummy, SMALL_RB_SIZE);

    for (i = 0; i < SMALL_RB_SIZE - 1; i++) {
        TEST_ASSERT_EQUAL(1, SmallRbWrite(&smallRb.ringBuffer, dummy, 1));
    }

    TEST_ASSERT_EQUAL(0, SmallRbWrite(&smallRb.ringBuffer, dummy, 1));
}

TEST(RingBufferGroup, SmallRbReadTest) {
    const uint8_t orig[SMALL_RB_SIZE] = "new";
    int messageLength = strlen((const char *)orig);

    SmallRbRead(&smallRb.ringBuffer, tempBuffer, SMALL_RB_SIZE);

    TEST_ASSERT_EQUAL(messageLength, SmallRbWrite(&smallRb.ringBuffer, orig, messageLength));
    TEST_ASSERT_EQUAL(messageLength, SmallRbGetFillLength(&smallRb.ringBuffer));
    TEST_ASSERT_EQUAL(messageLength, SmallRbRead(&smallRb.ringBuffer, tempBuffer, SMALL_RB_SIZE));
    TEST_ASSERT_EQUAL(0, SmallRbGetFillLength(&smallRb.ringBuffer));
    TEST_ASSERT_EQUAL_STRING(orig, tempBuffer);
}

TEST(RingBufferGroup, SmallRbVerifyFillLength) {
    uint8_t dummy = 0;
    uint16_t size;

    for (size = 0; size < SMALL_RB_SIZE - 1; size++) {
        TEST_ASSERT_EQUAL(size, SmallRbGetFillLength(&smallRb.ringBuffer));
        TEST_ASSERT_EQUAL(1, SmallRbWrite(&smallRb.ringBuffer, &dummy, 1));
    }

    TEST_ASSERT_EQUAL(0, SmallRbWrite(&smallRb.ringBuffer, &dummy, 1));
    TEST_ASSERT_EQUAL(size, SmallRbGetFillLength(&smallRb.ringBuffer));
}

TEST_GROUP_RUNNER(RingBufferGroup) {
    RUN_TEST_CASE(RingBufferGroup, SmallRbFillTest);
    RUN_TEST_CASE(RingBufferGroup, SmallRbReadTest);
    RUN_TEST_CASE(RingBufferGroup, SmallRbVerifyFillLength);
}
