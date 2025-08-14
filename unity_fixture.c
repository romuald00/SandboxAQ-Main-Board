//- Copyright (c) 2010 James Grenning and Contributed to Unity Project
/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include "unity/unity_fixture.h"
#include "unity/unity_internals.h"
#include "unity/unity_mem_alloc.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

UNITY_FIXTURE_T UnityFixture;

// If you decide to use the function pointer approach.
// int (*outputChar)(int) = putchar;

int verbose = 0;

void setUp(void) { /*does nothing*/
}
void tearDown(void) { /*does nothing*/
}

void announceTestRun(int runNumber) {
    UnityPrint("Unity test run ");
    UnityPrintNumber(runNumber + 1);
    UnityPrint(" of ");
    UnityPrintNumber(UnityFixture.RepeatCount);
    UNITY_OUTPUT_CHAR('\n');
}

int UnityMain(int argc, char *argv[], void (*runAllTests)()) {
    int result = UnityGetCommandLineOptions(argc, argv);
    unsigned int r;
    if (result != 0)
        return result;

    Unity.TestFile = NULL;
    Unity.CurrentTestName = NULL;
    Unity.CurrentTestLineNumber = 0;
    Unity.NumberOfTests = 0;
    Unity.TestFailures = 0;
    Unity.TestIgnores = 0;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;

    for (r = 0; r < UnityFixture.RepeatCount; r++) {
        announceTestRun(r);
        UnityBegin();
        runAllTests();
        UNITY_OUTPUT_CHAR('\n');
        UnityEnd();
    }

    if (Unity.NumberOfTests == 0) {
        // No tests were run, return -1
        return -1;
    } else {
        // Return the number of failures
        return UnityFailureCount();
    }
}

static int selected(const char *filter, const char *name) {
    if (filter == 0)
        return 1;
    return strstr(name, filter) ? 1 : 0;
}

static int testSelected(const char *test) {
    return selected(UnityFixture.NameFilter, test);
}

static int groupSelected(const char *group) {
    return selected(UnityFixture.GroupFilter, group);
}

static void runTestCase() {
}

void UnityTestRunner(unityfunction *setup,
                     unityfunction *testBody,
                     unityfunction *teardown,
                     const char *printableName,
                     const char *group,
                     const char *name,
                     const char *file,
                     int line) {
    if (testSelected(name) && groupSelected(group)) {
        Unity.CurrentTestFailed = 0;
        Unity.TestFile = file;
        Unity.CurrentTestName = printableName;
        Unity.CurrentTestLineNumber = line;
        if (!UnityFixture.Verbose)
            UNITY_OUTPUT_CHAR('.');
        else
            UnityPrint(printableName);

        Unity.NumberOfTests++;
        UnityMalloc_StartTest();
        // UnityPointer_Init();

        runTestCase();
        if (TEST_PROTECT()) {
            setup();
            testBody();
        }
        if (TEST_PROTECT()) {
            teardown();
        }
        if (TEST_PROTECT()) {
            // UnityPointer_UndoAllSets();
            if (!Unity.CurrentTestFailed)
                UnityMalloc_EndTest();
            UnityConcludeFixtureTest();
        } else {
            // aborting - jwg - di i need these for the other TEST_PROTECTS?
        }
    }
}

void UnityIgnoreTest() {
    Unity.NumberOfTests++;
    Unity.CurrentTestIgnored = 1;
    UNITY_OUTPUT_CHAR('!');
}

//-------------------------------------------------
// Malloc and free stuff
//

int malloc_count;
int malloc_fail_countdown = MALLOC_DONT_FAIL;

void UnityMalloc_MakeMallocFailAfterCount(int countdown) {
    malloc_fail_countdown = countdown;
}

void UnityMalloc_StartTest() {
    malloc_count = 0;
    malloc_fail_countdown = MALLOC_DONT_FAIL;
}

void UnityMalloc_EndTest() {
    malloc_fail_countdown = MALLOC_DONT_FAIL;
    if (malloc_count != 0) {
        TEST_FAIL_MESSAGE("This test leaks!");
    }
}

//--------------------------------------------------------
// Utility functions
int UnityFailureCount() {
    return Unity.TestFailures;
}

int UnityGetCommandLineOptions(int argc, char *argv[]) {
    int i;
    UnityFixture.Verbose = 0;
    UnityFixture.GroupFilter = 0;
    UnityFixture.NameFilter = 0;
    UnityFixture.RepeatCount = 1;

    if (argc == 1)
        return 0;

    for (i = 1; i < argc;) {
        if (strcmp(argv[i], "-v") == 0) {
            UnityFixture.Verbose = 1;
            i++;
        } else if (strcmp(argv[i], "-g") == 0) {
            i++;
            if (i >= argc)
                return 1;
            UnityFixture.GroupFilter = argv[i];
            i++;
        } else if (strcmp(argv[i], "-n") == 0) {
            i++;
            if (i >= argc)
                return 1;
            UnityFixture.NameFilter = argv[i];
            i++;
        } else if (strcmp(argv[i], "-r") == 0) {
            UnityFixture.RepeatCount = 2;
            i++;
            if (i < argc) {
                if (*(argv[i]) >= '0' && *(argv[i]) <= '9') {
                    UnityFixture.RepeatCount = atoi(argv[i]);
                    i++;
                }
            }
        } else {
            UnityPrint(" Unknown option");
            UNITY_OUTPUT_CHAR('\n');
            return 1;
        }
    }
    return 0;
}

void UnityConcludeFixtureTest() {
    if (Unity.CurrentTestIgnored) {
        Unity.TestIgnores++;
    } else if (!Unity.CurrentTestFailed) {
        if (UnityFixture.Verbose) {
            UnityPrint(" PASS");
            UNITY_OUTPUT_CHAR('\n');
        }
    } else if (Unity.CurrentTestFailed) {
        Unity.TestFailures++;
    }

    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
}
