/**
 * @file
 * This file defines library for secure string comparison.
 *
 * Copyright Nuvation Research Corporation 2018-2019. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_SECURE_STRNCMP_H_
#define NUVC_SECURE_STRNCMP_H_

#include "stdint.h"

/**
 * Constant time implementation of strncmp which is suitable for checking passwords.
 *
 * @param[in] input          untrusted data to compare
 * @param[in] reference      trusted data to compare
 * @param[in] size           maximum length to compare
 * @return                   0 if strings are equal
 */
int32_t securestrncmp(const char *input, const char *reference, uint32_t size);

#endif /* NUVC_SECURE_STRNCMP_H_ */
