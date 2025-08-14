/**
 * @file
 * File that starts the unity test.
 *
 * Copyright Nuvation Research Corporation 2014-2018. All Rights Reserved.
 * www.nuvation.com
 */

#include <stddef.h>
#include <stdint.h>

#include <unity/unity_fixture.h>

void RunAllTests(void) {
    RUN_TEST_GROUP(SprintfGroup);
    RUN_TEST_GROUP(RingBufferGroup);
    RUN_TEST_GROUP(DateTimeGroup);
    RUN_TEST_GROUP(ByteOrderGroup);
    RUN_TEST_GROUP(tryCatchGroup);
}

int main(int argc, char **argv) {
    return UnityMain(argc, argv, RunAllTests);
}
