/**
 * @file
 * API of the lean sprintf implementation.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_SPRINTF_H_
#define NUVC_SPRINTF_H_

#ifdef USE_NUVC_SNPRINTF

#define MAX_SNPRINTF_SIZE 255

/**
 * Lean implementation of the famous snprintf function.
 *
 * The caller has to make sure that there is enough buffer space allocated to
 * complete the action successfully.
 *
 * @param[in] buffer    points to the buffer to print into
 * @param[in] size      number of characters buffer can hold
 * @param[in] format    standard printf-like format + var args
 * @return              number of characters printed into buffer
 */
extern int snprintf(char *buffer, size_t size, const char *format, ...);

#endif /* USE_NUVC_SNPRINTF */

#endif /* NUVC_SPRINTF_H_ */
