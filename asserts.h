/**
 * @file
 * This file takes care of the asserts that are placed throughout the code.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_ASSERTS_H_
#define NUVC_ASSERTS_H_

#include <assert.h>

#if defined(__REMOVE_ASSERTS__)
#define ASSERT_TRUE(condition)
#else
#define ASSERT_TRUE(condition) assert(condition)
#endif /* defined(__REMOVE_ASSERTS__) */

#endif /* NUVC_ASSERTS_H_ */
