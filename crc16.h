/**
 * @file
 *
 * Copyright 2001-2010 Georges Menie (www.menie.org)
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Modifications by Philipp Schrader (www.nuvation.com)
 */

#ifndef NUVC_CRC16_H_
#define NUVC_CRC16_H_

#include <stdint.h>

#define INITIAL_CRC16_CCITT UINT16_C(0)

/**
 * Calculates the CRC16 of a given buffer.
 *
 * This function uses the CCITT polynomial for computation.
 *
 * The crc parameter designates the starting CRC value to use. This allows for
 * multiple invocations of this function to check segments that cannot fit
 * wholly into memory. The first invocation should have *crc set to
 * INITIAL_CRC16_CCITT.
 *
 * @param[in]     buf   data of which the crc is to be computed
 * @param[in]     len   length of the data
 * @param[in,out] crc   points to the start crc and final crc
 */
extern void crc16_ccitt(const void *buf, uint16_t len, uint16_t *crc);

#endif /* NUVC_CRC16_H_ */
