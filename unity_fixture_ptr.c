//- Copyright (c) 2010 James Grenning and Contributed to Unity Project
/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include "unity/unity_fixture_ptr.h"
#include "unity/unity_fixture.h"
#include "unity/unity_internals.h"
#include "unity/unity_mem_alloc.h"
#include <stdlib.h>
#include <string.h>

//--------------------------------------------------------
// Automatic pointer restoration functions
typedef struct _PointerPair {
    struct _PointerPair *next;
    void **pointer;
    void *old_value;
} PointerPair;

enum { MAX_POINTERS = 50 };
static PointerPair pointer_store[MAX_POINTERS];
static int pointer_index = 0;

void UnityPointer_Init() {
    pointer_index = 0;
}

void UnityPointer_Set(void **pointer, void *newValue) {
    if (pointer_index >= MAX_POINTERS)
        TEST_FAIL_MESSAGE("Too many pointers set");

    pointer_store[pointer_index].pointer = pointer;
    pointer_store[pointer_index].old_value = *pointer;
    *pointer = newValue;
    pointer_index++;
}

void UnityPointer_UndoAllSets() {
    while (pointer_index > 0) {
        pointer_index--;
        *(pointer_store[pointer_index].pointer) = pointer_store[pointer_index].old_value;
    }
}
