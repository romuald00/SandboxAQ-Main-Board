/**
 * @file
 * Unit test group file for function scop exception handling macros.
 *
 * Copyright Nuvation Research Corporation 2014-2018. All Rights Reserved.
 * www.nuvation.com
 */
#include "try_catch.h"
#include <unity/unity_fixture.h>

extern UNITY_FIXTURE_T tryCatchGroup;

TEST_GROUP(tryCatchGroup);

TEST_SETUP(tryCatchGroup) {
}

TEST_TEAR_DOWN(tryCatchGroup) {
}

/* This is just to help with local testing right here. */
typedef enum {
    StatusSuccess,
    StatusInvalidArgument,
    StatusInvalidValue,
} Status;

static Status runTryCatch(int a, int b) {
    Status status = StatusSuccess;

    TRY {
        /* This is compact, yet still very explicit. */
        if (a == 0)
            RAISE(ArgumentError);
        if (b == 0)
            RAISE(ArgumentError);

        /* We don't want to allow this condition. */
        if (a > b)
            RAISE(ValueError);
    }
    /* The errors raised don't necessarily have to correspond to error codes
     * returned from the function. You can do whatever makes sense. */
    CATCH(ArgumentError) {
        status = StatusInvalidArgument;
    }
    CATCH(ValueError) {
        status = StatusInvalidValue;
    }
    FINALLY {
        /* This code is always executed! */
        return status;
    }
}

TEST(tryCatchGroup, noErrorTest) {
    TEST_ASSERT_EQUAL(StatusSuccess, runTryCatch(2, 3));
}

TEST(tryCatchGroup, catchError1Test) {
    TEST_ASSERT_EQUAL(StatusInvalidArgument, runTryCatch(0, 3));
    TEST_ASSERT_EQUAL(StatusInvalidArgument, runTryCatch(2, 0));
}

TEST(tryCatchGroup, catchError2Test) {
    TEST_ASSERT_EQUAL(StatusInvalidValue, runTryCatch(3, 2));
}

TEST_GROUP_RUNNER(tryCatchGroup) {
    RUN_TEST_CASE(tryCatchGroup, noErrorTest);
    RUN_TEST_CASE(tryCatchGroup, catchError1Test);
    RUN_TEST_CASE(tryCatchGroup, catchError2Test);
}
