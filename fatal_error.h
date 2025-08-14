/**
 * @file
 * Assert handler.
 *
 * Assert handler which handles run-time error checking.
 * It also makes use of the Logger component.
 *
 * Copyright Nuvation Research Corporation 2013-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_FATALERROR_H_
#define NUVC_FATALERROR_H_

#include <stdint.h>
#include <stdio.h>

#ifdef NDEBUG
#define PARM_CRASH_IF(ignore1, ignore2, ignore3)
#define CRASH_IF(ignore1, ignore2)
#define ASSERT(ignore1)
#else
/**
 * parm is the parameter at the time of the crash
 * cond is the condition that caused the failure
 * text is the user message
 *
 * Example usage: PARM_CRASH_IF(len, len > MaxLen, "msg too long");
 */
#define PARM_CRASH_IF(parm, cond, text) ((cond) ? (void)ParmCrash(parm, #cond, text, __FILE__, __LINE__) : (void)0)
/**
 * cond is the condition that caused the failure
 * text is the user message
 */
#define CRASH_IF(cond, text) ((cond) ? (void)ParmCrash(0, #cond, text, __FILE__, __LINE__) : (void)0)
/**
 * cond is the condition that caused the failure
 */
#define ASSERT(cond) ((cond) ? (void)0 : (void)ParmCrash(0, "", "", __FILE__, __LINE__))
#endif /* NDEBUG */

/**
 * Function used by the PARAM_CRASH_IF(), CRASH_IF(), and ASSERT() macros.
 */
void ParmCrash(int parm, const char *pCond, const char *pText, const char *pFileName, int lineNum);

#endif /* NUVC_FATALERROR_H_ */
