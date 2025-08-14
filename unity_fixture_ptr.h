//- Copyright (c) 2010 James Grenning and Contributed to Unity Project
/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#ifndef UNITY_FIXTURE_PTR_H_
#define UNITY_FIXTURE_PTR_H_

#include "unity.h"
#include "unity_fixture_internals.h"
#include "unity_fixture_malloc_overrides.h"
#include "unity_internals.h"

// CppUTest Compatibility Macros
#define UT_PTR_SET(ptr, newPointerValue) UnityPointer_Set((void **)&ptr, (void *)newPointerValue)

#endif /* UNITY_FIXTURE_PTR_H_ */
