//- Copyright (c) 2010 James Grenning and Contributed to Unity Project
/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include "unity/unity_mem_alloc.h"
#include "unity/unity_fixture.h"
#include "unity/unity_internals.h"
#include <stdlib.h>
#include <string.h>

typedef struct GuardBytes {
    size_t size;
    char guard[sizeof(int)];
} Guard;

static const char *end = "END";

void *unity_malloc(size_t size) {
    char *mem;
    Guard *guard;

    if (malloc_fail_countdown != MALLOC_DONT_FAIL) {
        if (malloc_fail_countdown == 0)
            return 0;
        malloc_fail_countdown--;
    }

    malloc_count++;

    guard = (Guard *)malloc(size + sizeof(Guard) + 4);
    guard->size = size;
    mem = (char *)&(guard[1]);
    memcpy(&mem[size], end, strlen(end) + 1);

    return (void *)mem;
}

static int isOverrun(void *mem) {
    Guard *guard = (Guard *)mem;
    char *memAsChar = (char *)mem;
    guard--;

    return strcmp(&memAsChar[guard->size], end) != 0;
}

static void release_memory(void *mem) {
    Guard *guard = (Guard *)mem;
    guard--;

    malloc_count--;
    free(guard);
}

void unity_free(void *mem) {
    int overrun = isOverrun(mem); // strcmp(&memAsChar[guard->size], end) != 0;
    release_memory(mem);
    if (overrun) {
        TEST_FAIL_MESSAGE("Buffer overrun detected during free()");
    }
}

void *unity_calloc(size_t num, size_t size) {
    void *mem = unity_malloc(num * size);
    memset(mem, 0, num * size);
    return mem;
}

void *unity_realloc(void *oldMem, size_t size) {
    Guard *guard = (Guard *)oldMem;
    //    char* memAsChar = (char*)oldMem;
    void *newMem;

    if (oldMem == 0)
        return unity_malloc(size);

    guard--;
    if (isOverrun(oldMem)) {
        release_memory(oldMem);
        TEST_FAIL_MESSAGE("Buffer overrun detected during realloc()");
    }

    if (size == 0) {
        release_memory(oldMem);
        return 0;
    }

    if (guard->size >= size)
        return oldMem;

    newMem = unity_malloc(size);
    memcpy(newMem, oldMem, size);
    unity_free(oldMem);
    return newMem;
}
