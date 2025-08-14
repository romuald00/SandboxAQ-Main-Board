/**
 * @file
 * Outline of the API to convert between host and big- and little-endian.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_BYTE_ORDER_H_
#define NUVC_BYTE_ORDER_H_

#include <stdint.h>

/* Implement flip functions and then re-define them as necessary. */
static inline uint32_t __flip32(const uint32_t number) {
    return ((number /* & 0x000000ff */) << 24 | (number & 0x0000ff00) << 8 | (number & 0x00ff0000) >> 8 |
            (number /* & 0xff000000 */) >> 24);
}

static inline uint16_t __flip16(const uint16_t number) {
    return ((number /* & 0x00ff */) << 8 | (number /* & 0xff00 */) >> 8);
}

/* Re-define the functions for each platform. */
#if defined(__MSP430F6736__)
#define htol32(host32) (host32)
#define htol16(host16) (host16)
#define ltoh32(little32) (little32)
#define ltoh16(little16) (little16)
#define htob32(host32) __flip32(host32)
#define htob16(host16) __flip16(host16)
#define btoh32(big32) __flip32(big32)
#define btoh16(big16) __flip16(big16)

#elif defined(__DSPC5535__)
#define htol32(host32) __flip32(host32)
#define htol16(host16) __flip16(host16)
#define ltoh32(little32) __flip32(little32)
#define ltoh16(little16) __flip16(little16)
#define htob32(host32) (host32)
#define htob16(host16) (host16)
#define btoh32(big32) (big32)
#define btoh16(big16) (big16)

/* x86 */
#else
#define htol32(host32) (host32)
#define htol16(host16) (host16)
#define ltoh32(little32) (little32)
#define ltoh16(little16) (little16)
#define htob32(host32) __flip32(host32)
#define htob16(host16) __flip16(host16)
#define btoh32(big32) __flip32(big32)
#define btoh16(big16) __flip16(big16)

#endif /* defined(__MSP430F6736__) */

#endif /* NUVC_BYTE_ORDER_H_ */
